# Release of CrossLinkU-NX SoM - HMI Demo v6
2026-02-03

The HMI FPGA AI Firmware is an AI workload that enables 3D Head Pose, Person Detection, Face ID and Hand Landmarks on a CrossLinkU-NX SoM Demo Kit.

## Hardware Platform: 
Lattice CrossLinkU-NX SoM Demo Kit

## Package contents:
| Component           | Version       |
|---------------------|--------------|
| FPGA AI Firmware    | v6.57d1410  |
| RPI5 image          | 1.0.0        |
| EVE Version         | 6.7.10        |
| Web Server Version  | V6 version 6.7|

## Changes:
EVE-874 FIXED: Web App Options are checked or unchecked on their own
EVE-908 FIXED: There is no way to switch between Streaming and Sensing mode once one is selected
FG-999	Add Hand Landmarks to HMI pipeline
FG-1080	Add requirement for Face ID <200cm in depth
FG-1151	Allow toggling between NFOV and WFOV camera at runtime
FG-1155	FIXED: HMI pipeline freezes/crashes when 10 users or more are in the FOV
FG-1171	Integrate LSQ Face Detection network in HMI pipeline
FG-1044	Update landmarks validation network to version 1.1
FG-1111	FIXED: After "Register user" while outside required conditions, user will be automatically registered once inside required conditions
FG-1112	FIXED Pressing "Register new Face ID" when FaceID is disabled, still takes affect after you enable FaceID
FG-1161	FIXED: IPS value for Hand Landmarks in EVE Web App reverts to zero when user attempts to change it
FG-1198	FIXED: EdgeHUB does not display accurate FPS output of HMI when features IPS are set to anything besides MAX.

## Known Issues:
FG-1199	EdgeHUB does not always display the camera output correctly when EdgeHUB is launched
  Workaround: Refresh web browser
FG-1191	Significant FPS drops can be observed if Hand Landmarks are enabled 
FG-1162	Certain hand gestures have inaccurate landmarks when the hand is near image border
FG-1154 Face and Person detections drop occasionally for one frame
FG-1152 Ideal User selection is not stable when multiple users are standing close to each other
FG-1147 ace ID identification does not work when registering at 4m
  Workaround: Make sure you register users below 2m
EVE-923 After ULP mode recovers from sleep state, the enabled features are different than what the user selected
FG-1114 Items reflecting bright light can appear purple
FG-1020 Once the gallery is full (10 entries), registrations start replacing old entries randomly
  Workaround: Clear the gallery
FG-960	Reported pose always returns "Non-Frontal" when user's head/face is out of the FOV or not tracked
FG-832	All users can lose detection simultaneously for one frame while detected in HMI pipeline.
FG-829	Moving around too much over time causes Face ID to fail recognition