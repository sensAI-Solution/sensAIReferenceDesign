This folder hosts folders which contain directly-or-indirectly files that are needed to build GARD firmware. In addition to GARD firmware, the files needed to build the Firmware Loader are also placed here.  
Firmware and firmware loader are very closely related as the firmware loader is tasked with loading the firmware from the Flash and hence the firmware layout in Flash and some firmware internal locations need to be known to 
the Firmware loader for him to load the firmware.  
Folder interface located outside of gard_firmware also will contain files that need to be known to both GARD firmware and HUB software.  
Each folder within gard_firmware contains a README file that describes its contents. Please ensure that the README files stay relevant to the folder contents as new developers will find it extremely useful.  
