/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

/**
 * RFS = Root File System
 * This is a basic file-system which is used to track the modules located on the
 * flash. Unlike conventional file-systems, this file-system is not
 * hierarchical and does not have directories. It is a flat file-system which
 * contains a configuration section, a directory section and the space to hold
 * the modules.
 * The configuration section contains the metadata about the directory.
 * The directory section contains the entries for each module present on the
 * flash.
 * Each module is located as a contiguous block of data on the flash memory.
 *
 * Each modules can be one of the following:
 * 1) GARD Firmware
 * 2) ML network
 * 3) Camera Configurations
 * 4) GARD Profile
 *
 * 16-bit UIDs are used to represent the modules instead of having an ASCII
 * string as a module name. The UIDs are unique within the RFS.
 * Further the UIDs are grouped to be used by a family of modules.
 * Thus:
 * - GARD firmware modules will use a UID between 0x1001 to 0x1FFF. No two GARD
 *   firwmare modules on the flash will have the same UID.
 * - ML Network modules will use a UID between 0x2001 to 0x2FFF. No two ML
 *   Network modules on the flash will have the same UID.
 * - Camera Configuration modules will use a UID between 0x3001 to 0x3FFF. No
 *   two Camera Configuration modules on the flash will have the same UID.
 * - GARD Profile will use a UID between 0x4001 to 0x4FFF. Since there will be
 *   only one GARD Profile, the UID will be unique.
 */

/**
 *
 * This file contains the routines which are used to parse and read the
 * configuration and directory sections of the flash memory. The routines here
 * used to locate the modules on the flash and also read them in TCM if
 * requeted.
 */

#include "types.h"
#include "flashmap.h"
#include "fw_info.h"
#include "ospi_support.h"
#include "memmap.h"
#include "assert.h"
#include "data_xchg_space.h"
#include "utils.h"
#include "rfs.h"

/**
 * The code in this file can parse the configuration of version
 * RFS_CONFIG_LAYOUT_VERSION
 */
#define RFS_CONFIG_LAYOUT_VERSION   1U

/**
 * At a time we read up to MAX_RFS_DIR_ENTRIES_TO_READ entries from the
 * directory. This is because we have limited memory.
 */
#define MAX_RFS_DIR_ENTRIES_TO_READ 5U

/**
 * load_configuration_section() reads the configuration section from flash into
 * memory pointed by 'buffer'. Once the configuration section is read, the
 * function validates the contents of the configuration ane returns the status
 * of the checks.
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param buffer is the pointer to the memory where the configuration section
 *                will be loaded.
 * @param buffer_size is the size of the buffer in bytes. It should be at least
 *                    sizeof(struct rfs_config).
 *
 * @return true if the configuration section is loaded and validated
 * successfully, false if there is an error in reading or validating the
 * configuration
 */
bool load_configuration_section(void    *ospi_handle,
								void    *buffer,
								uint32_t buffer_size)
{
	struct rfs_config *config;

	/**
	 * Once sanity checks are done, read the configuration section from the
	 * flash in the buffer and then validate the contents of the configuration
	 * such as:
	 * # Signature
	 * # Layout version support
	 * # CRC if present
	 */
	GARD__DBG_ASSERT(
		ospi_handle != NULL && buffer != NULL &&
			buffer_size >= sizeof(struct rfs_config),
		"Invalid parameters provided to load_configuration_section");

	GARD__ASSERT(ospi_read_from_flash(
					 ospi_handle, buffer, RFS_CONFIG_START_ADDR,
					 sizeof(struct rfs_config)) == sizeof(struct rfs_config),
				 "Failed to read configuration section from flash");

	config = (struct rfs_config *)buffer;

	if (config->signature != RFS_CONFIG_SIGNATURE ||
		config->layout_version != RFS_CONFIG_LAYOUT_VERSION) {
		return false;
	}

	if (config->rfs_control & RCC_CRC_VALID) {
		uint32_t original_crc = config->optional_crc;
		uint32_t calculated_crc;

		/**
		 * To validate the CRC first reset the CRC field with 0's and then
		 * calculate the CRC over the complete structure. The calculated CRC
		 * value should match the value which was present before in the
		 * configuration section.
		 */
		config->optional_crc = 0;
		calculated_crc =
			calculate_checksum((uint8_t *)buffer, sizeof(struct rfs_config));
		config->optional_crc = original_crc;
		if (calculated_crc != config->optional_crc) {
			/* CRC mismatch. */
			return false;
		}
	}

	return true;
}

/**
 * locate_module_id() locates the module in the directory stored in flash memory
 * and returns the start address and size of the module.
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param module_uid is the unique identifier of the module to be located.
 * @param module_absl_flash_addr pointer to be filled with the flash address of
 * the module. This is an absolute flash address from start of flash memory.
 * @param module_size pointer to be filled with the size of the module.
 *
 * @return true if the module is located successfully, false otherwise.
 */
