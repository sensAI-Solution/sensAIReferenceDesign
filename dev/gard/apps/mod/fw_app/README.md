# GARD AI Pipeline
AI Pipeline, based on the GARD FW (Golden AI Reference Design).


Compiling the firmware
* Use the Makefile command `make build_app_module PROJECT=some_project` in the root folder. If there's no `PROJECT=`, it will default to building the HMI pipeline. This will generate the .elf and .bin files for the project.


Adding a new app
* Add your app to the Makefile, following the template starting from the line `PROJECT ?= hmi_pipeline`. Add any new definitions, cflags, etc that your app requires. Make sure to create a project directory in ./app_module to put any .c files specific to your app. Any folders in your project directory will be added recursively when compiling.

Debugging
* After generating the .elf file, add its path to the "Attach Firmware" launch config and then press run. The elf file is not automatically compiled by the debugger, you have to run the make command yourself.

