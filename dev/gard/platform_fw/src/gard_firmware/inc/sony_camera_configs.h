/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef SONY_CAMERA_CONFIG_H
#define SONY_CAMERA_CONFIG_H

#include "gard_types.h"

/**
 * This enum image_sensor_id defines the I2C address for image sensors
 */
enum image_sensor_id {
	IMAGE_SENSOR_SONY_IMX_219 = 0x10U,  // IMAGE SENSOR SONY IMX_219 = 0x10,
	IMAGE_SENSOR_SONY_MAX_ID  = 0xFF
};

/**
 * detect_image_sensor() is used to detect the image sensor by reading
 * the model ID register over I2C. This function is used specifically for
 * Sony image sensors.
 *
 * @return detection result for sony image sensors.
 */
bool detect_image_sensor(uint16_t              image_sensor_id,
						 struct i2cm_instance *p_i2cm);

/**
 * set_exposure() is used to adjust the camera's exposure gain control
 * by calculating the optimal exposure based on the camera exposure range.
 *
 * @param target_gray_avg is the target gray average value (0-255)
 */
void set_exposure(uint32_t target_gray_avg);

#endif /* SONY_CAMERA_CONFIG_H */
