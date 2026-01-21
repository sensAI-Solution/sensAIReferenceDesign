# Human Machine Interface Demo on CLNX

>[!WARNING]
>This repository uses Git LFS for artifact distribution and is not intended for active collaboration. Please follow the steps in the [official Github documentation for Git LFS](https://docs.github.com/en/repositories/working-with-files/managing-large-files/installing-git-large-file-storage) before cloning.
>If this step is skipped, certain binaries will not download properly, which may be difficult to diagnose in some cases.

## Introduction
The [CrossLinkU™-NX System-on-Module (SoM) HMI Demonstration Design](https://www.latticesemi.com/products/designsoftwareandip/intellectualproperty/demos/human-to-machine-interfacing-demonstration) is a demo solution for Human-Machine Interaction (HMI) applications. It enables AI-driven interaction capabilities such as person detection, face recognition, and face identification, delivering intuitive and intelligent user experiences at the edge.
This design is optimized for low-power embedded vision systems, leveraging Lattice’s FPGA technology and AI pipeline for real-time inferencing. It integrates with Raspberry Pi CM5 for visualization and control through a web-based dashboard.

### Key Features
- Edge AI processing for HMI applications.
- Real-time AI visualization via Lattice EVE SDK and web server.
- Ultra-low power operation using Always-On (AON) controller.
- Secure remote firmware updates with ECDSA authentication.

### Operational Modes

| Mode        | Description                               |
| ----------- | ----------------------------------------- |
| Streaming   | Continuous video + AI inference.          |
| Sensing     | AI detection only (no video passthrough). |
| ULP Sensing | Ultra-Low Power mode.                     |

## Prerequisites
Refer to [CrossLinkU-NX SoM Demo Kit](https://www.latticesemi.com/products/developmentboardsandkits/crosslinku-nx-system-on-module-som-board) (OPN: LIFCL-SOM-EVN) for more information.

### Setup
This HMI demo comes with a [user guide](docs/FPGA-RD-02333-1-0-CrossLinkU-NX-SoM-for-HMI-Demonstration.pdf), including details of setup, configuration information and troubleshooting methodology.

## Folder Structure
```
CLNXSOM_HMI_PACKAGE
├── CLNX_SPI_IMAGE
│ └── clnxsomdemo_64MB_01_12_00_00.bin
│
├── EVE_WEBSERVER
│ └── EVEWebAppHMI_v0.1.29_CLNX.zip
│
├── FW_UPDATE_TOOL
│ └── clnxsomdemo_fwupd_01.12.00.00
│
├── RESOURCES
│ └── config.txt
│ └── RPI_SCRIPTS
│  ├── post_programmer_operation.sh
│  ├── pre_programmer_operation.sh
│  └── run_eve_web_server.sh
│
├── RPI_DRIVERS
│ ├── lsc219.dtbo
│ └── lscc-imx219.ko
│
└── RPI_OS
  └── RPi_OS-arm64_1.0.0_CLNX_HMI_v5.img.xz
```

## Folder Content
| Folder                     | Content                                            | Linux Directory                 |
| -------------------------- | -------------------------------------------------- | ------------------------------- |
| RPI_OS                     | Raspberry Pi OS image.<sup>2</sup>                 | -                               |
| CLNX_SPI_IMAGE             | 64 MB CrossLinkU-NX Som SPI Flash Binary.          | /opt/clnx_som/spi_flash_image   |
| FW_UPDATE_TOOL<sup>1</sup> | CrossLinkU-NX SoM Firmware Management application. | /opt/clnx_som/FW_Update         |
| EVE_WEBSERVER<sup>1</sup>  | EVE and Webserver installer.                       | /opt/edgeHub                    |
| RESOURCES<sup>1</sup>      | RPi boot configuration file.                       | /opt/clnx_som/resources         |
| RPI_SCRIPTS<sup>1</sup>    | Demo and programmer scripts.                       | /opt/clnx_som/resources/scripts |
| RPI_DRIVERS<sup>1</sup>    | Lattice drivers and overlays for the demo.         | /opt/clnx_som/drivers           |

Notes:
1. Preinstalled into the RPi OS image.
2. Lattice Custom RPi OS for CrossLinkU-NX SoM HMI Demo. OS update is disabled by default.
