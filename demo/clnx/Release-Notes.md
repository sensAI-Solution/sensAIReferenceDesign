# CrossLinkU-NX SoM - HMI Demo Release Notes

## Index

- [Version 5.1](#version-51) - DHCP and Real-time sensor telemetry
- [Version 5.0](#version-5) - 3D Pose, Person Detection and Face ID Detection


## Version 5.1

**Date:** 8 April 2025

This release simplifies networking by running a DHCP server on the host so Ethernet comes up without manual IP setup, and it adds real-time visualization of sensor telemetry driven by actual connected power and temperature sensors.

### Hardware Platform
Lattice CrossLinkU-NX SoM Demo Kit

### Artifacts Details

| Component           | Version      |
|---------------------|--------------|
| FPGA AI Firmware    | 01.12.00.00  |
| RPi CM5 image       | 2.0.0        |
| EVE Version         | 6.7.5        |
| Web Server Version  | V5.1, 6.7    |

### Change Log

- EAI-273 Support for real time sensor readings
- EAI-336 Create OS Image with DHCP enabled
- EAI-416 v5.1: Performance & UI/UX changes

### Known Issues

- FG-1155 Firmware freezes when there are 10+ users in the scene
    - Workaround: Power cycle the SOM when the issue occurs
- FG-1154 Face and Person detections drop occasionally for one frame
- FG-1152 Ideal User selection is not stable when multiple users are standing close to each other
- EVE-908 There is no way to switch between Streaming and Sensing mode once one is selected
    - Workaround: Exit the web server with Ctrl+C and restart
- FG-1147 Face ID identification does not work when registering at 4m
    - Workaround: Make sure you register users below 2m
- FG-1020 Once the gallery is full (10 entries), registrations start replacing old entries randomly
    - Workaround: Clear the gallery
- EVE-923 After ULP mode recovers from sleep state, the enabled features are different than what the user selected
- FG-1114 Items reflecting bright light can appear purple
- FG-960 Reported pose always returns "Non-Frontal" when user's head/face is out of the FOV or not tracked
- FG-829 Moving around too much over time causes Face ID to fail recognition

## Version 5

**Date:** 18 December 2025

This release targets the CrossLinkU-NX SoM Demo Kit with FPGA-based AI that supports 3D Head Pose Estimation, Person Detection, and Face ID in the HMI feature set.

### Hardware Platform
Lattice CrossLinkU-NX SoM Demo Kit

### Artifacts Details

| Component           | Version      |
|---------------------|--------------|
| FPGA AI Firmware    | 01.12.00.00  |
| RPi CM5 image       | 1.0.0        |
| EVE Version         | 6.7.5        |
| Web Server Version  | V5, 6.7      |

### Change Log

- FG-1049	Integrate and calibrate the WFOV camera on the SOM
- FG-1064	FIXED: FaceID fails to recognize registered user when user stands at different distance than the distance they registered at.
- FG-1074	FIXED: FaceID fails to recognize the user within face threshold angle when they are beyond 170cm, even if they are registered at same distance
- FG-1113 Added auto-exposure in streaming mode
- FG-1115	Updated Person Detection with version 2.1
- FG-1066 Added Ultra-Low Power Mode
- EVE-820 Control Ultra-Low Power Mode through a GPIO pin
- EVE-920 FIXED: Display window is not correctly scaled when in Sensing mode
- EVE-918 Updated Web App title to Human-Machine Interface CLNX v5 Version 6.7
- EVE-901 FIXED: Crash when toggling features in the Web App
- EVE-877 FIXED: Ideal user has a Secondary user red Body Box
- EVE-869 Added FPS (metadata throughput) in the Web App
- EVE-898 Improved text quality of the metadata drawn on the image
- EVE-870 FIXED: Missing "Face ID for all users" checkbox
- EVE-874 FIXED: Options are checked and unchecked on their own
- EVE-873 Removed unecessary "Apply All" button as all controls are updated in real time
- EVE-871 FIXED: Refreshing the Web App window resets the user's settings

### Known Issues

- FG-1155 Firmware freezes when there are 10+ users in the scene
    - Workaround: Power cycle the SOM when the issue occurs
- FG-1154 Face and Person detections drop occasionally for one frame
- FG-1152 Ideal User selection is not stable when multiple users are standing close to each other
- EVE-908 There is no way to switch between Streaming and Sensing mode once one is selected
    - Workaround: Exit the web server with Ctrl+C and restart
- FG-1147 Face ID identification does not work when registering at 4m
    - Workaround: Make sure you register users below 2m
- FG-1020 Once the gallery is full (10 entries), registrations start replacing old entries randomly
    - Workaround: Clear the gallery
- EVE-923 After ULP mode recovers from sleep state, the enabled features are different than what the user selected
- FG-1114 Items reflecting bright light can appear purple
- FG-960 Reported pose always returns "Non-Frontal" when user's head/face is out of the FOV or not tracked
- FG-829 Moving around too much over time causes Face ID to fail recognition
