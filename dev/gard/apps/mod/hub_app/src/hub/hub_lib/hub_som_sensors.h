/******************************************************************************
 * Copyright (c) 2024 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_SENSORS_H__
#define __HUB_SENSORS_H__

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "hub.h"
#include "hub_globals.h"
#include "gard_info.h"

/* TBD-DPN: To handle 'invalid' sensor values if possible */
/* Please refer the variable of the same name in hub.py, They need to be in sync. */
#define HUB_INVALID_SENSOR_VALUE         (-1.0f)

/* 0xFF is an invalid I2C slave address */
#define HUB_INVALID_SENSOR_ID            (0xFF)

/**
 * TBD-DPN: This mapping will be read in from the app_profile
 * in subsequent releases.
 *
 * Sensor ID to I2C Slave address mappings for temperature sensors
 */
#define HUB_SOM_TEMP_SENSOR_1_SLAVE_ID   0x48
#define HUB_SOM_TEMP_SENSOR_2_SLAVE_ID   0x49

/**
 * SOM has TMP118 sensors for temperature measurement.
 *
 * These values are obtained from the datasheet for the TMP118 sensor
 * Link: https://www.ti.com/product/TMP118
 */
/* Register addresses for the sensor */
#define TMP118_REG_TEMP                  0x00  // Temperature Register
#define TMP118_REG_CONF                  0x01  // Configuration Register
#define TMP118_REG_STAT                  0x02  // Status Register

#define TMP118_MULTIPLIER                (0.0078125f)

/**
 * TBD-DPN: This mapping will be read in from the app_profile
 * in subsequent releases.
 *
 * Sensor ID to I2C Slave address mappings for energy sensors
 */
#define HUB_SOM_ENERGY_SENSOR_1_SLAVE_ID 0x42
#define HUB_SOM_ENERGY_SENSOR_2_SLAVE_ID 0x43

/**
 * SOM has INA236 sensors for energy measurement.
 *
 * These values are obtained from the datasheet for the INA236 sensor
 * Link: https://www.ti.com/product/INA236
 */
/* Register addresses for the sensor */
#define INA236_REG_CONFIG                0x00
#define INA236_REG_BUS_VOLTAGE           0x02
#define INA236_REG_POWER                 0x03
#define INA236_REG_CURRENT               0x04
#define INA236_REG_CALIB                 0x05  // Calibration register

#define SOM_MAX_CURRENT                  (10.0f)
#define SOM_SHUNT_REG                    (0.01f)
#define INA236_CURRENT_DIV               (32768.0f)

#define INA236_CURRENT_MULTIPLIER        (0.00512f)
#define INA236_VOLTAGE_MULTIPLIER        (0.00160f)
#define INA236_POWER_MULTIPLIER          (32.0f)

#endif /* __HUB_SENSORS_H__ */