bool locate_module_id(void     *ospi_handle,
					  uint32_t  module_uid,
					  uint32_t *module_absl_flash_addr,
					  uint32_t *module_size)
{
	struct rfs_config     config;
	struct rfs_dir_entry  dir_buffer[MAX_RFS_DIR_ENTRIES_TO_READ];
	const uint32_t        dir_buffer_size = sizeof(dir_buffer);

	struct rfs_dir_entry *dir_entry_in_buffer;
	struct rfs_dir_entry *dir_entry_in_flash;
	uint32_t              dir_data_to_process;
	uint32_t              dir_bytes_read;

	/**
	 * First read the configuration in a local variable and get the directory
	 * information. Use this directory information to locate the module
	 * UID in the directory. On finding the module UID, return the start address
	 * and the size of the module in the Flash.
	 */

	GARD__DBG_ASSERT(ospi_handle != NULL && module_absl_flash_addr != NULL &&
						 module_size != NULL,
					 "Invalid parameters provided to locate_module_id");

	/* Load and validate configuration section to get directory details. */
	if (!load_configuration_section(ospi_handle, &config, sizeof(config))) {
		/* Failed to load configuration section. */
		return false;
	}

	/* Set markers for directory boundaries. */
	dir_entry_in_flash  = (struct rfs_dir_entry *)(config.start_of_directory +
                                                  RFS_CONFIG_START_ADDR);

	dir_data_to_process = config.directory_size;

	do {
		/* Buffer address to read the directory from flash. */
		dir_entry_in_buffer = dir_buffer;

		/* Determine how many bytes to read. */
		dir_bytes_read      = MIN(dir_data_to_process, dir_buffer_size);

		/* Read directory entries from flash into buffer. */
		if (ospi_read_from_flash(ospi_handle, (uint8_t *)dir_entry_in_buffer,
								 (uint32_t)dir_entry_in_flash,
								 dir_bytes_read) != dir_bytes_read) {
			/* Failed to read directory entries from flash. */
			return false;
		}

		/* Move the pointer and reduce the count for next iteration. */
		dir_data_to_process -= dir_bytes_read;
		dir_entry_in_flash =
			(struct rfs_dir_entry *)(((uint8_t *)dir_entry_in_flash) +
									 dir_bytes_read);

		/* Check if module UID is present in any of the directory entries. */
		while (dir_bytes_read) {
			if (dir_entry_in_buffer->uid == module_uid) {
				/* Module FOUND. Return the module information. */
				*module_absl_flash_addr = dir_entry_in_buffer->start_addr +
										  config.start_of_directory +
										  RFS_CONFIG_START_ADDR;
				*module_size = dir_entry_in_buffer->size;

				return true;
			}

			dir_bytes_read -= config.directory_entry_size;
			dir_entry_in_buffer++;
		}

	} while (dir_data_to_process > 0);

	return false;
}

/**
 * read_module_from_rfs() loads the module with the specified UID from Flash
 * memory into the provided buffer. If the read request will overflow the module
 * size then the read transaction is truncated to fill only the data within the
 * module.
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param module_uid is the unique identifier of the module to be loaded.
 * @param module_read_offset is the offset from the start of the module to be
 * 							 read. module_read_offset should be 32-bit aligned.
 * @param read_bytes is the number of bytes to be read from the module.
 * 							 read_bytes should be multiple of 4.
 * @param buffer is the pointer to the memory where the module will be loaded.
 *
 * @return count of bytes of the module read from flash into the buffer. In case
 * 		   of error returns 0.
 */
uint32_t read_module_from_rfs(void    *ospi_handle,
							  uint32_t module_uid,
							  uint32_t module_read_offset,
							  uint32_t read_bytes,
							  void    *buffer)
{
	uint32_t module_flash_addr;
	uint32_t module_size;
	uint32_t read_addr;
	uint32_t bytes_available;

	/**
	 * First locate the module in the RFS and get the flash address and size.
	 * Next read the module from the flash memory into the provided buffer.
	 */

	GARD__DBG_ASSERT(ospi_handle != NULL && buffer != NULL && read_bytes != 0,
					 "Invalid parameters provided to read_module_from_rfs");

	GARD__DBG_ASSERT(module_read_offset % sizeof(uint32_t) == 0 &&
						 read_bytes % sizeof(uint32_t) == 0 &&
						 (uint32_t)buffer % sizeof(uint32_t) == 0,
					 "module_read_offset, read_bytes and buffer must be "
					 "aligned to 4 bytes in read_module_from_rfs");

	if (locate_module_id(ospi_handle, module_uid, &module_flash_addr,
						 &module_size) == false) {
		/* Module NOT FOUND. */
		return 0U;
	}

	if (module_read_offset >= module_size) {
		/* Offset beyond end of module data -- nothing to read. */
		return 0U;
	}

	/* Calculate the maximum readable size from offset, trim to buffer_size */
	bytes_available = module_size - module_read_offset;

	/* Copy only data that is present in the module. */
	read_bytes      = MIN(read_bytes, bytes_available);

	/* Flash address to read from should consider the read offset. */
	read_addr       = (uint32_t)module_flash_addr + module_read_offset;

	/* Read the module from flash to buffer. */
	if (ospi_read_from_flash(ospi_handle, buffer, read_addr, read_bytes) !=
		read_bytes) {
		return 0U;
	}

	return read_bytes;
}
