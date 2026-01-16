#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#----------------------------------------------------------------------------
# This is a mid-level Makefile that builds all HUB drivers
# It is invoked by the top-level HUB Makefile
# Currently, we only have one driver - lsc219
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile Identification
#-----------------------------------------------------------------------------
MKFILE	:= $(realpath $(lastword $(MAKEFILE_LIST)))

#-----------------------------------------------------------------------------
# Target for this Makefile
#-----------------------------------------------------------------------------
TGT_NAME  := $(notdir $(shell pwd))

#-----------------------------------------------------------------------------
# target output dirs
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(OUTPUT_DIR)/$(TGT_NAME)

all:
	make -C lsc219 all

package:
	make -C lsc219 package

enable_lsc219_cam:
	$(MAKE) -C lsc219 enable_lsc219_cam

clean:
	make -C lsc219 clean
	$(RM) $(TGT_OUTPUT_DIR)

export