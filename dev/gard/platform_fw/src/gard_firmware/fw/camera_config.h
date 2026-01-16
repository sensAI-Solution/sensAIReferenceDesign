/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include "types.h"

/**
 * Following structures cover the format 'within' the Camera Configuration
 * section.
 */

/**
 * enum CAMERA_COMMAND_ID defines the unique identifiers for different camera
 * commands. While most of them are commands that can be performed on the camera
 * a few reserved IDs are set aside to capture information about the camera.
 *
 * *** The UIDs assigned here could be the same as the UIDs in enum MODULE_ID
 * and this is fine since both enums are used in different contexts.
 */
enum CAMERA_COMMAND_ID {
	/**
	 * UIDs from 0x1001 through 0x1010 are RESERVED for hosting infmormative
	 * data. Of these 0x1001 is used to hold Basic Camera Information. Instead
	 * of modifying this structure it is advisable to use another Reserved UID
	 * to add more information. This way the Basic Configuration structure
	 * layout remains unchanged thus supporting multiple new code versions.
	 */
	CAMERA_COMMAND_BASIC_CAMERA_INFO        = 0x1001,
	CAMERA_COMMAND_VENDOR_EXTN_CAMERA_INFO  = 0x1002,

	/** CAMERA_COMMAND_CAMERA_INFO_LAST_UID marks the last UID that can be used
	 * for holding Camera Information. */
	CAMERA_COMMAND_CAMERA_INFO_LAST_UID     = 0x1010,

	/**
	 * UIDs from 0x1011 to 0xEFFF are used to represent the Camera Commands.
	 * These command codes are independent of the camera make/model.
	 * Currently listed command UIDs were added based on the commands that were
	 * known at the time of writing this file.
	 * When adding a new camera pick the existing Command UIDs for that camera.
	 * If UID does not exist for the desired command please pick the next
	 * available UID and write it here so that it becomes available to use for
	 * other cameras. Please feel free to add new commands as needed, but use
	 * the existing command UID for the same command across all cameras.
	 */
	CAMERA_COMMAND_START_CAMERA_UID         = 0x1011,
	CAMERA_COMMAND_STOP_CAMERA_UID          = 0x1012,
	CAMERA_COMMAND_CHANGE_FPS_UID           = 0x1013,
	CAMERA_COMMAND_CHANGE_WHITE_BALANCE_UID = 0x1014,
	CAMERA_COMMAND_SET_EXPOSURE_UID         = 0x1015,
	CAMERA_COMMAND_SET_AUTO_FOCUS_UID       = 0x1016,
	CAMERA_COMMAND_SET_CONTRAST_UID         = 0x1017,

	/**
	 * UIDs from 0xF000 to 0xFFFF are reserved for developer's during their
	 * development and testing with different cameras.
	 */
	CAMERA_COMMAND_START_OF_RESERVED_UID    = 0xF000,
	CAMERA_COMMAND_END_OF_RESERVED_UID      = 0xFFFF,
};

struct cc_dir_entry {
	/**
	 * uid which is unique within this camera config directory. This UID
	 * represents the camera configuration for a specific command to be
	 * performed on this camera. All the possible commands are listed in the
	 * enum CAMERA_COMMAND_ID
	 */
	uint32_t camera_command_uid;

	/**
	 * start_addr is the start address of the camera configuration in Flash
	 * memory w.r.t. the start of the camera configuration directory.
	 */
	uint32_t start_addr;

	/**
	 * size is the size of the camera configuration in bytes. The camera
	 * configuration command is placed sequentially in the flash without any
	 * fragments.
	 */
	uint32_t size;
} __attribute__((packed));

#define VENDOR_NAME_SIZE                   16
#define MODEL_NUMBER_SIZE                  16
#define DRIVER_VERSION_SIZE                16

/* camera_uid – is the unique identifier of the camera */
#define START_STREAMING_CAMERA_UID         0x3001U

/* camera_command_uid – is the unique identifier of the camera command. */
#define START_STREAMING_CAMERA_COMMAND_UID 0x1015U

/* Config record Packet count for delay insertion */
#define CONFIG_DELAY_RECORD_PACKET_COUNT   0xFFFFFFFFU

/* Default Target gray average to begin with. */
#define DEFAULT_TARGET_GRAY_AVERAGE        50U

struct basic_camera_information {
	/**
	 * layout_version is the version of the layout of the basic camera
	 * information structure. This is used to ensure that the code reading this
	 * structure is compatible with the layout of the structure.
	 */
	uint32_t layout_version;

	/**
	 * vendor_name is the name of the camera vendor. This value is not directly
	 * used by the firmware.
	 */
	uint8_t vendor_name[VENDOR_NAME_SIZE];

	/**
	 * model_number is the camera model string. This value is not directly
	 * used by the firmware.
	 */
	uint8_t model_number[MODEL_NUMBER_SIZE];

	/**
	 * driver_version is the version of the driver running on a host system
	 * which was used to extract this configuration. This helps indirectly
	 * identify the version of the configuration.
	 */
	uint8_t driver_version[DRIVER_VERSION_SIZE];

	/**
	 * Count of directory entries present in the camera configuration.
	 * This value is NOT in bytes.
	 * We do not anticipate the directory entry structure to change and hence
	 * the directory entry format is frozen and there is no need to store the
	 * directory entry size like the way it is stored in the Flash Configuration
	 * as that directory format could change in future.
	 */
	uint32_t directory_entry_count;
} __attribute__((packed));

/**
 * load_camera_command() loads the camera command configuration from the flash
 * memory based on the camera UID and action UID. This camera command
 * configuration is loaded into the provided buffer.
 */
uint32_t load_camera_command(void    *ospi_handle,
							 uint32_t camera_uid,
							 uint32_t camera_command_uid,
							 uint8_t *buffer,
							 uint32_t buffer_size);

/**
 * start_camera_streaming() initializes the camera with the camera config which
 * is located on flash memory.
 */
void start_camera_streaming(void);

/**
 * run_auto_exposure() is used to tune the camera's exposure gain control for
 * target grey scale value.
 */
void run_auto_exposure(void);

/**
 * get_image_gray_average() reads the gray average value from the ISP capture
 * registers and returns the value to the caller.
 */
uint32_t get_image_gray_average(void);

/**
 * set_target_gray_average() sets the target gray average value for auto
 * exposure algorithm.
 */
uint32_t set_target_gray_average(uint32_t gray_average);

/**
 * write_to_camera() writes the given data buffer to the camera over I2C bus.
 */
void write_to_camera(uint16_t image_sensor_id,
					 uint32_t byte_count,
					 uint8_t *p_data_buffer);

/**
 * setup_misp_config() programs the MOD mini-ISP configuration.
 * This is invoked during initialization.
 */
void setup_misp_config(void);

#endif /* CAMERA_CONFIG_H */
