/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "sys_platform.h"
#include "gpio.h"
#include "gpio_support.h"

/* GPIO instances */
struct gpio_instance gpio_0;
struct gpio_instance gpio_3;

/**
 * gpio_init() initializes all the GPIO instances used by the firmware.
 *
 * @param None
 *
 * @return None
 */
void gpio_init()
{
	/* Initialize the GPIO 0 instance. */
	gpio_pins_init(&gpio_0, GPIO0_INST_GPIO_MEM_MAP_BASE_ADDR,
				   GPIO0_INST_LINES_NUM, GPIO0_INST_GPIO_DIRS);

	/* Configure GPIO 0 pin 0 as output to control camera power. */
	gpio_pin_set_dir(&gpio_0, GPIO_PIN_0, GPIO_OUTPUT);

	/* Initialize GPIO 3 instance. */
	gpio_pins_init(&gpio_3, GPIO3_INST_GPIO_MEM_MAP_BASE_ADDR,
				   GPIO3_INST_LINES_NUM, GPIO3_INST_GPIO_DIRS);

	/* Configure GPIO 3 pin 22 as output to control camera power. */
	gpio_pin_set_dir(&gpio_3, GPIO_PIN_22, GPIO_OUTPUT);
}
