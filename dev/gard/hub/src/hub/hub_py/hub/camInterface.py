# -----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
# -----------------------------------------------------------------------------
# Camera Interface Library
#
# A Python library for camera management and video streaming
# Providing robust, user-friendly and abstracted interfaces for camera operations
# Supporting both USB cameras (via V4L2) and Raspberry Pi Camera Module (via Picamera2)
#
# The camInterface.py is designed to provide a unified interface for camera
# operations, including detection, setup, streaming, and resource management.
#
# Interfaces of CamInterface class:
#
# Camera Detection:
# 1.  [CamInterface] detect_cameras() - Detects all available cameras (USB and Pi Camera)
#     Note: This is called by __init__(), not required to call explicitly
# 2.  [CamInterface] get_CLNX_camera_id() - Gets device ID of Lattice CLNX USB camera
# 3.  [CamInterface] get_CPNX_camera_id() - Gets camera ID of Raspberry Pi Camera Module
# 4.  [CamInterface] _is_capture_device() - Checks if /dev/video device supports video capture
# 5.  [CamInterface] _detect_cameras_fallback() - Fallback method for camera detection
#
# Camera Setup:
# 1.  [CamInterface] setup_camera() - Configures camera with resolution and type
# 2.  [CamInterface] start_camera() - Initializes and starts camera streaming
# 3.  [CamInterface] stop_camera() - Stops streaming and releases camera resources
#
# Camera Operations:
# 1.  [CamInterface] get_camera_status() - Gets current status of a camera
# 2.  [CamInterface] get_camera_stream() - Gets raw video capture stream
# 3.  [CamInterface] generate_frames() - Generator function for MJPEG frame streaming
# 4.  [CamInterface] are_both_cameras_running() - Checks if both USB and Pi cameras are active
#
# Camera Frame Processing:
# 1.  [CamInterface] _validate_and_encode_frame() - Validates and encodes frames to JPEG
#
# Image Operations:
# 1.  [CamInterface] capture_image_from_gard() - Captures rescaled image from GARD system
#     Returns image info dictionary and binary image data
#
# Resource Management:
# 1.  [CamInterface] cleanup_resources() - Releases all camera resources and stops all streams
# -----------------------------------------------------------------------------

import os
import cv2
import threading
import time
import glob
import subprocess
import logging
import traceback
import numpy as np
import ctypes as ct

try:
    from picamera2 import Picamera2
    PICAMERA2_AVAILABLE = True
except ImportError:
    PICAMERA2_AVAILABLE = False


class DummyLogger:
    """Logger replacement using print statements."""

    def info(self, message):
        print(f"[INFO] {message}")

    def error(self, message):
        print(f"[ERROR] {message}")

    def warning(self, message):
        print(f"[WARN] {message}")

    def debug(self, message):
        print(f"[DEBUG] {message}")

class HubImgOpsCtx(ct.Structure):
    """ctypes structure corresponding to struct hub_img_ops_ctx in hub.h"""
    _fields_ = [
        ("camera_id", ct.c_uint8),
        ("p_image_buffer", ct.c_void_p),
        ("image_buffer_size", ct.c_uint32),
        ("image_buffer_address", ct.c_uint32),
        ("h_size", ct.c_uint16),
        ("v_size", ct.c_uint16),
        ("image_format", ct.c_uint),
    ]



