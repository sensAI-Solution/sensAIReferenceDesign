/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef GPIO_SUPPORT_H
#define GPIO_SUPPORT_H

#include "gard_types.h"
#include "assert.h"
#include "sys_platform.h"
#include "gpio.h"

extern struct gpio_instance gpio_0;
extern struct gpio_instance gpio_3;


/**
 * gpio_init() initializes all the GPIO instances used by the firmware.
 */
void gpio_init();

#endif /* GPIO_SUPPORT_H */
