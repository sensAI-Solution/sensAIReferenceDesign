/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "types.h"
#include "assert.h"
#include "rfs.h"
#include "utils.h"
#include "camera_config.h"
#include "ospi_support.h"
#include "sys_platform.h"
#include "i2c_master.h"
#include "sony_camera_configs.h"
#include "gpio.h"
#include "gpio_mapper.h"
#include "hw_regs.h"
#include "fw_globals.h"

/**
 * The code in this file can parse the camera configuration of version
 * CAMERA_CONFIG_LAYOUT_VERSION
 */
#define CAMERA_CONFIG_LAYOUT_VERSION 1U

/**
 * At a time we read up to MAX_CC_DIR_ENTRIES_TO_READ entries from the
 * directory. This is because we have limited memory.
 */
#define MAX_CC_DIR_ENTRIES_TO_READ   5U

/**
 * Buffer size used to hold the camera configuration data.
 */
#define CAMERA_CONFIG_BUFFER_SIZE    512U

/**
 * Delay count for 1 ms at 50 MHz clock frequency.
 * considering 50Mhz clock, 12 cycles for 1 iteration. Approximately ~5k
 * iterations for ~1 milliseconds delay
 */
#define DELAY_COUNT_1_MS_50_MHZ      0x1000U

/**
 * This structure image_sensor_cfg_data_record is used to hold all the variables
 * related to handling the image sensor configuration data transmission over
 * I2C.
 */
struct image_sensor_cfg_data_record {
	// Data size of configuration record in bytes.
	uint32_t data_packet_count;

	// Size of address + data field in bytes.
	uint8_t data_granularity;
} __attribute__((packed));

/**
 * This structure cam_i2cm is used to specify the I2C master instance
 * that is used to configure the image sensor.
 */
struct i2cm_instance cam_i2cm;

