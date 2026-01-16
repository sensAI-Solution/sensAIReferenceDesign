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
# Makefile for HUB library
#-----------------------------------------------------------------------------
TGT_DIR  := $(notdir $(HUB_LIB_DIR))
TGT_NAME := $(HUB_LIB_NAME)

#-----------------------------------------------------------------------------
# Build type override
#-----------------------------------------------------------------------------
# Uncomment for release build
#BUILD_TYPE=release

#-----------------------------------------------------------------------------
# target output dirs
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(OUTPUT_DIR)/$(TGT_DIR)

#-----------------------------------------------------------------------------
# target definitions
#-----------------------------------------------------------------------------
LIB_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME)

#-----------------------------------------------------------------------------
# Space separated defines for compiler call(s)
# Example: To add "-DMY_OPTION -DMY_OPTION2",
# 		   use "DEFINES += MY_OPTION1 MY_OPTION2"
# Note:    These defines will apply to all compiler invocations
#-----------------------------------------------------------------------------
DEFINES +=							    \
	$(TGT_SUBSYSTEM)

INCLUDES +=					            \
	$(HUB_LIB_DIR)			            \
	$(HUB_INC_DIR)						\
	$(INTERFACE_DIR)

SRCS :=									\
	hub_preinit.c						\
	hub_init.c							\
	hub_utils.c							\
	hub_globals.c						\
	hub_reg_ops.c						\
	hub_bulk_ops.c						\
	hub_data_ops.c						\
	hub_i2c.c							\
	hub_uart.c							\
	hub_usb.c							\
	hub_som_sensors.c					\
	hub_threading.c						\
	hub_gpio.c							\
	hub_img_ops.c						\
	hub_gard_cmds.c

HUB_HDR_FILE := $(HUB_INC_DIR)/hub.h

OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(SRCS)))
DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(SRCS)))

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
else ifeq (release, $(BUILD_TYPE))
DEBUG_OPTS = -O2
else
$(error Unknown build option! Should be debug / release)
endif

CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libcjson --cflags)
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libusb-1.0 --cflags)
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libgpiod --cflags)

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build package clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: $(TGT_OUTPUT_DIR) $(DEPS) $(LIB_FILE)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $(TGT_OUTPUT_DIR)

$(TGT_OUTPUT_DIR)/%.o: %.c $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(COMMON_DIR)/%.c $(MKFILE) $(VARS_FILE)
	$(CC) $(CFLAGS) -c $< -o $@

package: $(LIB_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(LIB_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(HUB_HDR_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(CONFIG_DIR) $(HUB_PKG_DIR)
	$(COPY_FROM_LINK) $(HUB_EXT_INSTALLS_DIR)/lib/*.so* $(HUB_PKG_DIR)

#-----------------------------------------------------------------------------
# - Generate dependencies (.d) with full paths for each .o
# - Make TGT_OUTPUT_DIR "order-only-prerequisite"
#   To ensure that no .d is rebuilt if its timestamp changes
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: %.c $(MKFILE) $(VARS_FILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(LIB_FILE): $(OBJS) $(HUB_HDR_FILE)
	$(CC) $(OBJS) -shared -o $@

clean:
	$(RM) $(TGT_OUTPUT_DIR)
