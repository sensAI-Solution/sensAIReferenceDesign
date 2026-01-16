#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# include Makefile_hub.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(GARD_DIR)/Makefile.vars
ifneq (setup, $(MAKECMDGOALS))
include $(VARS_FILE)
endif

#-----------------------------------------------------------------------------
# The following scripts are copied to the release folder during release target
# execution.
#-----------------------------------------------------------------------------
RELEASE_SCRIPTS:=							\
	$(SCRIPTS_DIR)/CameraConfigBuilder.py	\
	$(SCRIPTS_DIR)/xlate_camera_config.py	\
	$(SCRIPTS_DIR)/RootImageBuilder.py	\
	$(SCRIPTS_DIR)/RootImageBuilder-ConfigParser.py	\
	$(SCRIPTS_DIR)/RootImageBuilder-Config.yaml

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build build_fw build_fw_loader clean clean_fw clean_fw_loader

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: build_fw build_fw_loader
clean: clean_fw clean_fw_loader

build_fw:
	$(MAKE) --output-sync=target -C $(GARD_FW_DIR) build

build_fw_loader:
	$(MAKE) --output-sync=target -C $(GARD_FW_LDR_DIR) build

clean_fw:
	$(MAKE) -C $(GARD_FW_DIR) clean

clean_fw_loader:
	$(MAKE) -C $(GARD_FW_LDR_DIR) clean

release: build copy_files_to_release_folder

copy_files_to_release_folder: copy_fw_files_to_release_folder copy_fw_ldr_files_to_release_folder copy_scripts_to_release_folder

copy_fw_files_to_release_folder:
	$(MAKE) -C $(GARD_FW_DIR) release

copy_fw_ldr_files_to_release_folder:
	$(MAKE) -C $(GARD_FW_LDR_DIR) release

copy_scripts_to_release_folder:
	$(COPY) $(RELEASE_SCRIPTS) $(RELEASE_DIR)/

run_scripts_in_release_folder:
	$(CD) $(RELEASE_DIR) && $(PYTHON) RootImageBuilder.py --help
