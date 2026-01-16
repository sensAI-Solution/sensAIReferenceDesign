/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef FW_INFO_H
#define FW_INFO_H

#include "gard_types.h"
#include "app_module.h"

/**
 * The following offsets are the same between the GARD firmware loader and the
 * GARD firmware. It is important that these offsets are retained the same
 * forever.
 */

/* This offset is where the firmware's part-1 size if embedded. */
#define FW_PART1_SIZE_OFFSET       0x00000004
#define FW_PART1_SIZE_OFFSET_BYTES 4

/**
 * This offset is where the firmware loader writes (to in-memory copy) the flash
 * address from where the firmware should load the 2nd part of the firmware.
 */
#define FW_PART2_START_ADDR_IN_FLASH_OFFSET                                    \
	(FW_PART1_SIZE_OFFSET + FW_PART1_SIZE_OFFSET_BYTES)
#define FW_PART2_START_ADDR_IN_FLASH_OFFSET_BYTES 4

/**
 * This offset is where the firmware indicates if it has been compiled to work
 * on XIP mode of flash.
 */
#define FW_XIP_MODE_FLAG_OFFSET                                                \
	(FW_PART2_START_ADDR_IN_FLASH_OFFSET + FW_PART2_START_ADDR_IN_FLASH_OFFSET_BYTES)
#define FW_XIP_MODE_FLAG_OFFSET_BYTES 4

/**
 * This offset is where the count of entries in firmware's relocation table is
 * embedded.
 */
#define FW_RELOC_TABLE_ENTRY_COUNT_OFFSET                                      \
	(FW_XIP_MODE_FLAG_OFFSET + FW_XIP_MODE_FLAG_OFFSET_BYTES)
#define FW_RELOC_TABLE_ENTRY_COUNT_OFFSET_BYTES 4

/**
 * This offset is where the firmware's relocation table is embedded.
 */
#define FW_RELOC_TABLE_OFFSET                                                  \
	(FW_RELOC_TABLE_ENTRY_COUNT_OFFSET +                                       \
	 FW_RELOC_TABLE_ENTRY_COUNT_OFFSET_BYTES)

/**
 * Currently this table contains fixed entries. If more entries need to be added
 * to this table then they need to be manually added and the code recompiled.
 */

#endif /* FW_INFO_H */
