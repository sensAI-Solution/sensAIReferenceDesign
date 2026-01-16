
## Features

 - This reference design application demonstrates ML Based AI inference
   on live feed from IMX219 camera at native resolution of 3280x2464 at 30 FPS
 - The default design is compiled for 4 MIPI TX/RX lanes with 720 Mbps
   bandwidth/lane.
 - The design is compiled for Lattice CPNX100 SOM board (LFCPNX-SOM-EVN)
 - Features Lattice ADV CNN accelator IP version 3.0.0 configured in 32b
   data path mode, 4 LRAMs and 2 VE SPD packs.
 - The design provide RISCV subsystem to control AI inference and
   communication with external host like Raspberry PI CM5
 - ML IP Fmax is 120 MHz and RISCV IP Fmax is 108 MHz
 - Uses dual external Hyper RAMs running at 135 MHz and I2C,UART,MIPI
   connectivity between CPNX100 and CM5

## Tool versions

* Propel builder 2025.1.02506031608
* Radiant 2025.1.0.39.0
* Diamond 3.14.0.75.2

## Directory structure
|Item|Description  |
|--|--|
| gard_torna_revb_cpnx100.bit |  CPNX100 FPGA reference bitstream file provided in the package|
|torna_machxo3d_pwrup_impl1.jed|Xo3D FPGA reference bistream data for its config flash|
|mod_top_torna_revb_cpnx100_radiant.zip|Archive of CPNX100 Radiant top level project, rtl sources and constraints|
|xo3d.zip|Archive of the Diamond top level project, rtl sources and constraints|
|som_torna_revb_cpnx_gard_propel_builder.zip|Archive of Propel builder project instantiated in top level|
|mod_top_torna_revb_cpnx100_radiant/<br>mod_top_torna_revb_cpnx100.rdf|CPNX100 Radiant project|
|mod_top_torna_revb_cpnx100_radiant/impl_1|Reference implementation files of the CPNX100 Radiant project|
|mod_top_torna_revb_cpnx100_radiant/remote_files|All CPNX100 RTL sources and constraints|
|som_torna_revb_cpnx_gard_propel_builder/<br>som_torna_revb_cpnx_gard/cpnx_gard/cpnx_gard.sbx|CPNX100 Propel Builder project|
|mod_top_torna_revb_cpnx100_radiant/remote_files/sources/<br>src/rtl/mod_top.sv|CPNX100 Top level RTL file|
|mod_top_torna_revb_cpnx100_radiant/remote_files/sources/src/<br>constraints/mod_torna_revb_cpnx100_top_2L_912Mbps.pdc|CPNX100 design constraints file for 2 Lane MIPI passthrough at 1920x1080, 30 FPS|
|mod_top_torna_revb_cpnx100_radiant/remote_files/sources/src/<br>constraints/mod_torna_revb_cpnx100_top_4L_720Mbps_pt_or_ds_1L.pdc|Design constraints file for 4 Lane MIPI passthrough at 3280x2464, 30 FPS|
