/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

/**
 * This file holds all the instances of global variables that are used
 * between the firmware loader and the firmware.
 */

#include "gard_types.h"
#include "data_xchg_space.h"
#include "fw_info.h"
#include "fw_core.h"

extern uint8_t _bss_end;

/**
 * fw_part1_load_size contains the size which the firmware loader code should
 * load of the firmware from the Flash to the lower memory of the TCM.
 * The firmware will continue to load the 2nd part of the firwmare from the
 * flash starting from this address till the complete firmware is loaded in TCM.
 *
 * If this variable is set to 0 then the firmware loader will load the
 * complete firmware in TCM in one go.
 */
uint32_t fw_part1_load_size __attribute__((section(".fw_part1_sz_section"))) =
#ifdef SUPPORT_PARTIAL_FW_LOAD
	(uint32_t)&_bss_end;
#else
	0x00000000;
#endif

/**
 * fw_part2_load_addr_in_flash is the address in flash from where the second
 * part of the firmware is to be loaded in TCM. This value is initialized by the
 * firmware loader and used by the firmware to get the starting address in flash
 * memory from where to load the second part of the firmware in TCM.
 */
uint32_t fw_part2_load_addr_in_flash
	__attribute__((section(".fw_part2_flash_addr_section"))) = 0x00000000;

/**
 * The following variables are only available in XIP mode.
 */
uint32_t fw_reloc_table_entry_count
	__attribute__((section(".fw_reloc_table_entry_count_section"))) =
		sizeof(struct app_module_cbs_table) / sizeof(uint32_t);

/**
 * fw_run_in_xip_mode is the flag that indicates if the firmware will execute
 * from TCM or if it will execute from XIP. In XIP mode the firmware could have
 * some parts in the TCM and some part in Flash to be executed in XIP mode.
 */
#ifdef SUPPORT_XIP_MODE
uint32_t fw_run_in_xip_mode
	__attribute__((section(".fw_run_in_xip_flag_section"))) = 0x1;
#else
uint32_t fw_run_in_xip_mode
	__attribute__((section(".fw_run_in_xip_flag_section"))) = 0x0;
#endif
