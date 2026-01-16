#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Location of this Makefile w.r.t. HUB base directory
# Note: Change this variable when you change the relative location
#-----------------------------------------------------------------------------
MKFILE_REL_PATH := build/

#-----------------------------------------------------------------------------
# populate the PROJECT base directory
#-----------------------------------------------------------------------------
MKFILE            := $(realpath $(lastword $(MAKEFILE_LIST)))
# TBD-DPN Support full paths that contain spaces
MKFILE_PATH_WORDS := $(words $(MKFILE))
ifneq (1, $(MKFILE_PATH_WORDS))
$(error Full path of HUB Makefile contains spaces - Cannot continue!)
endif
PROJECT_BASE_DIR  := $(abspath $(subst $(MKFILE_REL_PATH),,$(dir $(MKFILE))))

#-----------------------------------------------------------------------------
# build dir
#-----------------------------------------------------------------------------
BUILD_DIR := $(PROJECT_BASE_DIR)/build

#-----------------------------------------------------------------------------
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(BUILD_DIR)/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all clean dist_clean setup_hub 										\
	build_hub run_hub_minimal_app run_hub_minimal_py run_hub_streaming_app	\
	run_hub_streaming_py package_hub clean_hub

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

setup: setup_hub

build: build_hub

package: package_hub

clean: clean_hub

# implicitcly called when HUB is built
setup_hub:
	$(MAKE) -C $(HUB_DIR) setup

build_hub:
	$(MAKE) -C $(HUB_DIR) build

package_hub: 
	$(MAKE) -C $(HUB_DIR) package

# Application related targets
run_hub_minimal_app: build_hub
	$(MAKE) -C $(HUB_DIR) run_minimal_app

run_hub_streaming_app: build_hub
	$(MAKE) -C $(HUB_DIR) run_streaming_app

run_hub_img_ops_app: build_hub
	$(MAKE) -C $(HUB_DIR) run_img_ops_app

run_hub_streaming_py: build_hub
	$(MAKE) -C $(HUB_DIR) run_streaming_py

run_hub_minimal_py: build_hub
	$(MAKE) -C $(HUB_DIR) run_minimal_py

# Camera related target(s)
enable_lsc219_cam:
	$(MAKE) -C $(HUB_DIR) enable_lsc219_cam

# TBD-DPN:
# This is for HUB compile and build on RPi. 
# To be re-visited for other platforms (like HUB on X86)
memcheck_hub: build_hub
	$(MAKE) -C $(HUB_DIR) run_memcheck

# cleanup related targets
clean_hub:
	$(MAKE) -C $(HUB_DIR) clean

# Cleans up HUB and its dependencies
dist_clean: clean_hub
	$(MAKE) -C $(HUB_DIR) dist_clean