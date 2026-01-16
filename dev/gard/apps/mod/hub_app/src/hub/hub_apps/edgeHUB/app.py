"""
edgeHUB Flask Application

Flask web application for edgeHUB camera and sensor management.
Provides REST API endpoints for camera control, image capture, and sensor data.
"""

import os
import sys
import json
import time
import signal
import atexit
import traceback
import subprocess
import base64
from datetime import datetime
from flask import Flask, render_template, jsonify, Response, request, send_file
import cv2
import numpy as np
import hub

app = Flask(__name__)

hub_instance = None
cam_instance = None
eve_instance = None

def load_config():
    """
    Load configuration from config.json file.
    
    This function attempts to load the configuration file from the same directory
    as this script. It first tries to read a default 'config.json' file to get
    the actual config filename from the 'files.config_filename' field, then loads
    that file. If the file is not found or contains invalid JSON, it returns an
    empty dictionary and prints appropriate error messages. The function handles
    both FileNotFoundError and JSONDecodeError exceptions gracefully.
    
    Returns:
        dict: Configuration dictionary, or empty dict if loading fails
    """
    config_filename = 'config.json'
    try:
        temp_config_path = os.path.join(os.path.dirname(__file__), config_filename)
        with open(temp_config_path, 'r') as f:
            temp_config = json.load(f)
            config_filename = temp_config.get('files', {}).get('config_filename', config_filename)
    except:
        pass
    config_path = os.path.join(os.path.dirname(__file__), config_filename)
    try:
        with open(config_path, 'r') as f:
            return json.load(f)
    except FileNotFoundError:
        error_msg = CONFIG.get('errors', {}).get('config_not_found', 'Warning: config.json not found at {path}, using defaults').format(path=config_path)
        print(error_msg)
        return {}
    except json.JSONDecodeError as e:
        error_msg = CONFIG.get('errors', {}).get('config_parse_error', 'Error parsing config.json: {error}, using defaults').format(error=e)
        print(error_msg)
        return {}

def postprocessing_eve(frame):
    if eve_instance:
        return eve_instance.send_image(frame)
    else:
        return frame

CONFIG = load_config()
USB_CAMERA_ID = CONFIG.get('cameras', {}).get('usb', {}).get('id', 0)
PI_CAMERA_ID = CONFIG.get('cameras', {}).get('pi', {}).get('id', 'pi')

