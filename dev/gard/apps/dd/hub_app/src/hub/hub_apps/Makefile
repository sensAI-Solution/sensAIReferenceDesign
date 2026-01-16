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
# Target for this Makefile
#-----------------------------------------------------------------------------
TGT_NAME  := hub_app

#-----------------------------------------------------------------------------
# Build type override
#-----------------------------------------------------------------------------
# Uncomment for release build
# BUILD_TYPE=release

#-----------------------------------------------------------------------------
# target output dirs
#-----------------------------------------------------------------------------
TGT_OUTPUT_DIR:= $(OUTPUT_DIR)/$(TGT_NAME)

#-----------------------------------------------------------------------------
# target definitions
#-----------------------------------------------------------------------------
# Basic test app
MINAPP_ELF_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME)_minimal.elf
# STREAMING app
STREAMING_ELF_FILE    := $(TGT_OUTPUT_DIR)/$(TGT_NAME)_streaming.elf
# IMG OPS app
IMG_OPS_ELF_FILE    := $(TGT_OUTPUT_DIR)/$(TGT_NAME)_img_ops.elf

APP_PY_FILE 	 := $(HUB_APP_DIR)/app.py

#-----------------------------------------------------------------------------
# Space separated defines for compiler call(s)
# Example: To add "-DMY_OPTION -DMY_OPTION2",
# 		   use "DEFINES += MY_OPTION1 MY_OPTION2"
# Note:    These defines will apply to all compiler invocations
#-----------------------------------------------------------------------------
DEFINES +=							    \
	$(TGT_SUBSYSTEM)

INCLUDES +=					            \
	$(HUB_APP_DIR)                      \
	$(HUB_INC_DIR)						\

MINAPP_SRCS :=							\
	app.c

STREAMING_SRCS :=						\
	streaming_app.c							

IMG_OPS_SRCS :=							\
	img_ops_app.c							

MINAPP_OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(MINAPP_SRCS)))
STREAMING_OBJS 	:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(STREAMING_SRCS)))
IMG_OPS_OBJS 	:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(notdir $(IMG_OPS_SRCS)))

MINAPP_DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(MINAPP_SRCS)))
STREAMING_DEPS 	:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(STREAMING_SRCS)))
IMG_OPS_DEPS 	:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(notdir $(IMG_OPS_SRCS)))

ifeq (debug, $(BUILD_TYPE))
DEBUG_OPTS = -O0 -g -ggdb3
else ifeq (release, $(BUILD_TYPE))
DEBUG_OPTS = -O2
else
$(error Unknown build option! Should be debug / release)
endif

SO_PATH = $(OUTPUT_DIR)/$(notdir $(HUB_LIB_DIR))
SO_FILE = $(basename $(HUB_LIB_NAME))

CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))
CFLAGS += -Wl,-rpath=$(SO_PATH)
CFLAGS += -Wl,-rpath=$(HUB_EXT_INSTALLS_DIR)/lib 

CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libcjson --cflags)
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libusb-1.0 --cflags)
CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libgpiod --cflags)

LDFLAGS += -L$(SO_PATH) -l$(SO_FILE:lib%=%)
LDFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libcjson --libs) 
LDFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libusb-1.0 --libs)
LDFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config libgpiod --libs)
LDFLAGS += -lpthread

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all build run_minimal_app run_streaming_app run_memcheck \
		run_img_ops_app run_minimal_py run_streaming_py package clean

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: $(TGT_OUTPUT_DIR) $(DEPS) $(MINAPP_ELF_FILE) $(STREAMING_ELF_FILE) \
       $(IMG_OPS_ELF_FILE)

run_minimal_app: $(MINAPP_ELF_FILE)
	$(MINAPP_ELF_FILE) $(CONFIG_DIR)/host_config.json $(CONFIG_DIR)

run_streaming_app: $(STREAMING_ELF_FILE)
	$(STREAMING_ELF_FILE) $(CONFIG_DIR)/host_config.json $(CONFIG_DIR)
	
run_img_ops_app: $(IMG_OPS_ELF_FILE)
	$(IMG_OPS_ELF_FILE) $(CONFIG_DIR)/host_config.json $(CONFIG_DIR) $(OUTPUT_DIR)

run_streaming_py:
	python streaming_app.py

run_minimal_py:
	python app.py
	
run_memcheck: $(ELF_FILE)
	valgrind --leak-check=full --show-leak-kinds=all $(MINAPP_ELF_FILE) \
	$(CONFIG_DIR)/host_config.json $(CONFIG_DIR)

package: build $(HUB_PKG_DIR)
	@$(COPY) $(MINAPP_ELF_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(STREAMING_ELF_FILE) $(HUB_PKG_DIR)
	@$(COPY) $(IMG_OPS_ELF_FILE) $(HUB_PKG_DIR)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $@

$(TGT_OUTPUT_DIR)/%.o: %.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(COMMON_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

#-----------------------------------------------------------------------------
# - Generate dependencies (.d) with full paths for each .o
# - Make TGT_OUTPUT_DIR "order-only-prerequisite"
#   To ensure that no .d is rebuilt if its timestamp changes
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: %.c $(MKFILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.c,$@) -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(MINAPP_ELF_FILE): $(MINAPP_OBJS) $(MKFILE)
	$(CC) $(CFLAGS) $(MINAPP_OBJS) $(LDFLAGS) -o $@

$(STREAMING_ELF_FILE): $(STREAMING_OBJS) $(MKFILE)
	$(CC) $(CFLAGS) $(STREAMING_OBJS) $(LDFLAGS) -o $@

$(IMG_OPS_ELF_FILE): $(IMG_OPS_OBJS) $(MKFILE)
	$(CC) $(CFLAGS) $(IMG_OPS_OBJS) $(LDFLAGS) -o $@

clean:
	$(RM) $(TGT_OUTPUT_DIR)
