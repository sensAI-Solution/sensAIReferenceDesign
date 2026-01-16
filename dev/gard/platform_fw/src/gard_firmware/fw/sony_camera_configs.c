/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "i2c_master.h"
#include "camera_config.h"
#include "sony_camera_configs.h"
#include "hw_regs.h"
#include "fw_globals.h"

/**
 * This enum image_sensor_address_size defines the size of address of
 * registers (in bytes) for sony image sensors
 */
enum image_sensor_address_size {
	IMAGE_SENSOR_SONY_IMX_219_ADDR_SIZE = 2U  // Address size for Sony IMX_219
};

/**
 * This enum image_sensor_model_id_size defines the size of model ID
 * (in bytes) of sony image sensors
 */
enum image_sensor_model_id_size {
	IMAGE_SENSOR_SONY_IMX_219_MODEL_ID_SIZE =
		2U  // Model ID size for Sony IMX_219
};

/**
 * This enum image_sensor_model_id_register_address defines the address of
 * register to read Model Id of sony image sensors
 */
enum image_sensor_model_id_register_address {
	IMAGE_SENSOR_SONY_IMX_219_MODEL_ID_REGISTER_ADDRESS =
		0x0000U  // Model ID register address for Sony IMX_219
};

/**
 * This enum image_sensor_model_id defines the model id value of sony image
 * sensors
 */
enum image_sensor_model_id {
	IMAGE_SENSOR_SONY_IMX_219_MODEL_ID = 0x0219U  // Model ID for Sony IMX_219
};

#define MAX_CAMERA_DETECTION_RETRY_COUNT 5U

/**
 * detect_image_sensor() is used to detect the image sensor by reading
 * the model ID register over I2C. This function is used specifically for
 * Sony image sensors.
 *
 * @param image_sensor_id I2C address for image sensor.
 * @param p_i2cm is i2c master instance.
 *
 * @return detection result for sony image sensors.
 */
bool detect_image_sensor(uint16_t image_sensor_id, struct i2cm_instance *p_i2cm)
{
	uint8_t  buffer[IMAGE_SENSOR_SONY_IMX_219_MODEL_ID_SIZE];
	uint16_t model_id_register_addr;
	/*<TBD-KDB> Loop variable added for delay. Shall be replaced with Delay
	 * function */
	uint32_t delay_counter;
	uint32_t retry_count = 0U;

	/**
	 * The function executes the following steps:
	 * 1. Send a I2C read request to the image sensor to read the model ID
	 * register.
	 * 2. Compare the read model ID with the expected model ID.
	 * 3. If the model ID matches, return true. Otherwise, return false.
	 */

	GARD__DBG_ASSERT((image_sensor_id < IMAGE_SENSOR_SONY_MAX_ID) &&
						 (NULL != p_i2cm),
					 "Invalid parameters provided to detect_image_sensor");

	if (IMAGE_SENSOR_SONY_IMX_219 == image_sensor_id) {
		/**
		 * For Sony IMX_219, the model ID register is at address 0x0000.
		 * The model ID is 0x0219.
		 */
		model_id_register_addr =
			IMAGE_SENSOR_SONY_IMX_219_MODEL_ID_REGISTER_ADDRESS;

		while (retry_count < MAX_CAMERA_DETECTION_RETRY_COUNT) {
			/* Delay of ~5ms to allow Camera power up */
			delay_counter = 0x5000U;
			while (0U != delay_counter) {
				delay_counter--;
			}

			/* Read the Model ID register */
			i2c_master_write(p_i2cm, image_sensor_id,
							 IMAGE_SENSOR_SONY_IMX_219_ADDR_SIZE,
							 (uint8_t *)&model_id_register_addr);
			i2c_master_read(p_i2cm, image_sensor_id,
							IMAGE_SENSOR_SONY_IMX_219_MODEL_ID_SIZE, buffer);

			if (IMAGE_SENSOR_SONY_IMX_219_MODEL_ID ==
				((buffer[0] << 8) | buffer[1])) {
				camera_started = true;
				return true;  // Detected Sony IMX_219
			}

			retry_count++;
		}

	} else {
		GARD__DBG_ASSERT(false, "Unsupported image sensor ID: %u",
						 image_sensor_id);
	}

	return false;
}