try:
    hub_instance = hub.HUB("", "", eve_bin_path = CONFIG.get("eve", {}).get("path", {}))
    cam_instance = hub_instance.cam_instance
    cam_instance.set_postprocessing_method(postprocessing_eve)
    eve_instance = hub_instance.eve_instance

    try:
        usb_camera_config = CONFIG.get('cameras', {}).get('usb', {})
        cam_instance.setup_camera(
            USB_CAMERA_ID, 
            width=usb_camera_config.get('width', 1920), 
            height=usb_camera_config.get('height', 1080), 
            camera_type=usb_camera_config.get('type', 'usb')
        )
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('usb_camera_setup_failed', 'USB Camera setup failed: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        traceback.print_exc()

    try:
        pi_camera_config = CONFIG.get('cameras', {}).get('pi', {})
        cam_instance.setup_camera(
            PI_CAMERA_ID, 
            width=pi_camera_config.get('width', 3280), 
            height=pi_camera_config.get('height', 2464), 
            camera_type=pi_camera_config.get('type', 'picamera2')
        )
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('pi_camera_setup_failed', 'Pi Camera setup failed: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        traceback.print_exc()
except Exception as e:
    error_msg = CONFIG.get('errors', {}).get('hub_init_failed', 'HUB initialization failed: {error}').format(error=e)
    print(error_msg)
    traceback.print_exc()

shutdown_in_progress = False


def cleanup_resources():
    """
    Cleanup function to properly release all camera resources.
    Delegates to cam_instance.cleanup_resources().
    """
    global shutdown_in_progress
    
    shutdown_in_progress = True
    if cam_instance:
        cam_instance.cleanup_resources()


def signal_handler(signum, frame):
    """
    Handle termination signals (Ctrl+C, SIGTERM) for graceful shutdown.
    Prevents multiple signal handling to avoid loops.
    """
    global shutdown_in_progress
    
    if shutdown_in_progress:
        error_msg = CONFIG.get('errors', {}).get('force_exiting', 'Force exiting...')
        try:
            hub_instance.logger.warning(error_msg)
        except (NameError, AttributeError):
            print("\n" + error_msg)
        exit_code = CONFIG.get('constants', {}).get('exit_code_force', 1)
        os._exit(exit_code)
    
    signal_name = "SIGINT" if signum == signal.SIGINT else "SIGTERM"
    error_msg = CONFIG.get('errors', {}).get('received_signal', 'Received {signal}').format(signal=signal_name)
    try:
        hub_instance.logger.info(error_msg)
    except (NameError, AttributeError):
        print(f"\n{error_msg}")
    try:
        cam_instance.cleanup_resources()
    except Exception:
        pass
    cleanup_delay = CONFIG.get('delays', {}).get('cleanup_sleep', 0.5)
    time.sleep(cleanup_delay)
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
signal.signal(signal.SIGTERM, signal_handler)
atexit.register(cleanup_resources)


@app.route("/")
def index():
    """Serve the main HTML page of the web application."""
    return render_template("index.html")
 

@app.route("/api/config")
def get_config():
    """API endpoint to serve configuration to frontend."""
    frontend_config = {
        'cameras': CONFIG.get('cameras', {}),
        'paths': CONFIG.get('paths', {}),
        'image_ops': CONFIG.get('image_ops', {}),
        'api_endpoints': CONFIG.get('api_endpoints', {}),
        'local_storage_keys': CONFIG.get('local_storage_keys', {}),
        'messages': CONFIG.get('messages', {}),
        'errors': CONFIG.get('errors', {}),
        'status': CONFIG.get('status', {}),
        'delays': CONFIG.get('delays', {}),
        'plot_labels': CONFIG.get('plot_labels', {}),
        'plot_trace_names': CONFIG.get('plot_trace_names', {}),
        'plot_metric_keys': CONFIG.get('plot_metric_keys', {}),
        'data_window_size': CONFIG.get('data_window_size', 40)
    }
    return jsonify(frontend_config)


@app.route("/get_live_data")
def get_live_data():
    """
    API endpoint to fetch live sensor data (temperature, voltage, current, power).
    Returns JSON with sensor readings and timestamp.
    """
    sensor_config = CONFIG.get('sensors', {})
    temp_config = sensor_config.get('temperature', {})
    energy_config = sensor_config.get('energy', {})
    
    TEMP_SENSOR_1_ID = temp_config.get('sensor_1_id', 1)
    TEMP_SENSOR_2_ID = temp_config.get('sensor_2_id', 2)
    ENERGY_SENSOR_1_ID = energy_config.get('sensor_1_id', 1)
    ENERGY_SENSOR_2_ID = energy_config.get('sensor_2_id', 2)

    temp_data = hub_instance.get_temperature_data([TEMP_SENSOR_1_ID, TEMP_SENSOR_2_ID])
    energy_data = hub_instance.get_energy_data([ENERGY_SENSOR_1_ID, ENERGY_SENSOR_2_ID])

    command_to_run = sensor_config.get('system_temp_command', ['vcgencmd', 'measure_temp'])

    result = subprocess.run(
        command_to_run,
        capture_output=True,
        text=True,
    )

    res = result.stdout

    if temp_data[0]:
        t1 = round(temp_data[0], 2)
    else:
        t1 = temp_data[0]

    if temp_data[1]:
        t2 = round(temp_data[1], 2)
    else:
        t2 = temp_data[1]
    live_data = {
        "timestamp": time.time() * 1000,
        "temp1": t1,
        "temp2": t2,
        "temp3": float(res[5:-3]),
        "voltage1": energy_data[0]["voltage"],
        "current1": energy_data[0]["current"],
        "power1": energy_data[0]["power"],
        "voltage2": energy_data[1]["voltage"],
        "current2": energy_data[1]["current"],
        "power2": energy_data[1]["power"],
        'isFpgaEnabled': eve_instance.isFpgaEnabled() if eve_instance else False,
        'isDisplayingAIMetadata': eve_instance.isManualMetadataEnabled() if eve_instance else False,
    }

    return jsonify(live_data)

@app.route("/enable_metadata", methods=["POST"])
def enable_metadata():
    if eve_instance:
        enabled = request.form.get("enabled")
        eve_instance.enableManualImageMetadata(enabled)
    return {}
     
@app.route("/video_feed")
def video_feed():
    """Legacy route redirects to CLNX camera feed for backward compatibility."""
    return video_feed_clnx()


@app.route("/video_feed/cpnx")
def video_feed_cpnx():
    """Serve MJPEG video stream from CPNX camera (Pi Camera)."""
    http_codes = CONFIG.get('http_status_codes', {})
    mime_types = CONFIG.get('mime_types', {})
    no_content = http_codes.get('no_content', 204)
    ok = http_codes.get('ok', 200)
    
    if PI_CAMERA_ID not in cam_instance.available_cameras:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))
    running_status = CONFIG.get('status', {}).get('running', 'Running')
    if cam_instance.get_camera_status(PI_CAMERA_ID) == running_status:
        return Response(
            cam_instance.generate_frames(PI_CAMERA_ID),
            mimetype=mime_types.get('multipart_mixed', 'multipart/x-mixed-replace; boundary=frame'),
            status=ok,
        )
    else:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))


