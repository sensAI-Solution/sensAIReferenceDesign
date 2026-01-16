#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Location of this Makefile w.r.t. base directory
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
$(error Full path of Makefile contains spaces - Cannot continue!)
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
.PHONY: all clean clean_gard build build_gard release rebuild

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build:
	$(MAKE) -C $(GARD_DIR) build

clean:
	$(MAKE) -C $(GARD_DIR) clean

build_gard: build

clean_gard: clean

rebuild: clean build

release: build create_release_folder
	$(MAKE) -C $(GARD_DIR) release

create_release_folder:
	$(MKDIR) $(RELEASE_DIR)