#if defined(TEST_AUTO_EXPOSURE)
const uint32_t MAX_EXPOSURE_SUPPORTED_BY_CAMERA = 0x90;
#else
const uint32_t MAX_EXPOSURE_SUPPORTED_BY_CAMERA = 0x20;
#endif
const uint32_t MIN_EXPOSURE_SUPPORTED_BY_CAMERA = 0;

#define DEFAULT_EXPOSURE_VALUE 6U

/**
 * set_exposure() is used to adjust the Sony IMX219 camera's exposure gain
 * control by calculating the optimal exposure in one step using proportional
 * control based on the full gray scale range (0-255).
 *
 * @param target_gray_avg is the target gray average value.
 *
 * @return None
 */
void set_exposure(uint32_t target_gray_avg)
{
	static uint32_t current_exposure     = DEFAULT_EXPOSURE_VALUE;
	uint32_t        new_exp_value        = current_exposure;
	uint8_t         command_to_camera[3] = {0x01, 0x5A, 0x00};
	int32_t         dist_from_target;

	GARD__DBG_ASSERT(target_gray_avg >= MIN_GRAY_AVERAGE_SUPPORTED_BY_ISP &&
						 target_gray_avg <= MAX_GRAY_AVERAGE_SUPPORTED_BY_ISP,
					 "Invalid target_gray_avg: %u", target_gray_avg);

#if defined(USE_PROPORTIONAL_CONTROL_FOR_EXPOSURE)
	/**
	 * Use basic proportional control to calculate new exposure value based on
	 * the range of exposure supported by the camera and the range of gray scale
	 * values supported by the ISP.
	 */
	new_exp_value = (MAX_EXPOSURE_SUPPORTED_BY_CAMERA * target_gray_avg) /
					MAX_GRAY_AVERAGE_SUPPORTED_BY_ISP;

	dist_from_target = dist_from_target; /* To keep the compiler happy. */
#else
	dist_from_target =
		(int32_t)get_image_gray_average() - (int32_t)target_gray_avg;

	if (dist_from_target > 50) {
		new_exp_value /= 2;
	} else if (dist_from_target > 20) {
		if (new_exp_value > 2U) {
			new_exp_value -= 2U;
		}
	} else if (dist_from_target < -30) {
		new_exp_value += 5;
	} else if (dist_from_target < -10) {
		new_exp_value += 1;
	}
#endif

	/**
	 * Clamp the new exposure value to valid range for Sony IMX219
	 * [min_exposure, max_exposure]
	 */
	if (new_exp_value > MAX_EXPOSURE_SUPPORTED_BY_CAMERA) {
		new_exp_value = MAX_EXPOSURE_SUPPORTED_BY_CAMERA;
	} else if (new_exp_value < MIN_EXPOSURE_SUPPORTED_BY_CAMERA) {
		new_exp_value = MIN_EXPOSURE_SUPPORTED_BY_CAMERA;
	}

	if (current_exposure == new_exp_value) {
		/* No change in exposure needed */
		return;
	}

	/**
	 * Send the new exposure value to the Sony IMX219 camera over I2C.
	 * The exposure register address is 0x157 and the exposure value is
	 * single byte long.
	 *
	 * *************************************************************************
	 * 	Remember the 16-bit address is sent with first byte on the wire
	 *  containing bits 7-15 and the next byte on the wire containing bits 0-7.
	 * *************************************************************************
	 */
	command_to_camera[2] = (uint8_t)(new_exp_value & 0xFFU);

	write_to_camera(IMAGE_SENSOR_SONY_IMX_219, sizeof(command_to_camera),
					command_to_camera);

	current_exposure = new_exp_value;
}
