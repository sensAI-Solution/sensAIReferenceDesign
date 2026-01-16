/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_I2C_H__
#define __HUB_I2C_H__

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include "gard_info.h"

/* Starting preamble of I2C bus string in Linux */
#define I2C_BUS_STRING_PREAMBLE "/dev/i2c-"

/**
 * Maximum size of I2C data for reading from GARD in a single
 * bulk read transaction
 *
 * This value has been found out empirically by running bulk
 * read operations on the I2C bus for various sizes and observing
 * the point at which failure was observed.
 */
#define MAX_I2C_RECV_DATA_SZ    (3 * 1024)

/**
 * Open an I2C bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 * Any further operation on the said I2C bus has to use this bus handle.
 */
int32_t hub_i2c_device_open(void *param);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 *  Perform a read operation on the I2C bus represented by the given bus handle.
 */
int32_t hub_i2c_device_read(int i2c_bus_hdl, void *p_buffer, uint32_t count);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Perform a write operation on the I2C bus represented by the given bus
 * handle.
 */
int32_t
	hub_i2c_device_write(int i2c_bus_hdl, const void *p_buffer, uint32_t count);

/**
 * TBD-DPN: Revisit the return code for this call.
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Close the I2C bus opened by the open call.
 */
int32_t hub_i2c_device_close(int i2c_bus_hdl);

#endif /* __HUB_I2C_H__ */