/**
 * configure_image_sensor configures image sensor over I2C.
 * ==============================================================================
 *                              BUFFER ARCHITECTURE
 * ==============================================================================
 *
 * LEVEL 1: COMPLETE CONFIGURATION BUFFER
 * +=========================================================================+
 * |                        Configuration Buffer                             |
 * |                          (byte_count bytes)                             |
 * +=========================================================================+
 * |                                                                         |
 * | LEVEL 2: RECORD STRUCTURE                                               |
 * | +---------------------+---------------------+-------------------------+ |
 * | |     Record #1       |     Record #2       |         ...             | |
 * | |    (Variable Size)  |    (Variable Size)  |                         | |
 * | +---------------------+---------------------+-------------------------+ |
 * |                                                                         |
 * | LEVEL 3: SINGLE RECORD BREAKDOWN                                        |
 * | +-----------------+-----------------------------------------------+     |
 * | | Record Header   |              I2C Data Packets                |      |
 * | |   (5 bytes)     |         (packet_count × granularity)         |      |
 * | +-----------------+-----------------------------------------------+     |
 * |                                                                         |
 * | LEVEL 4: HEADER BYTE STRUCTURE                                          |
 * | +--------+--------+--------+--------+--------+                          |
 * | | Byte 0 | Byte 1 | Byte 2 | Byte 3 | Byte 4 |                          |
 * | +--------+--------+--------+--------+--------+                          |
 * | |<------- data_packet_count ------->|gran.   |                          |
 * | |      (uint32_t / 4 bytes)         |1 byte  |                          |
 * |                                                                         |
 * | LEVEL 5: I2C PACKETS STRUCTURE                                          |
 * | +-------------+-------------+-------------+---------------------------+ |
 * | | I2C Packet1 | I2C Packet2 | I2C Packet3 |           ...             | |
 * | |(granularity)|(granularity)|(granularity)|                           | |
 * | |   bytes     |   bytes     |   bytes     |                           | |
 * | +-------------+-------------+-------------+---------------------------+ |
 *
 *  * CONCRETE EXAMPLE WITH BYTE-LEVEL DETAIL:
 * +=========================================================================+
 * | Buffer: 155 bytes total (Sony IMX219 initialization)                    |
 * +=========================================================================+
 * |                                                                         |
 * | RECORD #1: Basic Setup (155 bytes)                                      |
 * | +-------+-----------------------------------------------------------+   |
 * | |Header | I2C Data Packets (150 bytes)                             |    |
 * | |(5B)   | 50 packets × 3 bytes each                                |    |
 * | +-------+-----------------------------------------------------------+   |
 * |                                                                         |
 * | Header Bytes (Offset 0-4):                                              |
 * | +----+----+----+----+----+                                              |
 * | |0x32|0x00|0x00|0x00|0x03|  = packet_count=50, granularity=3            |
 * | +----+----+----+----+----+                                              |
 * |   0    1    2    3    4                                                 |
 * |                                                                         |
 * | First I2C Packet (Offset 5-7):                                          |
 * | +----+----+----+                                                        |
 * | |0x30|0x12|0x34|  = Write 0x1234 to register 0x30                       |
 * | +----+----+----+                                                        |
 * |   5    6    7                                                           |
 * |                                                                         |
 * | Second I2C Packet (Offset 8-10):                                        |
 * | +----+----+----+                                                        |
 * | |0x31|0x56|0x78|  = Write 0x5678 to register 0x31                       |
 * | +----+----+----+                                                        |
 * |   8    9   10                                                           |
 * |                                                                         |
 * | ... (continue for all 50 packets) ...                                   |
 * |                                                                         |
 * +=========================================================================+
 * | Buffer: 9 bytes total (Special header #1 : delay insertion)             |
 * +=========================================================================+
 * | RECORD #1: Basic Setup (9 bytes)                                        |
 * | +-------+-----------------------------------------------------------+   |
 * | |Header | I2C Data Packets (4 bytes)                               |    |
 * | |(5B)   | 1 packets × 4 bytes each                                 |    |
 * | +-------+-----------------------------------------------------------+   |
 * |                                                                         |
 * | Header Bytes (Offset 0-4):                                              |
 * | +----+----+----+----+----+                                              |
 * | |0xFF|0xFF|0xFF|0xFF|0x04|  = packet_count=0xFFFFFFFF, granularity=4    |
 * | +----+----+----+----+----+                                              |
 * |   0    1    2    3    4                                                 |
 * |                                                                         |
 * | First I2C Packet (Offset 5-8):                                          |
 * | +----+----+----+----+                                                   |
 * | |0x0A|0x00|0x00|0x00|  = Delay of 10 milliseconds                       |
 * | +----+----+----+----+                                                   |
 * |   5    6    7    8                                                      |
 * |                                                                         |
 * +=========================================================================+
 *
 * @param image_sensor_id I2C address for image sensor.
 * @param byte_count is the size of p_data_buffer in bytes.
 * @param p_data_buffer is the buffer holding configuration.
 *
 * @return true if the configuration is successful, false otherwise.
 */
static bool configure_image_sensor(uint16_t image_sensor_id,
								   uint32_t byte_count,
								   uint8_t *p_data_buffer)
{
	uint32_t                             config_data_idx;
	uint32_t                             config_idx = 0;
	struct image_sensor_cfg_data_record *p_config_record;
	uint8_t                             *p_config_data;
	uint32_t                             delay_counter;

	GARD__DBG_ASSERT((byte_count > 0U) && (NULL != p_data_buffer),
					 "Invalid configuration parameters");

	if (!detect_image_sensor(image_sensor_id, &cam_i2cm)) {
		return false;
	}

	while (config_idx < byte_count) {
		p_config_record =
			(struct image_sensor_cfg_data_record *)(p_data_buffer + config_idx);
		config_idx    += sizeof(struct image_sensor_cfg_data_record);
		p_config_data  = p_data_buffer + config_idx;

		for (config_data_idx = 0U;
			 config_data_idx < p_config_record->data_packet_count *
								   p_config_record->data_granularity;
			 config_data_idx += p_config_record->data_granularity) {
			if (CONFIG_DELAY_RECORD_PACKET_COUNT ==
				p_config_record->data_packet_count) {
				/* Setup a counter for delay equivalent in milliseconds */
				delay_counter = (*p_config_data * DELAY_COUNT_1_MS_50_MHZ);

				while (0U != delay_counter) {
					delay_counter--;
				}

				/* Encountered a special config record for delay insertion
				 * contains a single packet of sizeof(uint32_t) / 4 bytes with
				 * delay count in milliseconds */
				p_config_record->data_packet_count = 1U;
			} else {
				write_to_camera(image_sensor_id,
								p_config_record->data_granularity,
								p_config_data + config_data_idx);
			}
		}
		config_idx += p_config_record->data_packet_count *
					  p_config_record->data_granularity;
	}

	return true;
}

