/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef RFS_H
#define RFS_H

#include "types.h"

/**
 * This file defines the structure of the RFS (Root File System)
 * configuration and directory sections. The RFS configuration contains
 * information about the RFS Directory which defines the start and end of RFS
 * Directory. The RFS Directory contains the address and size of the modules
 * that are stored in Flash memory. The modules can be firmware, ML networks,
 * Camera Configurations, GARD Profiles stored in Flash memory.
 *
 * Below is a sample layout of the RFS configuration and directory sections with
 * references to the modules stored in Flash memory.
 *
 *  RFS Configuration Layout
 * ┌─────────────────────────────┐
 * │ Signature                   │
 * ├─────────────────────────────┤
 * │ Layout Version              │
 * ├─────────────────────────────┤
 * │ Update Count                │
 * ├─────────────────────────────┤
 * │ Optional CRC                │
 * ├─────────────────────────────┤
 * │ RFS Control                 │
 * ├─────────────────────────────┤
 * │ Start Of Directory in Flash ├───┐
 * ├─────────────────────────────┤   │
 * │ Directory Size              ├───┼─────────────────────────────────┐
 * ├─────────────────────────────┤   │                                 │
 * │ Directory Copies            │   │                                 │
 * ├─────────────────────────────┤   │                                 │
 * │ Directory Entry Size        ├───┼─────────────────┐               │
 * ├─────────────────────────────┤   │                 │               │
 * │ Load Firmware UID           │   │                 │               │
 * └─────────────────────────────┘   │                 │               │
 *                                   │                 │               │
 *                          ┌────────┘                 │               │
 *      RFS Directory       ▼                          │   ──────┐     |
 * ┌─────────────────────────────┐                     │         |     │
 * │ Module-1 UID                │                     │         |     │
 * ├─────────────────────────────┤                     │         |     │
 * │ Module-1 Start Addr         ├────┐                │         |     │
 * ├─────────────────────────────┤    │                │         |     │
 * │ Module-1 Size               │    │                │         |     │
 * └─────────────────────────────┘    │                │         |     │
 *                                    │           ──┐  │         |     │
 * ┌─────────────────────────────┐    │             │  |         |     │
 * │ Module-2 UID                │    │             │◄─┘         |     │
 * ├─────────────────────────────┤    │             │            |     │
 * │ Module-2 Start Addr         ├────┼────┐        |            |     │
 * ├─────────────────────────────┤    │    │        │            |     │
 * │ Module-2 Size               │    │    │        │            |     │
 * └─────────────────────────────┘    │    │        │            |     │
 *                                    │    │      ──┘            |     │
 * ┌─────────────────────────────┐    │    │                     |     │
 * │ Module-3 UID                │    │    │                     |     │
 * ├─────────────────────────────┤    │    │                     |     │
 * │ Module-3 Start Addr         ├────┼────┼───┐                 |     │
 * ├─────────────────────────────┤    │    │   │                 |     │
 * │ Module-3 Size               │    │    │   │                 |     │
 * └─────────────────────────────┘    │    │   │                 |     │
 *              .                     │    │   │                 |     │
 *              .                     │    │   |                 |     │
 *              .                     │    │   |                 |     │
 *              .                     │    |   |                 |◄────┘
 *              .                     │    |   |                 │
 * ┌─────────────────────────────┐    │    |   |                 │
 * │ Module-n UID                │    │    |   |                 │
 * ├─────────────────────────────┤    │    |   |                 │
 * │ Module-n Start Addr         │    │    |   |                 │
 * ├─────────────────────────────┤    │    |   |                 │
 * │ Module-n Size               │    │    |   |                 │
 * └─────────────────────────────┘    │    |   |                 │
 *                                    |    |   |           ──────┘
 *                                    |    |   |
 *                                    |    |   |
 * ┌────────────────────────┐         |    |   |
 * │Module-1                │◄────────┘    |   |
 * │ML-Network-1            │              |   |
 * └────────────────────────┘              |   |
 *                                         |   |
 * ┌────────────────────────┐              |   |
 * │Module-2                │◄─────────────┘   |
 * │Firmware                │                  |
 * └────────────────────────┘                  |
 * ┌────────────────────────┐                  |
 * │Module-3                │◄─────────────────┘
 * │ML-Network-2            │
 * └────────────────────────┘
 *
 *
 */

