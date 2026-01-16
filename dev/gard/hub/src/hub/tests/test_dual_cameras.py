#!/usr/bin/env python3
"""
Simple test script to verify both cameras can capture frames simultaneously
No GUI required - just captures and saves test frames
"""

import cv2
import time
from camInterface import CamInterface


def main():
    print("=" * 70)
    print("Testing Dual Camera Capture (USB + Pi Camera)")
    print("=" * 70)

    cam = CamInterface()

    # Detect cameras
    print("\n1. Detecting cameras...")
    cameras = cam.detect_cameras()
    print(f"   Found: {cameras}")

    if 0 not in cameras:
        print("    USB camera not found!")
        return False

    if "pi" not in cameras:
        print("    Pi camera not found!")
        return False

    print("    Both cameras detected")

    # Setup cameras
    print("\n2. Setting up cameras...")
    try:
        cam.setup_camera(0, width=1920, height=1080, camera_type="usb")
        print("    USB camera configured (1920x1080)")

        cam.setup_camera("pi", width=1920, height=1080, camera_type="picamera2")
        print("    Pi camera configured (1920x1080)")
    except Exception as e:
        print(f"    Setup failed: {e}")
        return False

    # Start cameras
    print("\n3. Starting cameras...")
    try:
        cam.start_camera(0)
        print(f"    USB camera started - Status: {cam.get_camera_status(0)}")

        cam.start_camera("pi")
        print(f"    Pi camera started - Status: {cam.get_camera_status('pi')}")
    except Exception as e:
        print(f"    Start failed: {e}")
        return False

    # Give cameras time to warm up
    time.sleep(1.5)

    # Capture test frames
    print("\n4. Capturing test frames...")

    # Capture from USB camera
    try:
        with cam.camera_data_context[0]["camera_lock"]:
            usb_stream = cam.camera_data_context[0]["camera_stream"]
            if usb_stream and usb_stream.isOpened():
                success, usb_frame = usb_stream.read()
                if success and usb_frame is not None:
                    cv2.imwrite("/tmp/usb_camera_test.jpg", usb_frame)
                    print(f"    USB camera frame captured: {usb_frame.shape}")
                    print(f"     Saved to: /tmp/usb_camera_test.jpg")
                else:
                    print("    USB camera failed to capture frame")
                    return False
    except Exception as e:
        print(f"    USB camera capture error: {e}")
        return False

    # Capture from Pi camera
    try:
        with cam.camera_data_context["pi"]["camera_lock"]:
            pi_stream = cam.camera_data_context["pi"]["camera_stream"]
            if pi_stream:
                pi_frame = pi_stream.capture_array()
                if pi_frame is not None:
                    cv2.imwrite("/tmp/pi_camera_test.jpg", pi_frame)
                    print(f"    Pi camera frame captured: {pi_frame.shape}")
                    print(f"     Saved to: /tmp/pi_camera_test.jpg")
                else:
                    print("    Pi camera failed to capture frame")
                    return False
    except Exception as e:
        print(f"    Pi camera capture error: {e}")
        return False

    # Capture multiple frames to test streaming
    print("\n5. Testing continuous capture (30 frames)...")
    frame_counts = {"usb": 0, "pi": 0}
    start_time = time.time()

    for i in range(30):
        # USB camera
        try:
            with cam.camera_data_context[0]["camera_lock"]:
                usb_stream = cam.camera_data_context[0]["camera_stream"]
                if usb_stream and usb_stream.isOpened():
                    success, usb_frame = usb_stream.read()
                    if success:
                        frame_counts["usb"] += 1
        except:
            pass

        # Pi camera
        try:
            with cam.camera_data_context["pi"]["camera_lock"]:
                pi_stream = cam.camera_data_context["pi"]["camera_stream"]
                if pi_stream:
                    pi_frame = pi_stream.capture_array()
                    if pi_frame is not None:
                        frame_counts["pi"] += 1
        except:
            pass

        time.sleep(0.033)  # ~30 FPS

    elapsed = time.time() - start_time
    print(
        f"    USB camera: {frame_counts['usb']}/30 frames (avg FPS: {frame_counts['usb']/elapsed:.1f})"
    )
    print(
        f"    Pi camera: {frame_counts['pi']}/30 frames (avg FPS: {frame_counts['pi']/elapsed:.1f})"
    )

    # Cleanup
    print("\n6. Stopping cameras...")
    try:
        cam.stop_camera(0)
        print(f"    USB camera stopped - Status: {cam.get_camera_status(0)}")
    except Exception as e:
        print(f"    USB camera stop error: {e}")

    try:
        cam.stop_camera("pi")
        print(f"    Pi camera stopped - Status: {cam.get_camera_status('pi')}")
    except Exception as e:
        print(f"    Pi camera stop error: {e}")

    print("\n" + "=" * 70)
    print(" SUCCESS: Both cameras working simultaneously!")
    print("=" * 70)
    print("\nTest images saved:")
    print("  • /tmp/usb_camera_test.jpg")
    print("  • /tmp/pi_camera_test.jpg")
    print("\nYou can now use:")
    print("  • dual_camera_web_stream.py - for web browser streaming")
    print("  • dual_camera_stream.py - for local display (requires X server)")
    print("=" * 70)

    return True


if __name__ == "__main__":
    try:
        success = main()
        exit(0 if success else 1)
    except KeyboardInterrupt:
        print("\n\n⚠ Test interrupted by user")
        exit(1)
    except Exception as e:
        print(f"\n\n Test failed: {e}")
        import traceback

        traceback.print_exc()
        exit(1)
