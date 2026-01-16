/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "ospi_support.h"
#include "flashmap.h"
#include "fw_reader.h"
#include "fw_info.h"
#include "data_xchg_space.h"
#include "memmap.h"
#include "rfs.h"
#include "utils.h"
#include "fw_core.h"


/**
 * get_default_fw_uid() reads the configuration section from flash and returns
 * the default firmware UID that is to be loaded by the firmware loader.
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param fw_uid pointer to be filled with the default firmware UID.
 *
 * @return true if the default firmware UID is read successfully, false
 *         otherwise.
 */
static bool get_default_fw_uid(void *ospi_handle, uint32_t *fw_uid)
{
	struct rfs_config config;

	if (NULL == ospi_handle || NULL == fw_uid) {
		return false;
	}

	GARD__DBG_ASSERT(
		load_configuration_section(ospi_handle, &config, sizeof(config)),
		"Failed to read configuration section from flash");

	*fw_uid = config.firmware_uid;

	return true;
}

/**
 * locate_firmware_in_flash locates the firmware in flash memory and returns
 * the address where it is stored.
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param fw_flash_addr pointer to be filled with the flash address of the
 * firmware.
 * @param fw_size pointer to be filled with the size of the firmware.
 *
 * @return true if the firmware is located successfully, false otherwise.
 */
bool locate_firmware_in_flash(void     *ospi_handle,
							  uint32_t *fw_flash_addr,
							  uint32_t *fw_size)
{
	uint32_t              uid_of_fw_to_load;

	if (ospi_handle == NULL || fw_flash_addr == NULL) {
		return false;
	}

	if (get_default_fw_uid(ospi_handle, &uid_of_fw_to_load) == false) {
		/* Failed to get default firmware UID. */
		return false;
	}

	if (0 == uid_of_fw_to_load) {
		/* No default firmware UID is configured. Stay in an endless loop */
		while (true) {
		}
	}

	return locate_module_id(ospi_handle, uid_of_fw_to_load,
							fw_flash_addr, fw_size);
}

/**
 * load_firmware_from_flash_and_execute reads the firmware upto specified size
 * from flash memory and copies it to the provided buffer. Even though the size
 * to be read is provided this routine will first read the firmware in flash and
 * find out if the firmware is coded to load in two parts. For such a firmware
 * only the first (lower) part of the firmware is loaded by this routine. The
 * routine will then update the in-memory firmware variable
 * fw_part2_load_addr_in_flash with the address of the last byte of the partial
 * firmware read from the flash. In either case the routine will return the
 * count of bytes of firmware it read from the flash which can help the caller
 * know if the complete firmware was loaded by the routine or not..
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param fw_flash_addr is the address in flash from where the firmware is to be
 * 						loaded.
 * @param fw_size is the size of the firmware to be loaded.
 * @param fw_buffer is the buffer where the firmware is to be copied.
 *
 * @return byte count of firmware loaded in memory from flash.
 *
 */
void load_firmware_from_flash_and_execute(void    *ospi_handle,
										  uint32_t fw_flash_addr,
										  uint32_t fw_size,
										  void    *fw_buffer)
{
	uint32_t       fw_bytes_to_read = 0;
	bool           is_partial_load  = true;
	extern uint8_t _start_of_himem_area;
	uint32_t       high_mem_start_addr;

	high_mem_start_addr = (uint32_t)(&(_start_of_himem_area) - (uint8_t *)0);

	GARD__DBG_ASSERT(
		ospi_handle != NULL && fw_size != 0,
		"Invalid parameters provided to load_firmware_from_flash_and_execute");

	/**
	 * At this point there is no further validation of the firmware as the
	 * firmware carries no signature to verify. We assume that the firmware is
	 * located at the flash location and continue.
	 *
	 * We first read an explicit location within the firmware to check if the
	 * firmware supports partial loading. If it does then we only load that much
	 * of the firmware in memory, if it does not then we load the complete
	 * firmware image in memory from the flash. The firmare is loaded at addr
	 * 0x00000000 in memory.
	 */
	GARD__ASSERT(ospi_read_from_flash(ospi_handle, (uint8_t *)&fw_bytes_to_read,
									  fw_flash_addr + FW_PART1_SIZE_OFFSET,
									  sizeof(fw_bytes_to_read)) ==
					 sizeof(fw_bytes_to_read),
				 "Failed to read firmware from flash");

	if (fw_bytes_to_read == 0) {
		/**
		 * Firmware does not support partial loading, read the complete firmware
		 * from flash iff it does not overwrite the firmware loader code present
		 * in high memory.
		 */
		GARD__DBG_ASSERT(fw_size <= high_mem_start_addr,
					 "Firmware size overwrites the HIMEM area of the loader. "
					 "Please compile firmware with Partial load support.");
		fw_bytes_to_read = fw_size;
		is_partial_load  = false;
	} else {
		/**
		 * Read only partial firmware, the rest of the loading will be done by
		 * the firmware itself.
		 */
		GARD__DBG_ASSERT(
			fw_bytes_to_read <= high_mem_start_addr,
			"Firmware low mem size overwrites the HIMEM area of the "
			"loader. Please try trimming the low memory area of the firmware.");
	}

	GARD__ASSERT(ospi_read_from_flash(
					 ospi_handle, (uint8_t *)(fw_buffer - FW_START_ADDR),
					 fw_flash_addr, fw_bytes_to_read) == fw_bytes_to_read,
				 "Failed to read firmware from flash");

	if (is_partial_load) {
		uint32_t *fw_part2_start_addr_in_flash;

		fw_part2_start_addr_in_flash =
			(uint32_t *)(FW_PART2_START_ADDR_IN_FLASH_OFFSET);

		/**
		 * Update the address in memory which tells the firmware the flash
		 * location it should load the rest of the firmware. This does not tell
		 * how much of the firmware needs to be loaded, that is to be figured
		 * out by the firmware.
		 */

		/**
		 * Since the firmware memory map of the extreme lower section of TCM is
		 * the same for both the firmware and firmware-loader, updating the
		 * flash address in this range is visible to both the firmware and the
		 * firmware loader code.
		 */

		*fw_part2_start_addr_in_flash = fw_flash_addr + fw_bytes_to_read;
	}

	/**
	 * RISCV fence instructions for data and instruction memory
	 * are used to ensure that all memory accesses are completed
	 * before jumping to the firmware code.
	 * fence.i provides explicit synchronization between writes to instruction memory and instruction fetches on the same hart,
	 * as advised in the RISC-V specification for loading new code into memory.
	 */
	__asm__ __volatile__("fence rw, rw");
	__asm__ __volatile__("fence.i");

	// Now jump to the firmware code which is at the start of TCM.
	__asm__ volatile("j " STR(FW_START_ADDR));

	__builtin_unreachable();
}

/**
 * Just reserve the space in memory for the relocation table of the firmware.
 */
DEFINE_APP_MODULE_CALLBACKS(NULL, NULL, NULL, NULL, NULL, NULL);
