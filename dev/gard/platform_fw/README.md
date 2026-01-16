# GARD
GARD stands for **Golden AI Reference Design**. It is an FPGA RTL design running on a Lattice FPGA, that instantiates various sub-systems such as:
1. RISC-V subsystem IP
2. ML engine IP (could include ISP / mini-ISP)
3. Host Interface IP
4. User Application IP [optional, customer-defined]


## GARD Firmware
The RISC-V subsystem on the GARD runs the GARD FW (firmware).

Its functions include, but are not limited to:
1. RISC-V CPU bootup
2. Interaction with the Host over serial interfaces (I2C, UART, etc.)
3. ML engine IP control
4. Sensor (camera, etc.) control and coordination of ML IP (and constituents)
5. Selective pre- and/or post- processing of sensor data


### Folder structure
Firmware files are spread across folders for modular identification as described:

#### src/gard_firmware/fw
This folder contains the files that constitute the fw-core module.

#### src/gard_firmware/fw_ldr
This folder contains the files that constitute the firmware loader.

#### src/gard_firmware/bsp
This folder contains the BSP files. These are provided along with the Soft IPs of the peripherals but could have been modified by the fw-core developers to suit their needs.
The files here could be used by both Firmware and Firmware Loader, depending on their requirements.

#### src/gard_firmware/common
This folder contains files which are developed by the GARD firmware developers. They are placed here so that they can also be used by both Firmware and Firmware Loader code.

#### src/gard_firmware/inc
This folder contains header files that describe the prototypes of the data and code. The source files containing the code and the data is spread across the folders described above.

#### src/interface
This folder contains the header files that describe the interface between HUB and GARD.

#### src/scripts
This folder contains scripts that are used to build in addition to other things - Camera Configurations and Firmware Image.

#### build
This folder contains the Makefile needed to build the firmware and firmware loader.

#### build/output
This folder contains the generated artifacts as a part of the build process. This folder is not
seen when downloaded from the source control system. This folder is generated as a part of the build process.


### Building GARD Firmware
#### Prerequisites
- Propel 2025.* installed on development/build system.
- Command shell with the path to the installed Propel tools such as C Compiler/Linker and Makefile

#### Building Firmware
1) Enter the folder build/
2) Run the command 'make'
3) The firmware and firmware-loader binaries will be generated in the folders *build/output/gard_firmware/fw/* and *build/output/gard_firmware/fw_ldr*

#### Generating fwroot.bin
1) Generate Camera configuration binary using the tool scripts/CameraConfigurationBuilder.py.
2) Generate fwroot.bin by running the tool scripts/RootImageBuilder.py.

### Flashing GARD Firmware
Once *fwroot.bin* has been generated flash this binary onto the SPI Flash of the hardware using the Radiant Programmer Tool.
This binary needs to be flashed at address 0x00300000.

### Embedding Firmware Loader within Bitstream
The Bitstream needs to be embedded with the firmware loader file *build/output/gard_firmware/fw_ldr/gard_fw_ldr.mem*

### Flashing Bitstream
The Bitstream embedded with the firmware loader should be flashed on the SPI Flash at address 0x0

## Testing GARD Firmware
For basic testing of the running firmware Host needs to use UART or I2C to send commands to the GARD firmware and receive expected responses. Currently there are no such scripts provided and instead HUB provided scripts can be run for the same.