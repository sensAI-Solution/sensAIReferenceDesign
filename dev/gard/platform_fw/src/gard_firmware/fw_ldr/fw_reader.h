/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef FW_READER_H
#define FW_READER_H

#include "gard_types.h"
#include "assert.h"
#include "ospi_support.h"

/**
 * locate_firmware_in_flash locates the firmware in flash memory and returns
 * the address where it is stored.
 */
bool locate_firmware_in_flash(void     *ospi_handle,
							  uint32_t *fw_flash_addr,
							  uint32_t *fw_size)
	__attribute__((section(".lowmem")));

void load_firmware_from_flash_and_execute(void    *ospi_handle,
										  uint32_t fw_flash_addr,
										  uint32_t fw_size,
										  void    *fw_buffer);

#endif /* FW_READER_H */