@app.route("/video_feed/clnx")
def video_feed_clnx():
    """Serve MJPEG video stream from CLNX camera (USB Camera)."""
    http_codes = CONFIG.get('http_status_codes', {})
    mime_types = CONFIG.get('mime_types', {})
    no_content = http_codes.get('no_content', 204)
    ok = http_codes.get('ok', 200)
    
    if USB_CAMERA_ID not in cam_instance.available_cameras:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))
    running_status = CONFIG.get('status', {}).get('running', 'Running')
    if cam_instance.get_camera_status(USB_CAMERA_ID) == running_status:
        return Response(
            cam_instance.generate_frames(USB_CAMERA_ID),
            mimetype=mime_types.get('multipart_mixed', 'multipart/x-mixed-replace; boundary=frame'),
            status=ok,
        )
    else:
        return Response("", status=no_content, mimetype=mime_types.get('text_plain', 'text/plain'))


@app.route("/video_feed/dual")
def video_feed_dual():
    """Return dual camera status and feed URLs. Dual display handled client-side."""
    cpnx_status = cam_instance.get_camera_status(PI_CAMERA_ID)
    clnx_status = cam_instance.get_camera_status(USB_CAMERA_ID)

    return jsonify(
        {
            "cpnx_status": cpnx_status,
            "clnx_status": clnx_status,
            "cpnx_feed": "/video_feed/cpnx",
            "clnx_feed": "/video_feed/clnx",
        }
    )


@app.route("/camera_control", methods=["POST"])
def camera_control():
    """
    Control camera start/stop operations.
    Supports single camera (clnx/cpnx) or dual camera mode.
    """
    action = request.form.get("action")
    camera_type = request.form.get("camera_type", "clnx")

    camera_id = PI_CAMERA_ID if camera_type == "cpnx" else USB_CAMERA_ID

    try:
        if action == "start":
            if camera_type == "dual":
                try:
                    cam_instance.start_camera(USB_CAMERA_ID)
                except Exception as e:
                    error_msg = CONFIG.get('errors', {}).get('failed_start_usb_camera', 'Failed to start USB Camera: {error}').format(error=e)
                    hub_instance.logger.error(error_msg)
                    traceback.print_exc()
                try:
                    cam_instance.start_camera(PI_CAMERA_ID)
                except Exception as e:
                    error_msg = CONFIG.get('errors', {}).get('failed_start_pi_camera', 'Failed to start Pi Camera: {error}').format(error=e)
                    hub_instance.logger.error(error_msg)
                    traceback.print_exc()
            else:
                cam_instance.start_camera(camera_id)
        elif action == "stop":
            if camera_type == "dual":
                try:
                    cam_instance.stop_camera(USB_CAMERA_ID)
                except Exception as e:
                    error_msg = CONFIG.get('errors', {}).get('failed_stop_usb_camera', 'Failed to stop USB Camera: {error}').format(error=e)
                    hub_instance.logger.error(error_msg)
                    traceback.print_exc()
                try:
                    cam_instance.stop_camera(PI_CAMERA_ID)
                except Exception as e:
                    error_msg = CONFIG.get('errors', {}).get('failed_stop_pi_camera', 'Failed to stop Pi Camera: {error}').format(error=e)
                    hub_instance.logger.error(error_msg)
                    traceback.print_exc()
            else:
                cam_instance.stop_camera(camera_id)

        status_success = CONFIG.get('status', {}).get('success', 'success')
        return jsonify(
            {"status": status_success, "camera_type": camera_type, "action": action}
        )
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_camera_control', 'Error in camera_control: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        traceback.print_exc()
        status_error = CONFIG.get('status', {}).get('error', 'error')
        return jsonify(
            {"status": status_error, "message": str(e), "camera_type": camera_type, "action": action}
        ), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)


