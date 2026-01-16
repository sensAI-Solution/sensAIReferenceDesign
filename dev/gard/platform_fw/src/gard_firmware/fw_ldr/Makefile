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

TGT_NAME:= gard_fw_ldr
GARD_FW_LDR_LD_FILE:= $(TGT_NAME).ld

INCLUDES +=							\
	$(COMMON_DIR)					\
	$(BSP_DIR)						\
	$(INTERFACE_DIR)				\
	$(INC_DIR)

SRCS:=										\
	$(COMMON_DIR)/compiler_check.c			\
	$(COMMON_DIR)/assert.c					\
	$(OSPI_BSP_DIR)/octal_spi_controller.c	\
	$(BSP_DIR)/start.S						\
	$(GARD_FW_LDR_DIR)/main.c				\
	$(COMMON_DIR)/data_xchg_space.c			\
	$(COMMON_DIR)/rfs.c						\
	$(COMMON_DIR)/utils.c					\
	$(GARD_FW_LDR_DIR)/ospi_support.c		\
	$(GARD_FW_LDR_DIR)/fw_reader.c

TGT_OUTPUT_DIR:= $(GARD_FW_LDR_OUT_DIR)

#
# TBD to add proper support for clang-tidy. The lines in this file are used when
# clang-tidy is run manually.
#
LINT_TOOL_CFG=$(LUNA_BASE_DIR)/.clang-tidy
LINT_TOOL_OPTS =									\
	--config-file='$(LINT_TOOL_CFG)'

OBJS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.o, $(filter %.c, $(notdir $(SRCS))))
OBJS 		+= $(patsubst %.S, $(TGT_OUTPUT_DIR)/%.o, $(filter %.S, $(notdir $(SRCS))))
DEPS 		:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.d, $(filter %.c, $(notdir $(SRCS))))
DEPS 		+= $(patsubst %.S, $(TGT_OUTPUT_DIR)/%.d, $(filter %.S, $(notdir $(SRCS))))
LINTOUTS	:= $(patsubst %.c, $(TGT_OUTPUT_DIR)/%.c.lintout, $(filter %.c, $(notdir $(SRCS))))
LINTOUTS	+= $(patsubst %.S, $(TGT_OUTPUT_DIR)/%.c.lintout, $(filter %.S, $(notdir $(SRCS))))

OUTPUT_LD_FILE   := $(TGT_OUTPUT_DIR)/$(TGT_NAME)_final.ld
BIN_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME).bin
ELF_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME).elf
MAP_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME).map
DIS_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME).S
MEM_FILE         := $(TGT_OUTPUT_DIR)/$(TGT_NAME).mem

CFLAGS  = $(C_OPTS)
CFLAGS += $(DEBUG_OPTS)

DEFINES +=	CPU_32BIT		\
			GARD_FW_LDR		\
			GARD_DEBUG

CFLAGS += $(foreach i,$(INCLUDES),-I$(i))
CFLAGS += $(foreach d,$(DEFINES),-D$(d))

LDFLAGS += -Map $(MAP_FILE)

FW_DEPS_TARGETS := build all builf_fw

#-----------------------------------------------------------------------------
# Phony targets
#-----------------------------------------------------------------------------
.PHONY: all

#-----------------------------------------------------------------------------
# targets
#-----------------------------------------------------------------------------
all: build

build: $(TGT_OUTPUT_DIR) $(DEPS) $(OUTPUT_LD_FILE) $(BIN_FILE) $(MEM_FILE)

$(TGT_OUTPUT_DIR):
	$(MKDIR) $(TGT_OUTPUT_DIR)

$(BIN_FILE): $(ELF_FILE)
	$(ELF_TO_BIN) $< $@

$(OUTPUT_LD_FILE): $(GARD_FW_LDR_LD_FILE)
	$(CC) -I$(INC_DIR) $(PRE_PROCESS_ONLY) $< > $@

$(TGT_OUTPUT_DIR)/%.o: %.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(COMMON_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(UART_BSP_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(I2C_BSP_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(OSPI_BSP_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(BSP_DIR)/%.S $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TGT_OUTPUT_DIR)/%.o: $(BSP_DIR)/%.c $(MKFILE)
	$(CC) $(CFLAGS) -c $< -o $@


#-----------------------------------------------------------------------------
# - Generate dependencies (.d) with full paths for each .o
# - Make TGT_OUTPUT_DIR "order-only-prerequisite"
#   To ensure that no .d is rebuilt if its timestamp changes
#-----------------------------------------------------------------------------
$(TGT_OUTPUT_DIR)/%.d: %.c $(MKFILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(COMMON_DIR)/%.c $(MKFILE) | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(UART_BSP_DIR)/%.c | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(I2C_BSP_DIR)/%.c | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(OSPI_BSP_DIR)/%.c | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(BSP_DIR)/%.S | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(TGT_OUTPUT_DIR)/%.d: $(BSP_DIR)/%.c | $(TGT_OUTPUT_DIR)
	$(CC) -MM -MT $(patsubst %.d,%.o,$@) -MT $@ $(CFLAGS) $< > $@

$(ELF_FILE): $(OBJS) $(OUTPUT_LD_FILE)
	@# create the .elf file
	$(LD) $(LDFLAGS) -v $(OBJS) -o $@
	@# print out (binary) section sizes in .elf
	$(SZ) $@
	$(GEN_DIS) $@ > $(DIS_FILE)

$(MEM_FILE): $(BIN_FILE)
	PADDED_FILE_SIZE=$$(((($$(stat -c %s $(BIN_FILE)) + 3) / 4) * 4)); \
	$(BIN_TO_MEM)

build_fw:
	$(MAKE) -C $(GARD_FW_LDR_DIR)

clean:
	$(RM) $(GARD_OUT_DIR)

release:
	$(COPY) $(BIN_FILE) $(RELEASE_DIR)/
	$(COPY) $(ELF_FILE) $(RELEASE_DIR)/
	$(COPY) $(MEM_FILE) $(RELEASE_DIR)/

ifneq ($(filter $(MAKECMDGOALS), $(FW_DEPS_TARGETS)),)
include $(DEPS)
endif
