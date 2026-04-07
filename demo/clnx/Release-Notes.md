# CrossLinkU-NX SoM - HMI Demo Release Notes
**Version:** 5.1

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