#define RFS_CONFIG_START_ADDR 0x300000
#define RFS_CONFIG_SIGNATURE  0x4343534CU  // "LSCC" in ASCII

enum MODULE_ID {
	/**
	 * This should not be present ideally, but is provided here if needed.
	 */
	MODULE_ID_EMPTY_ENTRY = 0,

	/**
	 * This helps identify Bad Blocks which should not be used to write data.
	 */
	MODULE_ID_BAD_BLOCK,

	/**
	 * The following IDs are used by GARD firmware's. This allows multiple
	 * firmwares to be present on the Flash with only one active firmware to
	 * boot as prescribed in the Configuration structure.
	 */
	MODULE_FW_START_ID       = 0x1001,
	MODULE_FW_END_ID         = 0x1FFE,

	/**
	 * This FW module ID can be used during development to try out different
	 * firmwares.
	 */
	MODULE_FW_TEMPORARY      = 0x1FFF,

	/**
	 * The following IDs are assigned to ML networks. This allows multiple ML
	 * networks to be present on the Flash; the App module layer instructs the
	 * FW Core to load needed ML network by providing this ID.
	 */
	MODULE_ML_START_ID       = 0x2001,
	MODULE_ML_END_ID         = 0x2FFF,

	/**
	 * The following IDs are used for hosting Camera Configurations. Each ID
	 * contains the configuration for a different camera, thus providing support
	 * for multiple cameras.
	 */
	MODULE_CAM_CFG_START_ID  = 0x3001,
	MODULE_CAM_CFG_END_ID    = 0x3FFF,

	/**
	 * The following IDs are used as Application Profiles or just Profiles.
	 * There is primarily only one profile which defines the complete hardware.
	 * The other profiles are provided in case a trimmed-down hardware is to be
	 * used for some specific use-cases.
	 */
	MODULE_APP_PROF_START_ID = 0x4001,
	MODULE_APP_PROF_END_ID   = 0x4FFF,

	/**
	 * The following IDs are currently not assigned and might be used in future.
	 */
	MODULE_UNUSED_START_ID   = 0x5001,
	MODULE_UNUSED_END_ID     = 0xFFFF,
};

/**
 * enum RFS_CONFIG_CTRL defines the control field bits in the used in
 * rfs_config.rfs_control.
 */
enum RFS_CONFIG_CTRL {
	/**
	 * This field is used to indicate if the configuration section has a valid
	 * CRC. If this bit is set, then the optional_crc field contains the CRC of
	 * the configuration section.
	 */
	RCC_CRC_VALID = (1U << 0),

	/**
	 * This field is used to indicate if the configuration section is valid.
	 * If this bit is set, then the configuration section is valid and can be
	 * used. When having redundant copies this field will be set during an
	 * upgrade to indicate that upgrade process is working on this copy and
	 * hence the contents of this copy are not valid, instead the other copy
	 * needs to be used. This situation is applicable when power is lost during
	 * upgrade which could leave the configuration section in an inconsistent
	 * state and the redundant copy needs to be used instead.
	 */
	RCC_VALID     = (1U << 1),
};

/**
 * struct rfs_config defines the structure of the configuration section in
 * Flash memory. This structure is located at RFS_CONFIG_START_ADDR in
 * flash. Routines which need to load modules from the Flash start with reading
 * this section to learn about the directory and then search the directory for
 * the Module UID which they need to load.
 */
struct rfs_config {
	/**
	 * This field contains the signature RFS_CONFIG_SIGNATURE which
	 * indicates to the parser routines the start of the configuration
	 * structure. The configuration structure acts as an anchor within the Flash
	 * for locating the rest of the data which is relatively located.
	 */
	uint32_t signature;

	/**
	 * Version of the layout of config and directory. Any changes to either of
	 * these will trigger an increment of this value.
	 */
	uint32_t layout_version;  // Version of the layout

	/**
	 * update_count tracks the number of updates to the configuration. This is
	 * used when we have a redundant copy and we need to know which copy could
	 * have the latest updates and needs to be consulted.
	 */
	uint32_t update_count;