class CamInterface:
    """Camera interface for managing camera operations."""

    def __init__(self, logger=None):
        """Initialize camera interface with optional logger."""
        self.camera_data_context = {}
        self.logger = logger if logger else DummyLogger()
        self.encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), 50]
        self.available_cameras = self.detect_cameras()
        self.cleanup_lock = threading.Lock()
        self.cleanup_done = False
        self.logger.debug("CamInterface initialized.")

    def detect_cameras(self):
        """Detect connected camera devices using v4l2-ctl or fallback method."""
        self.available_cameras = {}

        if PICAMERA2_AVAILABLE:
            try:
                camera_list = Picamera2.global_camera_info()
                for camera_info in camera_list:
                    if isinstance(camera_info, dict) and camera_info['Model'] == "imx219":
                            self.available_cameras["pi"] = {
                                "type": "picamera2",
                                "name": "Raspberry Pi Camera Module " + camera_info['Model']
                            }
                            self.logger.debug(f"Detected Raspberry Pi Camera Module: {camera_info['Model']}")
                            break
                
                if self.available_cameras.get("pi") is None:
                    self.logger.warning("No Raspberry Pi Camera Module detected")
            except Exception as e:
                self.logger.debug(f"Exception while detecting Picamera2 cameras: {e}")

        try:
            result = subprocess.run(
                ["v4l2-ctl", "--list-devices"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            
            if result.returncode == 0:
                lines = result.stdout.split("\n")
                current_device_name = None
                
                for line in lines:
                    line = line.strip()
                    if line and not line.startswith("/dev/video") and not line.startswith("/dev/media"):
                        current_device_name = line
                    elif line.startswith("/dev/video"):
                        try:
                            device_id = int(line.replace("/dev/video", ""))
                            if current_device_name and "platform:" not in current_device_name.lower():
                                if self._is_capture_device(device_id):
                                    self.available_cameras[device_id] = {
                                        "type": "usb",
                                        "name": current_device_name
                                    }
                                    self.logger.debug(
                                        f"Detected USB camera: {current_device_name} at /dev/video{device_id}"
                                    )
                        except ValueError:
                            continue
            else:
                self.logger.warning("v4l2-ctl command failed, falling back to glob method")
                self._detect_cameras_fallback()
                
        except Exception as e:
            error_msg = {
                subprocess.TimeoutExpired: "v4l2-ctl command timed out",
                FileNotFoundError: "v4l2-ctl not found"
            }.get(type(e), f"Error using v4l2-ctl for camera detection: {e}")
            self.logger.warning(f"{error_msg}, falling back to glob method")
            self._detect_cameras_fallback()

        return self.available_cameras

    def _is_capture_device(self, device_id):
        """Check if /dev/video device supports video capture capabilities."""
        try:
            result = subprocess.run(
                ["v4l2-ctl", "--device", f"/dev/video{device_id}", "--list-formats"],
                capture_output=True,
                text=True,
                timeout=2,
            )
            
            if result.returncode == 0:
                output = result.stdout
                if "Video Capture" in output or "Video Capture Multiplanar" in output:
                    return True
            return False
            
        except (subprocess.TimeoutExpired, FileNotFoundError, Exception) as e:
            self.logger.debug(f"Could not check capabilities for /dev/video{device_id}: {e}")
            return False

    def _detect_cameras_fallback(self):
        """Fallback method to detect cameras by opening each /dev/video* device."""
        video_devices = glob.glob("/dev/video*")
        for device_path in sorted(
            video_devices,
            key=lambda x: (
                int(x.replace("/dev/video", ""))
                if x.replace("/dev/video", "").isdigit()
                else 999
            ),
        ):
            try:
                device_id = int(device_path.replace("/dev/video", ""))
            except ValueError:
                continue
            if self._is_capture_device(device_id):
                self.available_cameras[device_id] = {
                    "type": "usb",
                    "name": f"USB Camera /dev/video{device_id}"
                }
                self.logger.debug(f"Detected USB camera: /dev/video{device_id}")

    def get_CLNX_camera_id(self):
        """
        Get the device ID of the Lattice CLNX camera (USB camera).
        Uses stored camera information from available_cameras to avoid re-querying v4l2.

        Returns:
            int: Device ID of Lattice camera, or -1 if not found
        """
        for camera_id, camera_info in self.available_cameras.items():
            if isinstance(camera_id, str):
                continue
            
            if isinstance(camera_info, dict):
                camera_name = camera_info["name"]
                if camera_info["type"] == "usb" and "Lattice" in camera_name and "USB" in camera_name:
                    self.logger.info(
                        f"Lattice CLNX camera found at /dev/video{camera_id}: {camera_name}"
                    )
                    return camera_id
        
        self.logger.warning(
            "Lattice camera not found in available_cameras, defaulting to device 0"
        )
        return -1

    def get_CPNX_camera_id(self):
        """Get camera ID for Pi Camera (CPNX). Returns 'pi' if available, None otherwise."""
        if "pi" in self.available_cameras:
            return "pi"
        else:
            self.logger.warning("CPNX (Pi Camera) not detected")
            return None

    def get_camera_stream(self, camera_id=0):
        """Get video stream from specified camera device."""
        cap = cv2.VideoCapture(camera_id)
        if not cap.isOpened():
            raise ValueError(f"Camera with ID {camera_id} could not be opened.")
        return cap

    def get_camera_status(self, camera_id):
        """Get current status of a camera."""
        if camera_id in self.camera_data_context:
            return self.camera_data_context[camera_id]["camera_status"]
        else:
            return "Unknown"
    

    def setup_camera(self, camera_id=0, width=1920, height=1080, camera_type=None):
        """Setup camera with specified resolution and configuration."""
        if camera_id is None:
            camera_id = self.get_CPNX_camera_id()

        if camera_id not in self.available_cameras:
            raise ValueError(
                f"Camera with ID {camera_id} not detected. Available: {list(self.available_cameras.keys())}"
            )

        if camera_type is None:
            camera_info = self.available_cameras[camera_id]
            if isinstance(camera_info, dict):
                camera_type = camera_info.get("type", "usb")
            else:
                camera_type = camera_info

        if camera_type not in ["usb", "picamera2"]:
            raise ValueError(
                f"Invalid camera_type: {camera_type}. Must be 'usb' or 'picamera2'."
            )

        if camera_type == "picamera2" and not PICAMERA2_AVAILABLE:
            raise ValueError(
                "Picamera2 requested but not installed. Use system site-packages to use picamera2."
            )

        self.camera_data_context[camera_id] = {}
        try:
            self.camera_data_context[camera_id]["camera_id"] = camera_id
            self.camera_data_context[camera_id]["camera_type"] = camera_type
            self.camera_data_context[camera_id]["width"] = width
            self.camera_data_context[camera_id]["height"] = height
            self.camera_data_context[camera_id]["camera_stream"] = None
            self.camera_data_context[camera_id]["camera_lock"] = threading.Lock()
            self.camera_data_context[camera_id]["generator_lock"] = threading.Lock()
            self.camera_data_context[camera_id]["stop_flag"] = threading.Event()
            self.camera_data_context[camera_id]["active_generators"] = 0
            self.camera_data_context[camera_id]["camera_status"] = "Stopped"

            self.logger.info(
                f"Camera {camera_id} ({camera_type}) setup with resolution {width}x{height}."
            )
        except Exception as e:
            raise CamInterfaceError(camera_id, "setup_camera")

    def start_camera(self, camera_id):
        """
        Start camera streaming with configured settings.
        Initializes camera hardware, sets format/resolution, and prepares for frame generation.

        Args:
            camera_id: Camera device ID to start

        Returns:
            bool: True if camera started successfully

        Raises:
            ValueError: If camera not set up
            CamInterfaceError: If start fails
        """
        if camera_id not in self.camera_data_context:
            if camera_id == "pi":
                raise ValueError(
                    f"Camera with ID {camera_id} is not set up. Run 'enable_lsc219_cam' first."
                )
            else:
                raise ValueError(
                    f"Camera with ID {camera_id} is not set up. Run setup_camera() first."
                )
        try:
            stop_flag = self.camera_data_context[camera_id].get("stop_flag")
            if stop_flag is not None:
                stop_flag.clear()
            with self.camera_data_context[camera_id]["generator_lock"]:
                self.camera_data_context[camera_id]["active_generators"] = 0
            
            time.sleep(0.5)
            
            with self.camera_data_context[camera_id]["camera_lock"]:
                if self.camera_data_context[camera_id]["camera_stream"] is None:
                    camera_type = self.camera_data_context[camera_id]["camera_type"]
                    width = self.camera_data_context[camera_id]["width"]
                    height = self.camera_data_context[camera_id]["height"]

                    if camera_type == "usb":
                        self.logger.info(f"Opening USB camera {camera_id} (/dev/video{camera_id})...")
                        cap = cv2.VideoCapture(camera_id, cv2.CAP_V4L2)

                        if not cap.isOpened():
                            self.camera_data_context[camera_id]["camera_stream"] = None
                            self.camera_data_context[camera_id]["camera_status"] = "Error"
                            self.logger.error(
                                f"Error: Could not open camera device {camera_id} (/dev/video{camera_id})."
                            )
                            return False

                        self.logger.info(f"USB camera {camera_id} opened successfully")
                        time.sleep(0.2)
                        
                        try:
                            fourcc_result = cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"YUYV"))
                            self.logger.debug(f"Set FOURCC to YUYV: {fourcc_result}")
                            time.sleep(0.1)
                        except Exception as e:
                            self.logger.warning(f"Failed to set FOURCC to YUYV: {e}")
                        
                        try:
                            width_result = cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
                            height_result = cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
                            self.logger.info(f"Set resolution to {width}x{height}: width={width_result}, height={height_result}")
                            time.sleep(0.1)
                        except Exception as e:
                            self.logger.error(f"Failed to set resolution: {e}")

                        try:
                            fps_result = cap.set(cv2.CAP_PROP_FPS, 30)
                            self.logger.info(f"Set FPS to 30: {fps_result}")
                            time.sleep(0.1)
                        except Exception as e:
                            self.logger.error(f"Failed to set FPS: {e}")

                        self.logger.info("Initializing camera buffers with dummy frame reads...")
                        try:
                            successful_reads = 0
                            for i in range(5):
                                ret, _ = cap.read()
                                if ret:
                                    successful_reads += 1
                                    self.logger.debug(f"Dummy frame {i+1} read successful")
                                    time.sleep(0.05)
                                    if successful_reads >= 2:
                                        break
                                else:
                                    self.logger.debug(f"Dummy frame {i+1} read failed, retrying...")
                                    time.sleep(0.1)
                        except Exception as e:
                            self.logger.warning(f"Error reading dummy frames (may be normal): {e}")

                        self.logger.info("Waiting for camera to fully initialize...")
                        time.sleep(0.3)

                        self.camera_data_context[camera_id]["camera_stream"] = cap
                        self.logger.info(f"USB camera {camera_id} initialized and ready")

                    elif camera_type == "picamera2":
                        time.sleep(0.5)

                        picam2 = Picamera2()
                        video_config = picam2.create_video_configuration(
                            main={"format": "RGB888", "size": (width, height)}
                        )
                        picam2.configure(video_config)
                        picam2.start()

                        time.sleep(0.5)
                        self.camera_data_context[camera_id]["camera_stream"] = picam2

                    self.camera_data_context[camera_id]["camera_status"] = "Running"
                    self.logger.info(
                        f"Camera device {camera_id} ({camera_type}) started."
                    )

                    return True
        except Exception as e:
            raise CamInterfaceError(camera_id, "start_camera")

    def stop_camera(self, camera_id):
        """
        Stop camera streaming and release resources.
        Signals generators to exit, waits for completion, then releases camera hardware.

        Args:
            camera_id: Camera device ID to stop

        Returns:
            bool: True if camera stopped successfully, False if camera was not set up

        Raises:
            CamInterfaceError: If stop fails
        """
        if camera_id not in self.camera_data_context:
            self.logger.debug(
                f"Camera with ID {camera_id} is not set up, nothing to stop."
            )
            return False
        try:
            stop_flag = self.camera_data_context[camera_id].get("stop_flag")
            generator_lock = self.camera_data_context[camera_id].get("generator_lock")
            
            if stop_flag is not None:
                self.logger.info(f"Setting stop flag for camera {camera_id}...")
                stop_flag.set()
            
            if generator_lock is not None:
                self.logger.info(f"Waiting for generators to finish for camera {camera_id}...")
                timeout = 2.0  # Reduced timeout for faster shutdown
                start_time = time.time()
                
                while time.time() - start_time < timeout:
                    with generator_lock:
                        active_count = self.camera_data_context[camera_id].get("active_generators", 0)
                        if active_count == 0:
                            self.logger.info(f"All generators finished for camera {camera_id}")
                            break
                        self.logger.debug(f"Waiting for {active_count} generator(s) to finish for camera {camera_id}...")
                    time.sleep(0.05)  # Check every 50ms instead of 100ms
                else:
                    with generator_lock:
                        active_count = self.camera_data_context[camera_id].get("active_generators", 0)
                        if active_count > 0:
                            self.logger.warning(
                                f"Timeout waiting for {active_count} generator(s) to finish for camera {camera_id}, proceeding with stop"
                            )
                            self.camera_data_context[camera_id]["active_generators"] = 0
            
            with self.camera_data_context[camera_id]["camera_lock"]:
                if self.camera_data_context[camera_id]["camera_stream"] is not None:
                    camera_type = self.camera_data_context[camera_id]["camera_type"]

                    if camera_type == "usb":
                        try:
                            self.camera_data_context[camera_id]["camera_stream"].release()
                        except Exception as e:
                            self.logger.warning(f"Error releasing USB camera {camera_id}: {e}")
                    elif camera_type == "picamera2":
                        try:
                            self.camera_data_context[camera_id]["camera_stream"].stop()
                        except Exception as e:
                            self.logger.debug(f"Error stopping Pi camera {camera_id}: {e}")
                        try:
                            self.camera_data_context[camera_id]["camera_stream"].close()
                        except Exception as e:
                            self.logger.debug(f"Error closing Pi camera {camera_id}: {e}")

                    self.camera_data_context[camera_id]["camera_stream"] = None
                    self.camera_data_context[camera_id]["camera_status"] = "Stopped"

                    self.logger.info(f"Camera device {camera_id} stopped.")
                    return True
        except Exception as e:
            self.logger.error(f"Error stopping camera {camera_id}: {e}")
            self.logger.error(traceback.format_exc())
            raise CamInterfaceError(camera_id, "stop_camera")

    def are_both_cameras_running(self):
        """
        Check if both USB and Pi cameras are currently running.
        Used to optimize FPS when dual cameras are active.

        Returns:
            bool: True if both cameras are running, False otherwise
        """
        usb_running = False
        pi_running = False

        for cam_id, context in self.camera_data_context.items():
            if context.get("camera_status") == "Running":
                if context.get("camera_type") == "usb":
                    usb_running = True
                elif context.get("camera_type") == "picamera2":
                    pi_running = True

        return usb_running and pi_running

    def _validate_and_encode_frame(self, frame, camera_id):
        """
        Validate frame and encode it to JPEG format.
        
        Args:
            frame: numpy array representing the frame
            camera_id: Camera ID for error reporting
            
        Returns:
            bytes: Encoded JPEG frame data
            
        Raises:
            FrameEncodingError: If frame validation or encoding fails
        """
        if frame.size == 0:
            raise FrameEncodingError(
                camera_id, 
                "empty_frame",
                f"Frame is empty (size=0)"
            )
        
        if len(frame.shape) < 2:
            raise FrameEncodingError(
                camera_id,
                "invalid_shape",
                f"Frame has invalid shape: {frame.shape}"
            )
        
        channels = frame.shape[2] if len(frame.shape) == 3 else 1
        if channels not in [1, 3, 4]:
            raise FrameEncodingError(
                camera_id,
                "invalid_channels",
                f"Frame has invalid number of channels: {channels} (shape: {frame.shape})"
            )
        
        try:
            ret, buffer = cv2.imencode(".jpg", frame, self.encode_param)
            if not ret:
                raise FrameEncodingError(
                    camera_id,
                    "encode_failed",
                    "cv2.imencode returned False"
                )
            
            frame_bytes = buffer.tobytes()
            if len(frame_bytes) == 0:
                raise FrameEncodingError(
                    camera_id,
                    "empty_encoded",
                    "Encoded frame is empty"
                )
            
            return frame_bytes
            
        except FrameEncodingError:
            raise
        except Exception as e:
            raise FrameEncodingError(
                camera_id,
                "encode_exception",
                f"Exception during encoding: {str(e)}",
                original_error=e
            ) from e

    def generate_frames(self, camera_id):
        """
        Generator function that continuously yields JPEG frames for streaming.
        Automatically adjusts FPS: ~30 FPS for single camera, ~10 FPS for dual cameras.
        Frame capture and encoding happen outside locks to prevent blocking between cameras.

        Args:
            camera_id: Camera device ID to generate frames from

        Yields:
            bytes: MJPEG formatted frame data
        """
        if camera_id not in self.camera_data_context:
            return
        
        stop_flag = self.camera_data_context[camera_id].get("stop_flag")
        if stop_flag is None:
            return
        
        with self.camera_data_context[camera_id]["generator_lock"]:
            self.camera_data_context[camera_id]["active_generators"] += 1
        
        consecutive_failures = 0
        max_consecutive_failures = 5
        last_error_log_time = {}
        error_log_interval = 5.0  # Only log same error once per 5 seconds
        
        try:
            while True:
                if stop_flag.is_set():
                    self.logger.debug(f"Stop flag set for camera {camera_id}, exiting generator")
                    break
                
                sleep_time = 0
                if self.are_both_cameras_running():
                    sleep_time = 0.2  # 200ms total
                else:
                    sleep_time = 0.033  # 33ms total

                time.sleep(sleep_time)

                if stop_flag.is_set():
                    break

                stream = None
                camera_type = None
                
                try:
                    with self.camera_data_context[camera_id]["camera_lock"]:
                        camera_status = self.camera_data_context[camera_id]["camera_status"]
                        if camera_status != "Running":
                            break
                        
                        stream = self.camera_data_context[camera_id]["camera_stream"]
                        camera_type = self.camera_data_context[camera_id].get(
                            "camera_type", "usb"
                        )
                except Exception as e:
                    self.logger.error(f"Error accessing camera context: {e}")
                    break

                if stream is None:
                    break

                success = False
                frame = None

                try:
                    if camera_type == "usb":
                        if stream.isOpened():
                            try:
                                success, frame = stream.read()
                                if not success:
                                    consecutive_failures += 1
                                    if consecutive_failures >= max_consecutive_failures:
                                        self.logger.error(
                                            f"Too many consecutive failures for USB camera {camera_id}, exiting generator to prevent OS crash"
                                        )
                                        break
                                    self.logger.warning(
                                        f"Failed to read frame from USB camera {camera_id} (UVC error may have occurred) [{consecutive_failures}/{max_consecutive_failures}]"
                                    )
                                    time.sleep(0.4)
                                    continue
                                else:
                                    consecutive_failures = 0
                                    if frame is None:
                                        self.logger.warning(f"USB camera {camera_id} read returned None frame despite success=True")
                                        continue
                            except Exception as read_error:
                                consecutive_failures += 1
                                if consecutive_failures >= max_consecutive_failures:
                                    self.logger.error(
                                        f"Too many consecutive exceptions for USB camera {camera_id}, exiting generator to prevent OS crash"
                                    )
                                    self.logger.error(traceback.format_exc())
                                    break
                                self.logger.error(
                                    f"Exception reading frame from USB camera {camera_id}: {read_error} [{consecutive_failures}/{max_consecutive_failures}]"
                                )
                                self.logger.error(traceback.format_exc())
                                time.sleep(0.5)
                                continue
                        else:
                            self.logger.error(f"USB camera {camera_id} stream is not opened")
                            break
                    elif camera_type == "picamera2":
                        try:
                            frame = stream.capture_array()
                            success = frame is not None
                            if success:
                                consecutive_failures = 0
                            else:
                                self.logger.warning(f"Pi camera {camera_id} returned None frame")
                                time.sleep(0.1)
                                continue
                        except Exception as capture_error:
                            consecutive_failures += 1
                            if consecutive_failures >= max_consecutive_failures:
                                self.logger.error(
                                    f"Too many consecutive exceptions for Pi camera {camera_id}, exiting generator"
                                )
                                self.logger.error(traceback.format_exc())
                                break
                            self.logger.error(
                                f"Exception capturing frame from Pi camera {camera_id}: {capture_error} [{consecutive_failures}/{max_consecutive_failures}]"
                            )
                            self.logger.error(traceback.format_exc())
                            time.sleep(0.2)
                            continue
                except Exception as e:
                    consecutive_failures += 1
                    if consecutive_failures >= max_consecutive_failures:
                        self.logger.error(
                            f"Too many consecutive errors for camera {camera_id}, exiting generator"
                        )
                        self.logger.error(traceback.format_exc())
                        break
                    self.logger.error(
                        f"Error capturing frame from camera {camera_id}: {e} [{consecutive_failures}/{max_consecutive_failures}]"
                    )
                    self.logger.error(traceback.format_exc())
                    time.sleep(0.2)
                    continue

                if success and frame is not None:
                    try:
                        frame_bytes = self._validate_and_encode_frame(frame, camera_id)
                        yield (
                            b"--frame\r\n"
                            b"Content-Type: image/jpeg\r\n\r\n" + frame_bytes + b"\r\n"
                        )
                    except FrameEncodingError as frame_error:
                        error_key = f"{frame_error.error_type}_{camera_id}"
                        current_time = time.time()
                        if error_key not in last_error_log_time or (current_time - last_error_log_time[error_key]) >= error_log_interval:
                            if frame_error.error_type == "encode_exception":
                                self.logger.error(f"Error encoding frame for camera {camera_id}: {frame_error.message}")
                                if frame_error.original_error:
                                    self.logger.error(traceback.format_exc())
                            else:
                                self.logger.warning(f"Camera {camera_id}: {frame_error.message}, skipping")
                            last_error_log_time[error_key] = current_time
                        continue
                else:
                    if not success:
                        self.logger.debug(f"Camera {camera_id} frame read was not successful, skipping")
                    elif frame is None:
                        self.logger.debug(f"Camera {camera_id} frame is None, skipping")
        except Exception as e:
            if stop_flag.is_set():
                self.logger.debug(f"Camera {camera_id} stopped, exiting generator")
            else:
                self.logger.error(f"Error generating frames for camera {camera_id}: {e}")
                self.logger.error(traceback.format_exc())
        finally:
            with self.camera_data_context[camera_id]["generator_lock"]:
                self.camera_data_context[camera_id]["active_generators"] = max(
                    0, self.camera_data_context[camera_id]["active_generators"] - 1
                )

    def cleanup_resources(self):
        """Cleanup function to release all camera resources. Thread-safe."""
        with self.cleanup_lock:
            if self.cleanup_done:
                return
            
            self.cleanup_done = True

            try:
                cameras_to_stop = []
                for camera_id in self.camera_data_context.keys():
                    status = self.get_camera_status(camera_id)
                    if status == "Running":
                        cameras_to_stop.append(camera_id)
                
                for camera_id in cameras_to_stop:
                    try:
                        self.stop_camera(camera_id)
                    except Exception as e:
                        self.logger.error(f"Error stopping camera {camera_id}: {e}")
                
                for camera_id in list(self.camera_data_context.keys()):
                    try:
                        if "stop_flag" in self.camera_data_context[camera_id]:
                            self.camera_data_context[camera_id]["stop_flag"].set()
                        if "camera_stream" in self.camera_data_context[camera_id]:
                            stream = self.camera_data_context[camera_id]["camera_stream"]
                            if stream is not None:
                                try:
                                    if self.camera_data_context[camera_id].get("camera_type") == "usb":
                                        stream.release()
                                    else:
                                        stream.stop()
                                        stream.close()
                                except Exception:
                                    pass
                    except Exception as e:
                        self.logger.warning(f"Warning during force stop of camera {camera_id}: {e}")

            except Exception as e:
                self.logger.warning(f"Warning during cleanup: {e}")


    def capture_image_from_gard(self, hub_instance, gard, camera_id):
        """
        Capture a rescaled image from the connected GARD and get the properties of the image.
        
        Args:
            hub_instance: HUB instance
            gard_num: GARD number
            camera_id: Camera ID
            
        Returns:
            tuple: (dict, bytes) - Image info dictionary and image data, or None on failure
        """
        if not gard:
            self.logger.error(f"GARD handle is None")
            return None
        
        # Initialize image operations context
        img_ops_ctx = HubImgOpsCtx()
        img_ops_ctx.camera_id = camera_id
        
        hub_instance.hub_lib.hub_capture_rescaled_image_from_gard.argtypes = [
            ct.c_void_p,  # gard_handle_t
            ct.POINTER(HubImgOpsCtx),  # struct hub_img_ops_ctx *
        ]
        hub_instance.hub_lib.hub_capture_rescaled_image_from_gard.restype = ct.c_int
        
        ret = hub_instance.hub_lib.hub_capture_rescaled_image_from_gard(
            gard.get_gard_handle(),
            ct.byref(img_ops_ctx)
        )
        if ret != 0:  # HUB_SUCCESS = 0
            self.logger.error("Error in hub_capture_rescaled_image_from_gard!")
            return None
        
        self.logger.info("Rescaled image captured successfully")
        self.logger.info(f"Image properties: width={img_ops_ctx.h_size}, height={img_ops_ctx.v_size}, format={img_ops_ctx.image_format}")
        
        
        self.logger.info(f"Image buffer size: {img_ops_ctx.image_buffer_size}")
        self.logger.info(f"Image buffer address: {img_ops_ctx.image_buffer_address}")
        
        ret, image_data = gard.receive_data(
            img_ops_ctx.image_buffer_address,
            img_ops_ctx.image_buffer_size
        )
        if ret != 0:  # HUB_SUCCESS = 0
            self.logger.error("Error in hub_recv_data_from_gard!")
            return None
        
        self.logger.info(f"Received image data: {len(image_data)} bytes")
        
        hub_instance.hub_lib.hub_send_resume_pipeline.argtypes = [
            ct.c_void_p,  # gard_handle_t
            ct.c_uint8,   # uint8_t camera_id
        ]
        hub_instance.hub_lib.hub_send_resume_pipeline.restype = ct.c_int
        
        ret = hub_instance.hub_lib.hub_send_resume_pipeline(
            gard.get_gard_handle(),
            img_ops_ctx.camera_id
        )
        if ret != 0:  # HUB_SUCCESS = 0
            self.logger.error("Error in hub_send_resume_pipeline!")
            return None
        
        self.logger.info("Pipeline resumed successfully")

        image_info = {
            "camera_id": img_ops_ctx.camera_id,
            "width": img_ops_ctx.h_size,
            "height": img_ops_ctx.v_size,
            "format": img_ops_ctx.image_format,
            "buffer_size": img_ops_ctx.image_buffer_size,
            "buffer_address": img_ops_ctx.image_buffer_address,
        }
        
        return image_info, image_data


class CamInterfaceError(Exception):
    """Custom exception for camera interface errors."""
    
    def __init__(self, camera_id, func_name: str):
        self.message = f"Failed in {func_name} for camera {camera_id}"
        super().__init__(self.message)


class FrameEncodingError(Exception):
    """Custom exception for frame validation and encoding errors.
    Used to handle non-success scenarios gracefully without crashing."""
    
    def __init__(self, camera_id, error_type: str, message: str, original_error=None):
        """
        Args:
            camera_id: Camera ID that generated the error
            error_type: Type of error ('invalid_shape', 'invalid_channels', 'encode_failed', 'encode_exception', 'empty_frame', 'empty_encoded')
            message: Human-readable error message
            original_error: Original exception if this wraps another exception
        """
        self.camera_id = camera_id
        self.error_type = error_type
        self.message = message
        self.original_error = original_error
        super().__init__(self.message)
