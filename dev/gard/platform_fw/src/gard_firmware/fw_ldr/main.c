/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "sys_platform.h"
#include "octal_spi_controller.h"
#include "ospi_support.h"
#include "fw_reader.h"
#include "memmap.h"

/**
 * main() is the entry point of the firmware loader after the assembly code has
 * setup the stack and initialized the BSS section. This routine performs three
 * main tasks:
 * 1. Initializes the OSPI controller.
 * 2. Locates the firmware in flash memory.
 * 3. Loads the firmware from flash memory and executes it.
 *
 * The code never reaches the end of main as
 * load_firmware_from_flash_and_execute() jumps to the firmware code and does
 * not return.
 *
 * @param None
 *
 * @return This function does not return as it jumps to the firmware code.
 *         However, it is defined to return int to satisfy the C standard.
 */
int main(void)
{
	void    *ospi_handle;
	uint32_t fw_flash_addr;
	uint32_t fw_size;

	ospi_handle = ospi_init();
	GARD__DBG_ASSERT(ospi_handle != NULL, "OSPI initialization failed");

	GARD__DBG_ASSERT(
		locate_firmware_in_flash(ospi_handle, &fw_flash_addr, &fw_size),
		"Failed to locate firmware in flash");

	load_firmware_from_flash_and_execute(ospi_handle, fw_flash_addr, fw_size,
										 (void *)FW_START_ADDR);

	/**
	 * This line will never be reached, but it is here to avoid compiler
	 * warnings about reaching the end of main without returning a value.
	 */
	return 0;
}
