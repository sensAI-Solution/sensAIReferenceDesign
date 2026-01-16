# Host-accelerated Unified Bridge (H.U.B.)  
Development repository for the HUB and GARD FW.

**IMPORTANT NOTE:**
- The latest stable release of HUB code is in the **"main"** branch.
- The latest stable release of GARD FW code is in the **"fw_core-main"** branch.

## HUB
HUB stands for **Host-accelerated Unified Bridge** - a software framework for connecting to and controlling the GARD and GARD FW.

The aim of HUB is to function as an active conduit for control and data transfers between:
- Host Applications (running on RPi Linux), and
- Edge AI designs instantiated in the GARD,

thus enabling a gamut of applications seeking to accelerate AI/ML operations on Lattice FPGA's.

## GARD
GARD stands for **Golden AI Reference Design**. It is an FPGA RTL design running on a Lattice FPGA, that instantiates various sub-systems such as:
1. RISC-V subsystem IP
2. ML engine IP (could include ISP / mini-ISP)
3. Host Interface IP
4. User Application IP [optional, customer-defined]

The aim being to enable Lattice FPGA buyers get a go-to platform for setting up and evaluating a vareity of ML models and use-case offerings from Lattice with minimal setup and developmental effort.

## GARD FW
The RISC-V subsystem on the GARD runs the GARD FW (firmware).

Its functions include, but are not limited to:
1. RISC-V CPU bootup
2. Interaction with the Host over serial interfaces (I2C, UART, etc.)
3. ML engine IP control
4. Sensor (camera, etc.) control and coordination of ML IP (and constituents)
5. Selective pre- and/or post- processing of sensor data