@app.route("/camera_status/<camera_type>")
def camera_status(camera_type):
    """
    Get the current status of a camera or both cameras.

    Args:
        camera_type: 'cpnx', 'clnx', "No Camera Found", or 'dual'

    Returns:
        JSON with camera status information
    """
    if camera_type == "cpnx":
        status_config = CONFIG.get('status', {})
        no_camera = status_config.get('no_camera_found', 'No Camera Found')
        if PI_CAMERA_ID not in cam_instance.available_cameras:
            status = no_camera
        else:
            status = cam_instance.get_camera_status(PI_CAMERA_ID)
    elif camera_type == "clnx":
        status_config = CONFIG.get('status', {})
        no_camera = status_config.get('no_camera_found', 'No Camera Found')
        if USB_CAMERA_ID not in cam_instance.available_cameras:
            status = no_camera
        else:
            status = cam_instance.get_camera_status(USB_CAMERA_ID)
    elif camera_type == "dual":
        status_config = CONFIG.get('status', {})
        no_camera = status_config.get('no_camera_found', 'No Camera Found')
        status = {
            "cpnx": no_camera if PI_CAMERA_ID not in cam_instance.available_cameras else cam_instance.get_camera_status(PI_CAMERA_ID),
            "clnx": no_camera if USB_CAMERA_ID not in cam_instance.available_cameras else cam_instance.get_camera_status(USB_CAMERA_ID),
        }
    else:
        status = CONFIG.get('status', {}).get('unknown', 'Unknown')

    return jsonify({"camera_type": camera_type, "status": status})