/**
 * write_to_camera() writes the camera configuration to the image sensor over
 * I2C. This call can be made only after configure_image_sensor() has been
 * called.
 *
 * @param image_sensor_id I2C address for image sensor.
 * @param byte_count is the size of p_data_buffer in bytes.
 * @param p_data_buffer is the buffer holding configuration.
 *
 * @return Nothing
 */
void write_to_camera(uint16_t image_sensor_id,
					 uint32_t byte_count,
					 uint8_t *p_data_buffer)
{
	GARD__DBG_ASSERT(0U == i2c_master_write(&cam_i2cm, image_sensor_id,
											(uint8_t)byte_count, p_data_buffer),
					 "I2C write failed for image sensor %u", image_sensor_id);
}

/**
 * load_camera_command() loads the camera command configuration from the flash
 * memory based on the camera UID and action UID. The function first loads the
 * camera configuration by locating it using camera_uid. It then parses this
 * camera configuration to load the camera command using the camera_command_uid
 *
 * @param ospi_handle is the OSPI controller handle.
 * @param camera_uid is the unique identifier of the camera.
 * @param camera_command_uid is the unique identifier of the camera command.
 * @param buffer is the pointer to the memory where the camera command
 * 				 configuration is written by the function.
 * @param buffer_size is the size of the buffer in bytes.
 *
 * @return the byte count of the camera command configuration loaded in memory.
 * 			0 if the function fails.
 */
