/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_bulk_ops.h"

/**
 * Write a data blob of a specified size from a given buffer
 * to an address in the SOM's HRAM represented by the gard handle.
 *
 * @param: p_gard_handle is the GARD handle to use for write blob
 * @param: p_buffer is a buffer containing data to write
 * @param: addr is the address in HRAM to write to
 * @param: count is the number of bytes to write
 * @return: hub_ret_code return code indicating success/failure
 * 		HUB_SUCCESS for success
 * 		HUB_FAILURE_SEND_DATA for failure
 */
enum hub_ret_code hub_write_data_blob_to_gard(gard_handle_t p_gard_handle,
											  const void   *p_buffer,
											  uint32_t      addr,
											  uint32_t      count)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard = NULL;

	gard                         = (struct hub_gard_info *)p_gard_handle;

	bus_type                     = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_USB:
		/* Lock the data bus mutex before bus operations */
		hub_mutex_lock(&gard->data_bus->bus_mutex);
		bus_hdl = gard->data_bus->fops.device_open(&gard->data_bus->usb);
		break;
	default:
		hub_pr_err("%s: Bus not supported for write_data_blob!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_write_blob_1;
	}

	if (bus_hdl < 0) {
		hub_pr_err("Error opening USB bus for write_blob\n");
		goto err_write_blob_2;
	}

	struct hub_usb_ops_map usb_ops = {
		.p_buffer = (void *)p_buffer,
		.addr     = addr,
	};

	/* Note the opaque bundling of p_buffer and addr for USB */
	ssize_t nwrite = gard->data_bus->fops.device_write(
		bus_hdl, (const void *)&usb_ops, count);
	/* Also note we check for 1 to adhere to Python code */
	/* if (count != nwrite) { code commented for try-out */
	if (1 != nwrite) {
		hub_pr_err("Error sending write_blob data\n");
		goto err_write_blob_2;
	}

	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	hub_pr_dbg("SUCCESS - Wrote %d bytes @ 0x%x\n", count, addr);

	/* Note: Bus is not closed here - it stays open for subsequent operations */

	return HUB_SUCCESS;

err_write_blob_2:
	if (bus_hdl >= 0) {
		ret = gard->data_bus->fops.device_close(bus_hdl);
		(void)ret;
	}
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_write_blob_1:
	return HUB_FAILURE_SEND_DATA;
}

/**
 * Read a data blob of a specified size from a given buffer
 * from an address in the SOM's HRAM represented by the gard handle.
 *
 * @param: p_gard_handle is the GARD handle for preforming the read blo
 * @param: p_buffer is a buffer to be filled on successful rea
 * @param: addr is the address in HRAM to read fro
 * @param: count is the number of bytes to read
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_RECV_DATA on failure
 */
enum hub_ret_code hub_read_data_blob_from_gard(gard_handle_t p_gard_handle,
											   void         *p_buffer,
											   uint32_t      addr,
											   uint32_t      count)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard = NULL;

	gard                         = (struct hub_gard_info *)p_gard_handle;

	bus_type                     = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_USB:
		/* Lock the data bus mutex before bus operations */
		hub_mutex_lock(&gard->data_bus->bus_mutex);
		bus_hdl = gard->data_bus->usb.bus_hdl;
		break;
	default:
		hub_pr_err("%s: Bus not supported for read_data_blob!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_read_blob_1;
	}

	if (bus_hdl < 0) {
		hub_pr_err("Error opening USB bus for read_blob\n");
		goto err_read_blob_2;
	}

	struct hub_usb_ops_map usb_ops = {
		.p_buffer = (void *)p_buffer,
		.addr     = addr,
	};

	/* Note the opaque bundling of p_buffer and addr for USB */
	ssize_t nread =
		gard->data_bus->fops.device_read(bus_hdl, (void *)&usb_ops, count);
	/* Also note we check for 1 to adhere to Python code */
	/* if (count != nwrite) { code commented for try-out */
	if (1 != nread) {
		hub_pr_err("Error getting read_blob data\n");
		goto err_read_blob_2;
	}

	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	hub_pr_dbg("SUCCESS - Read %d bytes from 0x%x!\n", count, addr);

	/* Note: Bus is not closed here - it stays open for subsequent operations */

	return HUB_SUCCESS;

err_read_blob_2:
	ret = gard->data_bus->fops.device_close(bus_hdl);
	(void)ret;
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_read_blob_1:
	return HUB_FAILURE_RECV_DATA;
}