@app.route("/api/capture_image_from_gard", methods=["POST"])
def capture_image_from_gard():
    """API endpoint to capture an image from GARD and return it in requested format."""
    try:
        gard_config = CONFIG.get('gard', {})
        gard_num = gard_config.get('default_gard_num', 0)
        camera_id = gard_config.get('default_camera_id', 0)
        
        status_error = CONFIG.get('status', {}).get('error', 'error')
        validation = CONFIG.get('validation', {})
        http_codes = CONFIG.get('http_status_codes', {})
        
        try:
            gard = hub_instance.get_gard(gard_num)
            if gard is None:
                return jsonify({
                    "status": status_error,
                    "message": validation.get('gard_not_found', 'GARD {gard_num} not found').format(gard_num=gard_num)
                }), http_codes.get('not_found', 404)
        except Exception as e:
            error_msg = CONFIG.get('errors', {}).get('error_getting_gard', 'Error getting GARD: {error}').format(error=str(e))
            return jsonify({
                "status": status_error,
                "message": error_msg
            }), http_codes.get('internal_server_error', 500)
        
        result = cam_instance.capture_image_from_gard(hub_instance, gard, camera_id)
        
        if result is None:
            return jsonify({
                "status": status_error,
                "message": validation.get('capture_failed', 'Failed to capture image from GARD')
            }), http_codes.get('internal_server_error', 500)
        
        image_info, image_data = result
        width = image_info['width']
        height = image_info['height']
        image_format = image_info['format']
        buffer_size = len(image_data)
        
        format_constants = CONFIG.get('image_format_constants', {})
        rgb_non_planar = format_constants.get('rgb_non_planar', 0)
        rgb_planar = format_constants.get('rgb_planar', 1)
        
        try:
            num_pixels = width * height
            expected_size_non_planar = num_pixels * 3
            expected_size_planar = num_pixels * 3
            expected_size_grayscale = num_pixels
            
            if buffer_size == expected_size_non_planar or buffer_size == expected_size_planar:
                if image_format == rgb_non_planar:
                    img_array = np.frombuffer(image_data, dtype=np.uint8).reshape((height, width, 3))
                    img_bgr = cv2.cvtColor(img_array, cv2.COLOR_RGB2BGR)
                elif image_format == rgb_planar:
                    R = np.frombuffer(image_data[0:num_pixels], dtype=np.uint8).reshape((height, width))
                    G = np.frombuffer(image_data[num_pixels:2*num_pixels], dtype=np.uint8).reshape((height, width))
                    B = np.frombuffer(image_data[2*num_pixels:3*num_pixels], dtype=np.uint8).reshape((height, width))
                    img_bgr = np.stack((B, G, R), axis=-1)
                else:
                    img_rgb = np.frombuffer(image_data, dtype=np.uint8)
                    R = img_rgb[0:num_pixels]
                    G = img_rgb[num_pixels:2*num_pixels]
                    B = img_rgb[2*num_pixels:3*num_pixels]
                    img_bgr = np.stack((B, G, R), axis=-1).reshape((height, width, 3))
            elif buffer_size == expected_size_grayscale:
                img_gray = np.frombuffer(image_data, dtype=np.uint8).reshape((height, width))
                img_bgr = cv2.cvtColor(img_gray, cv2.COLOR_GRAY2BGR)
            else:
                if buffer_size >= num_pixels * 3:
                    img_rgb = np.frombuffer(image_data, dtype=np.uint8)
                    R = img_rgb[0:num_pixels]
                    G = img_rgb[num_pixels:2*num_pixels]
                    B = img_rgb[2*num_pixels:3*num_pixels]
                    img_bgr = np.stack((B, G, R), axis=-1).reshape((height, width, 3))
                elif buffer_size >= num_pixels:
                    img_gray = np.frombuffer(image_data[0:num_pixels], dtype=np.uint8).reshape((height, width))
                    img_bgr = cv2.cvtColor(img_gray, cv2.COLOR_GRAY2BGR)
                else:
                    error_msg = CONFIG.get('errors', {}).get('buffer_size_mismatch', 'Buffer size {buffer_size} doesn\'t match expected dimensions {width}x{height} for format {image_format}').format(buffer_size=buffer_size, width=width, height=height, image_format=image_format)
                    raise ValueError(error_msg)
            
            requested_format = None
            if request.is_json and request.get_data():
                data = request.get_json(silent=True)
                if data:
                    requested_format = data.get('format', None)
            if not requested_format:
                default_format = CONFIG.get('image_ops', {}).get('default_format', 'bin')
                requested_format = request.args.get('format', default_format).lower()
            mime_types = CONFIG.get('mime_types', {})
            response_data = None
            response_mimetype = None
            
            mime_types = CONFIG.get('mime_types', {})
            errors = CONFIG.get('errors', {})
            image_ops = CONFIG.get('image_ops', {})
            
            if requested_format == 'bin':
                response_data = image_data
                response_mimetype = mime_types.get('application_octet_stream', 'application/octet-stream')
            elif requested_format == 'png':
                success, encoded_img = cv2.imencode('.png', img_bgr)
                if not success:
                    error_msg = errors.get('failed_encode_image', 'Failed to encode image to {image_format}').format(image_format='PNG')
                    raise ValueError(error_msg)
                response_data = encoded_img.tobytes()
                response_mimetype = mime_types.get('image_png', 'image/png')
            elif requested_format == 'bmp':
                success, encoded_img = cv2.imencode('.bmp', img_bgr)
                if not success:
                    error_msg = errors.get('failed_encode_image', 'Failed to encode image to {image_format}').format(image_format='BMP')
                    raise ValueError(error_msg)
                response_data = encoded_img.tobytes()
                response_mimetype = mime_types.get('image_bmp', 'image/bmp')
            else:
                jpeg_quality = image_ops.get('jpeg_quality', 95)
                success, encoded_img = cv2.imencode('.jpg', img_bgr, [cv2.IMWRITE_JPEG_QUALITY, jpeg_quality])
                if not success:
                    error_msg = errors.get('failed_encode_image', 'Failed to encode image to {image_format}').format(image_format='JPEG')
                    raise ValueError(error_msg)
                response_data = encoded_img.tobytes()
                response_mimetype = mime_types.get('image_jpeg', 'image/jpeg')
            
            return Response(
                response_data,
                mimetype=response_mimetype,
                headers={
                    'Content-Disposition': 'inline',
                    'X-Image-Width': str(width),
                    'X-Image-Height': str(height),
                    'X-Image-Format': str(image_format),
                    'X-Response-Format': requested_format
                }
            )
        except Exception as e:
            error_msg = CONFIG.get('errors', {}).get('error_processing_image', 'Error processing image: {error}').format(error=str(e))
            hub_instance.logger.error(error_msg)
            traceback.print_exc()
            return jsonify({
                "status": status_error,
                "message": error_msg
            }), http_codes.get('internal_server_error', 500)
            
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_capture_gard_image', 'Error in capture_image_from_gard endpoint: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        traceback.print_exc()
        status_error = CONFIG.get('status', {}).get('error', 'error')
        return jsonify({
            "status": status_error,
            "message": str(e)
        }), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)
