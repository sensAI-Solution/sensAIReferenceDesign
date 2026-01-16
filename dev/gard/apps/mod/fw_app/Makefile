#=============================================================================
#
# Copyright(c) 2025 Mirametrix Inc. All rights reserved.
#
# These coded instructions, statements, and computer programs contain
# unpublished proprietary information written by MMX and
# are protected by copyright law. They may not be disclosed
# to third parties or copied or duplicated in any form, in whole or
# in part, without the prior written consent of MMX.
#
#=============================================================================

# HUB project paths
PROJECT_BASE_DIR ?= ../../../platform_fw
BUILD_DIR := $(PROJECT_BASE_DIR)/build

include $(PROJECT_BASE_DIR)/build/Makefile.vars
include $(PROJECT_BASE_DIR)/src/gard_firmware/Makefile.vars
include $(PROJECT_BASE_DIR)/src/gard_firmware/fw/Makefile

# GARD APP project paths
APP_DIR := app_module
APP_OUTPUT_DIR = ./output

# To create a new app:
# * Create a folder for your app under ./app_module, eg ./app_module/your_project
# * Add your app_module.c and any other project-specific .c files in that folder.
#		./app_module/your_project/your_app_module.c
# * Include the name of your project when running make: `make build_app_module PROJECT=your_project`

PROJECT ?= mod_pipeline
ifeq ($(PROJECT),hmi_pipeline)
    PROJECT_DIR := hmi_pipeline
    PROJECT_DEFINES := SOME_DEFINE #Just put SOME_DEFINITION, the -D is added automatically
    PROJECT_CFLAGS := -Wno-error=inline # add Wno-error so inlining failures don't terminate the build
else ifeq ($(PROJECT),defect_detection_pipeline)
    PROJECT_DIR := defect_detection_pipeline
    PROJECT_DEFINES := ML_APP_MOD START_CAMERA_STREAM_ON_BOOT
    PROJECT_CFLAGS := -Wno-error=inline
else ifeq ($(PROJECT),mod_pipeline)
    PROJECT_DIR := mod_pipeline
    PROJECT_DEFINES := ML_APP_MOD START_CAMERA_STREAM_ON_BOOT
    PROJECT_CFLAGS := -Wno-error=inline
endif


DEBUG ?= false
ifeq ($(DEBUG),false)
    CONFIG_SUFFIX := 
    # In Release, add -Os so we don't overflow size
    DBG_REL_CFLAGS := -Os -flto
    # Build type for HUB. 
    BUILD_TYPE := release 
else
    PROJECT_DEFINES += DEBUG
    CONFIG_SUFFIX := _debug
    # In Debug, add -f**-sections flags to reduce binary size without
    # having to use -Os size optimizations (which prevent debugging).
    DBG_REL_CFLAGS := -ffunction-sections -fdata-sections
    # (Note these flags should probably not be used in Release:
    # 	https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#index-ffunction-sections)
    # Build type for HUB. 
    BUILD_TYPE := debug
endif


ifeq ($(OS), Windows_NT)
	# Use Propel's busybox.exe, which includes a find command (see busybox.exe --list)
	FIND_CMD := busybox.exe find
else
	FIND_CMD := find
endif

# Add app module files
APP_SRCS := $(wildcard $(shell $(FIND_CMD) ${APP_DIR}/$(PROJECT_DIR)/ -type f -name '*.c'))  $(shell $(FIND_CMD) $(APP_DIR)/common/ -type f -name '*.c') $(shell $(FIND_CMD) $(APP_DIR)/common/ -type f -name '*.S') 
INCLUDES := $(INCLUDES) $(shell $(FIND_CMD) $(APP_DIR)/common/ -type d) ${GARD_DIR}/fw
APP_OBJS := $(patsubst $(APP_DIR)/%.c,$(TGT_OUTPUT_DIR)/$(APP_DIR)/%.o,$(APP_SRCS))
OBJS += $(APP_OBJS)

# Extract unique directories from OBJS
DIRS := $(sort $(dir $(OBJS)))
DIRS += $(APP_OUTPUT_DIR)/$(PROJECT_DIR)

# Create directories if needed
.PHONY: create-dirs
create-dirs:
	@mkdir -p $(DIRS)

# Fix relative path in submodule so it's available for the app module linker step
GARD_FW_LD_FILE := $(GARD_FW_DIR)/$(GARD_FW_LD_FILE)

#extra rule for gard_fw/fw since it assumes relative path ./ in sub-makefile
$(TGT_OUTPUT_DIR)/%.o: $(GARD_FW_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

# Build GARD with extra C_OPTS to ignore unused-but-set-variable warnings
submodule_target:
	$(MAKE) -C $(BUILD_DIR) build_gard C_OPTS="$(C_OPTS) -Wno-error=unused-but-set-variable"

# avoid multiple definitions with sample app module and real app module
.PHONY: delete_sample_app
delete_sample_app:
	@rm $(TGT_OUTPUT_DIR)/sample_app_module.o

# Also delete it from the .o list
OBJS := $(filter-out %sample_app_module.o,$(OBJS))

# LDFLAGS in GCC format rather than LD format since we link with GCC here
LDFLAGS =						\
	-Wl,-Map,"$(TGT_OUTPUT_DIR)/$(TGT_NAME).map"	\
	-Wl,--print-memory-usage	\
	-Wl,--gc-sections			\
	-nostartfiles 

CFLAGS += $(DBG_REL_CFLAGS) $(PROJECT_CFLAGS) $(foreach d,$(PROJECT_DEFINES),-D$(d))

APP_MODULE_ELF := $(APP_OUTPUT_DIR)/$(PROJECT_DIR)/$(PROJECT_DIR)$(CONFIG_SUFFIX).elf
APP_MODULE_BIN := $(APP_OUTPUT_DIR)/$(PROJECT_DIR)/$(PROJECT_DIR)$(CONFIG_SUFFIX).bin
# link to .elf (used to launch OpenOCD from VS Code)
APP_MODULE_LNK := $(APP_OUTPUT_DIR)/app_module_last_built.elf

$(APP_MODULE_ELF): submodule_target delete_sample_app create-dirs $(OBJS) config_info_message
	@# create the .elf file
	$(CC) -T $(OUTPUT_LD_FILE) $(CFLAGS) $(LDFLAGS) -v $(OBJS) -o $@ -lgcc -lc
	@# print out (binary) section sizes in .elf
	$(SZ) $@

$(APP_MODULE_BIN): $(APP_MODULE_ELF)
	$(ELF_TO_BIN) $< $@

$(APP_MODULE_LNK): $(APP_MODULE_ELF)
	$$(ln -f $(APP_MODULE_ELF) $(APP_MODULE_LNK))

build_app_module: $(APP_MODULE_ELF) $(APP_MODULE_BIN) $(APP_MODULE_LNK)

.PHONY: clean_app_module

clean_app_module:
	@rm -rf $(OUTPUT_DIR)
	@rm -rf $(APP_OUTPUT_DIR)

config_info_message:
    $(info ***********************)
    $(info ***********************)
    $(info ***********************)
    $(info Hardware configuration:)
    $(info * DEVICE: SOM)
    $(info * FPGA: CPNX)
    $(info ***********************)
    $(info Software configuration:)
    $(info * Pipeline: $(PROJECT))
    $(info * App module debug mode: $(DEBUG))
    $(info * HUB build type: $(BUILD_TYPE))
    $(info ***********************)
    $(info ***********************)
    $(info ***********************)