uint32_t load_camera_command(void    *ospi_handle,
							 uint32_t camera_uid,
							 uint32_t camera_command_uid,
							 uint8_t *buffer,
							 uint32_t buffer_size)
{
	uint32_t                        flash_addr;
	struct basic_camera_information basic_camera_info;
	uint32_t                        camera_config_size;
	struct cc_dir_entry             cc_dir_buffer[MAX_CC_DIR_ENTRIES_TO_READ];
	const uint32_t                  cc_dir_buffer_size = sizeof(cc_dir_buffer);
	struct cc_dir_entry            *cc_dir_entry_in_buffer;
	struct cc_dir_entry            *cc_dir_entry_in_flash;
	uint32_t                        cc_dir_start_addr_in_flash;
	uint32_t                        cc_dir_data_to_process;
	uint32_t                        cc_dir_bytes_read;
	bool                            cc_command_found = false;
	uint32_t                        camera_command_bytes_read;

	/**
	 * The following sequence of operations is performed:
	 * 1. Load the camera configuration section from flash.
	 * 2. Locate the camera command UID within the camera configuration
	 *    directory.
	 * 3. Read the camera command configuration from flash into the provided
	 *    buffer.
	 * 4. Return the size of the camera command configuration loaded in the
	 *    buffer.
	 */
	GARD__DBG_ASSERT(NULL != ospi_handle && NULL != buffer && 0 != buffer_size,
					 "Invalid parameters provided to load_camera_command");

	/* Locate camera configuration in flash */
	if (locate_module_id(ospi_handle, camera_uid, &flash_addr,
						 &camera_config_size) == false) {
		/* Camera configurataion NOT FOUND. */
		return 0;
	}

	/**
	 * The camera configuration directory is the first thing present within the
	 * camera configuration module. Scan it to locate the camera command UID.
	 * Read the camera config directory in small chunks in memory for searching
	 * to minimize memory usage.
	 */

	/* Set markers for directory boundaries. */
	cc_dir_start_addr_in_flash = (uint32_t)flash_addr;
	cc_dir_entry_in_flash = (struct cc_dir_entry *)cc_dir_start_addr_in_flash;

	/**
	 * Read the first entry which points to the basic configuration information.
	 * This information also contains the count of directory entries in this
	 * camera configuration. We need this so that we do not search out of the
	 * directory boundaries.
	 */
	if (ospi_read_from_flash(ospi_handle, (uint8_t *)cc_dir_buffer,
							 (uint32_t)cc_dir_entry_in_flash,
							 sizeof(struct cc_dir_entry)) !=
		sizeof(struct cc_dir_entry)) {
		/* Failed to read first directory entry from flash. */
		return 0;
	}

	/**
	 * Validate the first entry has the Basic Camera Information UID.
	 * If it does not then we cannot proceed as we do not know the directory
	 * length.
	 */
	if (CAMERA_COMMAND_BASIC_CAMERA_INFO !=
		cc_dir_buffer[0].camera_command_uid) {
		/* Invalid camera configuration directory. */
		return 0;
	}

	/**
	 * Read the basic configuration structure from Flash to find the directory
	 * entry count.
	 */
	if (ospi_read_from_flash(
			ospi_handle, (uint8_t *)&basic_camera_info,
			(uint32_t)cc_dir_buffer[0].start_addr + cc_dir_start_addr_in_flash,
			sizeof(basic_camera_info)) != sizeof(basic_camera_info)) {
		/* Failed to read first directory entry from flash. */
		return 0;
	}

	/**
	 * Total directory (maximum) bytes to search. Deduct one entry as that one
	 * points to the basic information structure which we are done with.
	 */
	cc_dir_data_to_process = (basic_camera_info.directory_entry_count - 1) *
							 sizeof(struct cc_dir_entry);
	cc_dir_entry_in_flash++;

	do {
		/* Buffer address to read the directory from flash. */
		cc_dir_entry_in_buffer = cc_dir_buffer;

		/* Determine how many bytes to read. */
		cc_dir_bytes_read = MIN(cc_dir_data_to_process, cc_dir_buffer_size);

		/* Read directory entries from flash into buffer. */
		if (ospi_read_from_flash(ospi_handle, (uint8_t *)cc_dir_entry_in_buffer,
								 (uint32_t)cc_dir_entry_in_flash,
								 cc_dir_bytes_read) != cc_dir_bytes_read) {
			/* Failed to read directory entries from flash.*/
			return 0;
		}

		/* Move the pointer and reduce the count for next iteration. */
		cc_dir_data_to_process -= cc_dir_bytes_read;
		cc_dir_entry_in_flash =
			(struct cc_dir_entry *)(((uint8_t *)cc_dir_entry_in_flash) +
									cc_dir_bytes_read);

		/* Search for the camera command UID. */
		while (cc_dir_bytes_read) {
			if (cc_dir_entry_in_buffer->camera_command_uid ==
				camera_command_uid) {
				/* Camera Command Configuration FOUND. */
				cc_command_found = true;
				break;
			}

			cc_dir_bytes_read -= sizeof(struct cc_dir_entry);
			cc_dir_entry_in_buffer++;
		}

	} while ((cc_dir_data_to_process > 0) && (!cc_command_found));

	if (!cc_command_found) {
		/* Camera Command Configuration NOT FOUND. */
		return 0;
	}

	camera_command_bytes_read = MIN(cc_dir_entry_in_buffer->size, buffer_size);

	/* Read the camera configuration command from flash into buffer. */
	if (ospi_read_from_flash(
			ospi_handle, buffer,
			cc_dir_entry_in_buffer->start_addr + cc_dir_start_addr_in_flash,
			camera_command_bytes_read) != camera_command_bytes_read) {
		/* Failed to read camera configuration command from flash.*/
		return 0;
	}

	return camera_command_bytes_read;
}

/**
 * start_camera_streaming() loads the camera command configuration from the
 * flash memory based on the camera UID and action UID into local buffer. The
 * function then loads the retrieved camera configuration by sending it to the
 * camera using I2C bus.
 *
 * @param None
 * @return None
 */
