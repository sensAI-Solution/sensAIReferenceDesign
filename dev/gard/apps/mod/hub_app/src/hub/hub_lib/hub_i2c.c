/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_i2c.h"

/**
 * TBD-DPN: The current release does not have an OS abstraction layer (OSAL).
 * In subsequent releases, we will have a HUB OSAL to take care of all
 * operations performed by the underlying OS on behalf of / for HUB.
 */

/**
 * TBD-DPN: Currently, we support only 1 I2C.
 * This logic will be changed when we add support for multiple I2Cs
 */
static struct hub_gard_bus_i2c_props *p_i2c_ctx = NULL;

/**
 * Open an I2C bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 * Any further operation on the said I2C bus has to use this bus handle.
 *
 * @param: param is an opaque pointer to the bus's properties.
 *
 * @return: 0/positive integer indicates success, and is the bus's handle
 * 		 -1 for failure
 */
int32_t hub_i2c_device_open(void *param)
{
	int bus_hdl;

	p_i2c_ctx                  = (struct hub_gard_bus_i2c_props *)param;
	char     i2c_bus[PATH_MAX] = {0};

	uint32_t bus_num           = p_i2c_ctx->num;
	uint32_t slave_id          = p_i2c_ctx->slave_id;

	sprintf(i2c_bus, "%s%d", I2C_BUS_STRING_PREAMBLE, bus_num);

	if (p_i2c_ctx->is_open) {
		hub_pr_dbg("Bus already open with bus_hdl:%d\n", p_i2c_ctx->bus_hdl);
		return p_i2c_ctx->bus_hdl;
	}

	bus_hdl = open(i2c_bus, O_RDWR);
	if (bus_hdl < 0) {
		hub_pr_err("Error opening %s\n", i2c_bus);
		goto err_i2c_open_1;
	}

	if (ioctl(bus_hdl, I2C_SLAVE, slave_id) == -1) {
		hub_pr_err("Error setting slave id %d on bus %d\n", slave_id, bus_num);
		goto err_i2c_open_2;
	}

	hub_pr_dbg("Opened bus:%d slave:%d speed:%d with bus_hdl:%d\n",
			   p_i2c_ctx->num, p_i2c_ctx->slave_id, p_i2c_ctx->speed, bus_hdl);

	p_i2c_ctx->bus_hdl = bus_hdl;
	p_i2c_ctx->is_open = true;

	return bus_hdl;

err_i2c_open_2:
	close(bus_hdl);
err_i2c_open_1:
	p_i2c_ctx = NULL;
	return -1;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Perform a write operation on the I2C bus represented by the given bus
 * handle.
 *
 * @param: i2c_bus_hdl is the handle to the I2C bus
 * @param: p_buffer is a pointer to a buffer with data to write
 * @param: count is the number of bytes to be written
 *
 * @return: Number of bytes written.
 * We also print a debug error if this number is not equal to the count
 */
int32_t
	hub_i2c_device_write(int i2c_bus_hdl, const void *p_buffer, uint32_t count)
{
	ssize_t nwrite;

	nwrite = write(i2c_bus_hdl, p_buffer, count);
	if (nwrite != count) {
		hub_pr_err("Error writing to %d\n", i2c_bus_hdl);
		goto err_write;
	}

	hub_pr_dbg("Wrote %ld bytes\n", nwrite);

err_write:
	return nwrite;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 *  Perform a read operation on the I2C bus represented by the given bus handle.
 *
 * @param: i2c_bus_hdl is the handle to the I2C bus
 * @param: p_buffer is a pointer to a buffer to be filled by the read
 * @param: count is the number of bytes to be read
 *
 * @return: Number of bytes read.
 * We also print a debug error if this number is not equal to the count
 */
int32_t hub_i2c_device_read(int i2c_bus_hdl, void *p_buffer, uint32_t count)
{
	/* We read the given size in bytes in chunks of MAX_I2C_RECV_DATA_SZ */
	ssize_t  nread, ntotal = 0;
	uint32_t chunk_index = 0;
	uint32_t num_chunks  = count / MAX_I2C_RECV_DATA_SZ;
	uint32_t left_over   = count % MAX_I2C_RECV_DATA_SZ;

	/* Read the data in chunks of MAX_I2C_RECV_DATA_SZ */
	while (chunk_index < num_chunks) {
		/* Read the next chunk of data */
		nread = read(i2c_bus_hdl, p_buffer + chunk_index * MAX_I2C_RECV_DATA_SZ,
					 MAX_I2C_RECV_DATA_SZ);
		if (MAX_I2C_RECV_DATA_SZ != (size_t)nread) {
			hub_pr_err("Error reading from %d, chunk_index:%d, nread:%ld\n",
					   i2c_bus_hdl, chunk_index, nread);
			goto err_read;
		}
		ntotal += nread;
		chunk_index++;
	}

	if (left_over > 0) {
		nread = read(i2c_bus_hdl, p_buffer + num_chunks * MAX_I2C_RECV_DATA_SZ,
					 left_over);
		if (nread != left_over) {
			hub_pr_err("Error reading from %d, left_over:%d, nread:%ld\n",
					   i2c_bus_hdl, left_over, nread);
			goto err_read;
		}
		ntotal += nread;
	}

err_read:
	return (int32_t)ntotal;
}

/**
 * TBD-DPN: Revisit the return code for this call.
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Close the I2C bus opened by the open call.
 *
 * @param: i2c_bus_hdl Handle to the open I2C bus.
 *
 * @return: 0
 */
int32_t hub_i2c_device_close(int i2c_bus_hdl)
{
	if (NULL != p_i2c_ctx) {
		close(i2c_bus_hdl);
		hub_pr_dbg("Closed bus_hdl %d\n", i2c_bus_hdl);
		p_i2c_ctx->bus_hdl = -1;
		p_i2c_ctx->is_open = false;

		p_i2c_ctx          = NULL;

		return 0;
	}

	hub_pr_dbg("Bus already closed / not open!\n");

	return 0;
}