@app.route("/api/save_image", methods=["POST"])
def save_image():
    """API endpoint to save an image on the server. Accepts image data, save path, and classification."""
    try:
        data = request.get_json()
        save_on_server = data.get("save_on_server", False)
        
        status_error = CONFIG.get('status', {}).get('error', 'error')
        validation = CONFIG.get('validation', {})
        if not save_on_server:
            return jsonify({
                "status": status_error,
                "message": validation.get('save_on_server_false', 'save_on_server flag is False. Use client-side download instead.')
            }), CONFIG.get('http_status_codes', {}).get('bad_request', 400)
        
        save_path = data.get("save_path", "").strip()
        constants = CONFIG.get('constants', {})
        classification = data.get("classification", constants.get('classification_normal', 'normal'))
        image_data = data.get("image_data")  # Base64 encoded image data
        requested_format = data.get("image_format", "").lower().strip()  # Get format from request
        
        hub_instance.logger.debug(f"Save image request - path: {save_path}, has_image_data: {bool(image_data)}, classification: {classification}, format: {requested_format}")

        if not save_path:
            save_path = CONFIG.get('paths', {}).get('default_server_save_path', '/home/lattice/Downloads/edgeHUB/')
            os.makedirs(save_path, exist_ok=True)
        else:
            save_path = os.path.expanduser(save_path)
            if not save_path.startswith('/'):
                return jsonify({
                    "status": status_error,
                    "message": validation.get('absolute_path_required', 'Please provide an absolute path (starting with / or ~)')
                }), CONFIG.get('http_status_codes', {}).get('bad_request', 400)

        image_ops_config = CONFIG.get('image_ops', {})
        valid_formats = image_ops_config.get('valid_formats', ['png', 'jpg', 'jpeg', 'bmp', 'bin'])
        if requested_format and requested_format in valid_formats:
            image_format = requested_format
        else:
            image_format = image_ops_config.get('image_format', 'png')
        
        filename_prefix = image_ops_config.get('filename_prefix', 'edgehub_imageops')
        timestamp_format = image_ops_config.get('timestamp_format', '%Y%m%d_%H%M%S')
        timestamp = datetime.now().strftime(timestamp_format)
        filename = f"{filename_prefix}_{classification}_{timestamp}.{image_format}"
        
        if save_path.endswith('/') or os.path.isdir(save_path):
            final_save_path = os.path.join(save_path.rstrip('/'), filename)
            save_dir = save_path.rstrip('/')
        else:
            save_dir = os.path.dirname(save_path)
            final_save_path = os.path.join(save_dir, filename) if save_dir else filename

        if save_dir and not os.path.exists(save_dir):
            try:
                os.makedirs(save_dir, exist_ok=True)
            except Exception as e:
                return jsonify({
                    "status": status_error,
                    "message": validation.get('cannot_create_directory', 'Cannot create directory: {error}').format(error=str(e))
                }), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)

        try:
            img = None
            
            if image_data:
                try:
                    image_bytes = base64.b64decode(image_data)
                    
                    if image_format.lower() == 'bin':
                        with open(final_save_path, 'wb') as f:
                            f.write(image_bytes)
                        img = None
                    else:
                        nparr = np.frombuffer(image_bytes, np.uint8)
                        img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                        if img is None:
                            error_msg = CONFIG.get('errors', {}).get('failed_decode_base64_data', 'Failed to decode image from base64 data')
                            raise ValueError(error_msg)
                except Exception as e:
                    return jsonify({
                        "status": status_error,
                        "message": validation.get('failed_decode_base64', 'Failed to decode base64 image data: {error}').format(error=str(e))
                    }), CONFIG.get('http_status_codes', {}).get('bad_request', 400)
            else:
                return jsonify({
                    "status": status_error,
                    "message": validation.get('no_image_data', 'No image data or path provided')
                }), CONFIG.get('http_status_codes', {}).get('bad_request', 400)
            
            if img is not None:
                jpeg_quality = image_ops_config.get('jpeg_quality', 95)
                png_compression = image_ops_config.get('png_compression', 3)
                
                if image_format.lower() == 'jpg' or image_format.lower() == 'jpeg':
                    cv2.imwrite(final_save_path, img, [cv2.IMWRITE_JPEG_QUALITY, jpeg_quality])
                elif image_format.lower() == 'png':
                    cv2.imwrite(final_save_path, img, [cv2.IMWRITE_PNG_COMPRESSION, png_compression])
                elif image_format.lower() == 'bmp':
                    cv2.imwrite(final_save_path, img)
                else:
                    if not final_save_path.lower().endswith('.png'):
                        final_save_path = os.path.splitext(final_save_path)[0] + '.png'
                    cv2.imwrite(final_save_path, img, [cv2.IMWRITE_PNG_COMPRESSION, png_compression])
        except Exception as e:
            return jsonify({
                "status": status_error,
                "message": validation.get('failed_save_image', 'Failed to save image: {error}').format(error=str(e))
            }), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)

        files_config = CONFIG.get('files', {})
        metadata_extension = files_config.get('metadata_extension', '.metadata.json')
        metadata_path = final_save_path + metadata_extension
        try:
            metadata = {
                "classification": classification,
                "saved_at": datetime.now().isoformat(),
                "image_path": final_save_path,
                "original_path": save_path
            }
            with open(metadata_path, 'w') as f:
                json.dump(metadata, f, indent=2)
        except Exception as e:
            error_msg = CONFIG.get('errors', {}).get('warning_metadata', 'Warning: Could not save metadata: {error}').format(error=e)
            hub_instance.logger.warning(error_msg)

        status_success = CONFIG.get('status', {}).get('success', 'success')
        messages = CONFIG.get('messages', {})
        saved_msg = messages.get('image_saved_success', 'Image saved successfully to {path} with status: {classification}').format(path=final_save_path, classification=classification)
        hub_instance.logger.info(saved_msg)
        
        saved_to_msg = messages.get('image_saved_to', 'Image saved to {path}').format(path=final_save_path)
        return jsonify({
            "status": status_success,
            "message": saved_to_msg,
            "saved_path": final_save_path,
            "filename": filename,
            "classification": classification
        })
    except Exception as e:
        error_msg = CONFIG.get('errors', {}).get('error_save_image', 'Error in save_image: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        traceback.print_exc()
        return jsonify({
            "status": status_error,
            "message": str(e)
        }), CONFIG.get('http_status_codes', {}).get('internal_server_error', 500)

@app.route('/json')
def get_json():
    return eve_instance.get_json()    
                
if __name__ == "__main__":
    try:
        server_config = CONFIG.get('server', {})
        host = server_config.get('host', '0.0.0.0')
        port = server_config.get('port', 5000)
        
        server_messages = CONFIG.get('server_messages', {})
        hub_instance.logger.info(server_messages.get('starting_app', 'Starting edgeHUB application...'))
        hub_instance.logger.info(server_messages.get('server_available', 'Server available at: http://{host}:{port}').format(host=host, port=port))
        hub_instance.logger.info(server_messages.get('press_ctrl_c', 'Press Ctrl+C to stop'))

        app.run(
            host=host,
            port=port,
            debug=server_config.get('debug', True),
            threaded=server_config.get('threaded', True),
            use_reloader=server_config.get('use_reloader', False)
        )

    except KeyboardInterrupt:
        server_messages = CONFIG.get('server_messages', {})
        hub_instance.logger.info(server_messages.get('keyboard_interrupt', 'Keyboard interrupt received'))
        cleanup_resources()

    except Exception as e:
        server_messages = CONFIG.get('server_messages', {})
        error_msg = server_messages.get('error_occurred', 'Error: {error}').format(error=e)
        hub_instance.logger.error(error_msg)
        cleanup_resources()

    finally:
        cleanup_resources()
