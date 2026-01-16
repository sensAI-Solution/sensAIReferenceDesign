# Multiple Object Detection Reference Design on CPNX
This page is the starting point for anybody intending to setup the CPNX MOD demo.

## Introduction
This demo shows the capabilities of a multi-object detection network by running the network on FPGA, receiving the results on a Raspberry Pi and displaying them overlayed on the live image from the camera.

## Required Material
The following components are required to run the MOD demo on the CertusPro-NX SOM:

### Hardware
- CertusPro-NX SOM Board:
    - OPN: LFCPNX-SOM-EVN
    - CertusPro-NX Package: LFCPNX-100-9ASG256I
    - MachXO3D™ Package: LCMXO3D-9400
- CertusPro-NX Carrier Module (OPN: LF-GEN-CR-PCBA)
- Raspberry Pi Compute Module 5 (CM5)
- Mini-USB Type-A UART Cable:
    - Included in the CertusPro-NX SOM board kit.
    - Used for programming the bitstream, firmware, and ensuring proper terminal prints.
- IMX219 Camera Module: Raspberry Pi Camera Module 2/Arducam 8 MP IMX219 Camera with four-lane support.
- Power adapter for board power: 5 V 25 W AC/DC external wall mount (class II) adapter multi-blade (sold separately) input.

### Software
- [Lattice Radiant™ Programmer](https://www.latticesemi.com/Products/DesignSoftwareAndIP/FPGAandLDS/Radiant)  (version 2025.1)
- [Raspberry Pi Software](https://www.raspberrypi.com/software/)
- [rpiboot](https://github.com/raspberrypi/usbboot/raw/master/win32/rpiboot_setup.exe)

## Demo Setup
Everything for a demonstration is ready to use after flashing the CM5 with the image in [RPI_OS](RPI_OS/) and programming the [bitstream](BITSTREAM/) at 0x00000000 as well as the [root image](ROOT_IMAGE/) at 0x00300000­. More details are available in the [MOD Demo User Guide](docs/MOD_User_Guide.pdf).


## Build from Reference Design
[MOD Reference Design Instructions](docs/MOD_Reference_Design.pdf)