	/***
	 * optional_crc is an optional field which can be used to store the CRC of
	 * the configuration section. This can be used to verify the integrity of
	 * the configuration section. The flag RCC_CRC_VALID if SET
	 * indicates that this variable contains a valid calculated CRC. During CRC
	 * calculation this field should be set to 0.
	 */
	uint32_t optional_crc;

	/**
	 * rfs_control is a bit-field which contains various flags related to the
	 * configuration section. The bits in this field are defined in the
	 * RFS_CONFIG_CTRL enum.
	 */
	uint32_t rfs_control;

	/**
	 * start_of_directory is the start address of the directory section in
	 * Flash memory. This is a relative address from the start of the
	 * configuration section. Since this address is relative to the start of
	 * the configuration section - the value in this field is same in both the
	 * copies thus ensuring the same CRC value exists in both the copies.
	 */
	uint32_t start_of_directory;

	/**
	 * directory_size is the size of the directory section in bytes. The count
	 * of directory entries can be calculated using the formula:
	 * directory_size/directory_entry_size
	 */
	uint32_t directory_size;

	/**
	 * directory_copies is the number of copies of the directory section in
	 * Flash memory. Normally this is one but we could have further redundancy
	 * if needed since directory is the metadata to reach the individual modules
	 * and any corruption to this section will leave us with the inability to
	 * load the modules.
	 */
	uint32_t directory_copies;

	/**
	 * directory_entry_size is the size of each directory entry in bytes.
	 * Directory format could change in future (maybe add attributes to module),
	 * in any case the layout of the existing entry should not be changed only
	 * new fields can be added. This way the old code can continue to work with
	 * the new format albeit with limitations. This field will be used by the
	 * parser of the old code to jump over the entries.
	 */
	uint32_t directory_entry_size;

	/**
	 * firmware_uid is the UID of the firmware which the firmware loader should
	 * load from Flash onto TCM.
	 * This is the only field in this structure which has nothing to do with the
	 * RFS but is provided here to avoid complicating the code and structures to
	 * host separate firmware loading configurations.
	 */
	uint32_t firmware_uid;
} __attribute__((packed));

/**
 * struct rfs_dir_entry defines the structure of a directory entry in the
 * directory section of Flash memory. Each entry contains information about a
 * module stored in Flash.
 * This directory_entry structure is used to fill the directory section. This
 * structure is different than the directory structure present within the Camera
 * Configuration section. Though they may look similar, the search code SHOULD
 * reference the correct structures to browse directory entries. This is
 * because if one structure changes, the other may not change, thus breaking the
 * code which uses them incorrectly.
 */
struct rfs_dir_entry {
	/**
	 * uid is the unique identifier of the module.
	 * Module UIDs are stored in directory for faster searches. Since the UIDs
	 * are not arranged sequentially, a linear search should be used.
	 */
	uint32_t uid;

	/**
	 * start_addr is the start address of the module in Flash memory w.r.t. the
	 * start of the directory (not the configuration section).
	 */
	uint32_t start_addr;

	/**
	 * size is the size of the module in bytes. The module is placed
	 * sequentially in the flash without any fragments.
	 */
	uint32_t size;
} __attribute__((packed));

/**
 * load_configuration_section() loads the configuration section from Flash
 * memory into the provided buffer.
 */
bool load_configuration_section(void    *ospi_handle,
								void    *buffer,
								uint32_t buffer_size);

/**
 * locate_module_id() locates the module in the directory stored in flash memory
 * and returns the start address and size of the module.
 */
bool locate_module_id(void     *ospi_handle,
					  uint32_t  module_uid,
					  uint32_t *module_absl_flash_addr,
					  uint32_t *module_size);

/**
 * read_module_from_rfs() reads the module with the specified UID from Flash
 * memory into the provided buffer starting from the specified offset and
 * reading the specified number of bytes.
 */
uint32_t read_module_from_rfs(void    *ospi_handle,
							  uint32_t module_uid,
							  uint32_t module_read_offset,
							  uint32_t read_bytes,
							  void    *buffer);

#endif /* RFS_H */

