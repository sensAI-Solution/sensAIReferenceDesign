# Release of CrossLinkU-NX SoM - HMI Demo
2025-12-18

The HMI FPGA AI Firmware is an AI workload that enables 3D Head Pose, Person Detection and Face ID on a CrossLinkU-NX SoM Demo Kit.

## Hardware Platform: 
Lattice CrossLinkU-NX SoM Demo Kit

## Package contents:
| Component           | Version       |
|---------------------|--------------|
| FPGA AI Firmware    | 01.12.00.00  |
| RPI5 image          | 1.0.0        |
| EVE Version         | 6.7.5        |
| Web Server Version  | V5 version 6.7|

## Changes:
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

## Known Issues:
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