/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef GPIO_MAPPER_H
#define GPIO_MAPPER_H

/**
 * This file provides mapping of GPIO pins to their connected destinations. Only
 * the mapping of the destinations that are used for communication with the
 * destination devices are provided in this file. The rest of the GPIO pins that
 * are used for debugging are not listed here. It is important that the GPIO
 * pins used for debugging DO NOT conflict with the GPIO pins referenced here.
 */

#include "gard_types.h"
#include "assert.h"
#include "gpio.h"
#include "gpio_support.h"

/**
 * Following are some of the mappings of GPIO pins to their connected
 * destinations.
 * Helper macros are provided for the firmware to use these GPIO pins instead
 * of using the GPIO pin numbers directly.
 */

#define GPIO_INST_CONNECTED_TO_CAMERA gpio_0
#define GPIO_PIN_TO_CAMERA_POWER      GPIO_PIN_0

#define GPIO_INST_CONNECTED_TO_HOST   gpio_3
#define GPIO_PIN_TO_HOST_IRQ          GPIO_PIN_22

#define SET_GPIO_HIGH_TO_CAMERA_POWER() \
	gpio_pin_write(&GPIO_INST_CONNECTED_TO_CAMERA, \
				   GPIO_PIN_TO_CAMERA_POWER, GPIO_OUTPUT_HIGH)

#define SET_GPIO_LOW_TO_CAMERA_POWER()  \
	gpio_pin_write(&GPIO_INST_CONNECTED_TO_CAMERA, \
				   GPIO_PIN_TO_CAMERA_POWER, GPIO_OUTPUT_LOW)

#define SET_GPIO_HIGH_TO_HOST_IRQ() \
	gpio_pin_write(&GPIO_INST_CONNECTED_TO_HOST, \
				   GPIO_PIN_TO_HOST_IRQ, GPIO_OUTPUT_HIGH)

#define SET_GPIO_LOW_TO_HOST_IRQ()  \
	gpio_pin_write(&GPIO_INST_CONNECTED_TO_HOST, \
				   GPIO_PIN_TO_HOST_IRQ, GPIO_OUTPUT_LOW)

#endif /* GPIO_MAPPER_H */