void start_camera_streaming(void)
{
	uint8_t  cam_buffer[CAMERA_CONFIG_BUFFER_SIZE];
	uint32_t camera_command_bytes_read;

	/* Set Output GPIO pin 0 for camera Power to ON */
	SET_GPIO_HIGH_TO_CAMERA_POWER();

	/* Initialize the I2C master instance for camera communication */
	(void)i2c_master_init(&cam_i2cm,
						  I2C_MST0_INST_I2C_CONTROLLER_MEM_MAP_BASE_ADDR);
	(void)i2c_master_config(&cam_i2cm, I2CM_ADDR_7BIT_MODE, INT_MODE,
							I2C_MST0_INST_PRESCALER);

	/* Retrieve camera configuration from flash */
	camera_command_bytes_read = load_camera_command(
		sd, START_STREAMING_CAMERA_UID, START_STREAMING_CAMERA_COMMAND_UID,
		cam_buffer, sizeof(cam_buffer));

	GARD__ASSERT(camera_command_bytes_read > 0U,
				 "Failed to load camera command");

	/* Configure the image sensor using the retrieved camera command */
	configure_image_sensor(IMAGE_SENSOR_SONY_IMX_219, camera_command_bytes_read,
						   cam_buffer);
}

/**
 * target_gray_average holds the target gray average value for auto exposure.
 * Change this value if this value does not agree with the exposure that the AI
 * can work with.
 */
static uint32_t target_gray_average = DEFAULT_TARGET_GRAY_AVERAGE;

/**
 * get_target_gray_average() returns the target gray average value for auto
 * exposure algorithm.
 *
 * @return target gray average value.
 */
static uint32_t get_target_gray_average(void)
{
	return target_gray_average;
}

/**
 * set_target_gray_average() sets the target gray average value for auto
 * exposure algorithm.
 *
 * @param gray_average is the target gray average value to be set.
 *
 * @return previous target gray average value.
 */
uint32_t set_target_gray_average(uint32_t gray_average)
{
	uint32_t previous_target = target_gray_average;
	target_gray_average      = gray_average;

	return previous_target;
}

/**
 * get_image_gray_average() reads the gray average value from the ISP capture
 * registers and returns the value to the caller.
 *
 * @return gray average value of the captured image as calculated by ISP.
 */
uint32_t get_image_gray_average(void)
{
	uint8_t gray_average;

	/* Read the gray average value from ISP capture statistics register */
	gray_average = (uint8_t)(GARD__CAPTURE_IMAGE_STATS__IMG_AVG);

	return (uint32_t)gray_average;
}

#if defined(GARD_DEBUG)
/**
 * auto_exposure_run_count keeps track of number of times auto exposure has
 * been run. Useful during debugging.
 */
uint32_t auto_exposure_run_count = 0;
#endif

/**
 * run_auto_exposure() is used to perform auto exposure on the camera by
 * retrieving the brightness of the captured image and then calculating the new
 * exposure gain delta to be applied to the camera.
 *
 * @return None
 */
void run_auto_exposure(void)
{
	uint32_t img_gray_average;
	uint32_t tgt_gray_average;

	GARD__DBG_ASSERT(auto_exposure_enabled, "Auto Exposure is not enabled");

	/* Get the gray average of the image from the ISP. */
	img_gray_average = get_image_gray_average();

	/* Get the target gray average for comparison. */
	tgt_gray_average = get_target_gray_average();

	/* Set the camera sensor exposure based on current and target gray averages.
	 */
	if (img_gray_average != tgt_gray_average) {
#if defined(GARD_DEBUG)
		auto_exposure_run_count++;
#endif
		set_exposure(tgt_gray_average);
	}
}

/**
 * setup_misp_config() programs the MOD mini-ISP configuration.
 * This is invoked during initialization.
 *
 * @return None
 */
void setup_misp_config(void)
{
	/**
	 * Configures the image capture system with frame buffer settings and
	 * capture features. Sets up the AXI configuration, frame buffer write base
	 * address, and enables auto white balance mode.
	 */
	GARD__SET_MISP_CONFIGS();
}
