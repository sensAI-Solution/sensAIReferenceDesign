#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Makefile Identification
#-----------------------------------------------------------------------------
MKFILE	:= $(realpath $(lastword $(MAKEFILE_LIST)))

#-----------------------------------------------------------------------------
# include Makefile.vars
#-----------------------------------------------------------------------------
VARS_FILE :=  $(HUB_DRIVERS_DIR)/lsc219/Makefile.vars
include $(VARS_FILE)

#-----------------------------------------------------------------------------
# Target for this Makefile
#-----------------------------------------------------------------------------
TGT_NAME  := $(notdir $(shell pwd))

#-----------------------------------------------------------------------------
# target output dirs
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(TGT_OUTPUT_DIR)/$(TGT_NAME)

#-----------------------------------------------------------------------------
# target definitions
#-----------------------------------------------------------------------------
DRV_FILE 	 	:= $(TGT_OUTPUT_DIR)/imx219.ko
DTBO_FILE 	 	:= $(TGT_OUTPUT_DIR)/$(TGT_NAME).dtbo

#-----------------------------------------------------------------------------
# Build type override
#-----------------------------------------------------------------------------
# Uncomment for release build
# BUILD_TYPE=release

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
else ifeq (release, $(BUILD_TYPE))
DEBUG_OPTS = -O2
else
$(error Unknown build option! Should be debug / release)
endif

CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

DRIVER_SRCS:=					\
	imx219.c					\

DTSI_SRCS:=						\
	lsc219_overlay.dtsi			\

#----------------------------------------------------------------------------
# IMX219-based camera driver for imx219 camera
#----------------------------------------------------------------------------
obj-m += $(patsubst %.c, %.o, $(DRIVER_SRCS))

#----------------------------------------------------------------------------
# Kernel source directory
#
# The Makefile assumes that the kernel headers have been installed
# in /lib/modules/$(shell uname -r)/build
#
# As of now, taken care of by scripts in the sister repo: HUB_packages
#----------------------------------------------------------------------------
KSRC := /lib/modules/$(shell uname -r)/build

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build_driver build_dtbo package clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: $(TGT_OUTPUT_DIR) build_driver build_dtbo

$(TGT_OUTPUT_DIR):
	$(MKDIR) $@

build_driver: $(MKFILE) $(DRIVER_SRCS) $(TGT_OUTPUT_DIR)
	@# Copy the driver srcs to the target output dir for module compile
	@rsync -a --delete $(DRIVER_SRCS) $(TGT_OUTPUT_DIR)
	$(MAKE) -C $(KSRC) M=$(TGT_OUTPUT_DIR) src=$(shell pwd) modules

build_dtbo: $(MKFILE) $(DTSI_SRCS) $(TGT_OUTPUT_DIR)
	kdtc -@ -O dtb -o $(DTBO_FILE) $(DTSI_SRCS)
	@echo "DTBO file generated: $(DTBO_FILE)"

package: build_driver build_dtbo $(OUT_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(DRV_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(DTBO_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(SCRIPTS_DIR)/enable_lsc219_cam.sh $(HUB_PKG_DIR)

enable_lsc219_cam: $(TGT_OUTPUT_DIR) build_driver build_dtbo $(OUT_FILE)
	$(SCRIPTS_DIR)/enable_lsc219_cam.sh $(TGT_OUTPUT_DIR)

clean:
	$(RM) $(TGT_OUTPUT_DIR)
