/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_reg_ops.h"

/**
 * Write a given 32-bit value to a register address in GARD memory
 * represented by the gard handle.
 * 
 * @param: p_gard_handle GARD handle for preforming the write
 * @param: reg_addr register address inside the GARD memory map
 * @param: p_value value to be written to the GARD register
 * 
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_WRITE_REG on failure
 */
enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
									 uint32_t       reg_addr,
									 const uint32_t value)
{
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard               = NULL;
	struct _host_requests   write_reg_cmd      = {0};
	struct _host_responses  write_reg_response = {0};

	gard                     = (struct hub_gard_info *)p_gard_handle;

	write_reg_cmd.command_id = WRITE_REG_VALUE_TO_GARD_AT_OFFSET;
	write_reg_cmd.write_reg_value_to_gard_at_offset_request.offset_address =
		reg_addr;
	write_reg_cmd.write_reg_value_to_gard_at_offset_request.data = value;
	write_reg_cmd.write_reg_value_to_gard_at_offset_request.end_of_data_marker =
		END_OF_DATA_MARKER;

	bus_type = gard->control_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->control_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->control_bus->uart.bus_hdl;
		break;
	default:
		hub_pr_err("Bus not supported for write_reg!\n");
		goto err_write_reg_1;
	}

	/* Lock the control bus mutex before bus operations */
	hub_mutex_lock(&gard->control_bus->bus_mutex);

	/* We now assume that the bus is open! */
	nwrite = gard->control_bus->fops.device_write(
		bus_hdl, &write_reg_cmd.command_id, sizeof(write_reg_cmd.command_id));
	if (sizeof(write_reg_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending write_reg cmd\n");
		goto err_write_reg_2;
	}

	nwrite = gard->control_bus->fops.device_write(
		bus_hdl, &write_reg_cmd.command_body,
		sizeof(write_reg_cmd.write_reg_value_to_gard_at_offset_request));
	if (sizeof(write_reg_cmd.write_reg_value_to_gard_at_offset_request) !=
		nwrite) {
		hub_pr_err("Error sending write_reg data\n");
		goto err_write_reg_2;
	}

	/* Bus response collect */
	nread = gard->control_bus->fops.device_read(
		bus_hdl,
		(void *)&write_reg_response.write_reg_value_to_gard_at_offset_response,
		sizeof(write_reg_response.write_reg_value_to_gard_at_offset_response));
	if (sizeof(write_reg_response.write_reg_value_to_gard_at_offset_response) !=
		nread) {
		hub_pr_err("Error getting write_reg response\n");
		goto err_write_reg_2;
	}

	hub_mutex_unlock(&gard->control_bus->bus_mutex);

	if (ACK_BYTE !=
		write_reg_response.write_reg_value_to_gard_at_offset_response.ack) {
		hub_pr_err("Error in write_reg ack\n");
		goto err_write_reg_2;
	}

	hub_pr_dbg("SUCCESS!\n");

	return HUB_SUCCESS;

err_write_reg_2:
	hub_mutex_unlock(&gard->control_bus->bus_mutex);
err_write_reg_1:
	return HUB_FAILURE_WRITE_REG;
}

/**
 * Read the 32-bit value of a register address in GARD memory
 * represented by the gard handle.
 * 
 * @param: p_gard_handle GARD handle for preforming the read
 * @param: reg_addr register address inside the GARD memory map
 * @param: p_value buffer to be filled on successful read
 * 
 * @return: hub_ret_code
 * 			HUB_SUCCESS on success
 * 			HUB_FAILURE_READ_REG on failure
 */
enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
									uint32_t      reg_addr,
									uint32_t     *p_value)

{
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard              = NULL;
	struct _host_requests   read_reg_cmd      = {0};
	struct _host_responses  read_reg_response = {0};

	gard                    = (struct hub_gard_info *)p_gard_handle;

	read_reg_cmd.command_id = READ_REG_VALUE_FROM_GARD_AT_OFFSET;
	read_reg_cmd.read_reg_value_from_gard_at_offset_request.offset_address =
		reg_addr;
	read_reg_cmd.read_reg_value_from_gard_at_offset_request.end_of_data_marker =
		END_OF_DATA_MARKER;

	bus_type = gard->control_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->control_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->control_bus->uart.bus_hdl;
		break;
	default:
		hub_pr_err("Bus not supported for read_reg!\n");
		goto err_read_reg_1;
	}

	/* Lock the control bus mutex before bus operations */
	hub_mutex_lock(&gard->control_bus->bus_mutex);

	/* We now assume that the bus is open! */
	nwrite = gard->control_bus->fops.device_write(
		bus_hdl, &read_reg_cmd.command_id, sizeof(read_reg_cmd.command_id));
	if (sizeof(read_reg_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending read_reg cmd\n");
		goto err_read_reg_2;
	}

	nwrite = gard->control_bus->fops.device_write(
		bus_hdl, &read_reg_cmd.command_body,
		sizeof(read_reg_cmd.read_reg_value_from_gard_at_offset_request));
	if (sizeof(read_reg_cmd.read_reg_value_from_gard_at_offset_request) !=
		nwrite) {
		hub_pr_err("Error sending read_reg reg_addr\n");
		goto err_read_reg_2;
	}

	/* Bus repsonse collect */
	nread = gard->control_bus->fops.device_read(
		bus_hdl,
		(void *)&read_reg_response.read_reg_value_from_gard_at_offset_response,
		sizeof(read_reg_response.read_reg_value_from_gard_at_offset_response));
	if (sizeof(read_reg_response.read_reg_value_from_gard_at_offset_response) !=
		nread) {
		hub_pr_err("Error getting read_reg response\n");
		goto err_read_reg_2;
	}

	hub_mutex_unlock(&gard->control_bus->bus_mutex);

	if ((START_OF_DATA_MARKER !=
		 read_reg_response.read_reg_value_from_gard_at_offset_response
			 .start_of_data_marker) ||
		(END_OF_DATA_MARKER !=
		 read_reg_response.read_reg_value_from_gard_at_offset_response
			 .end_of_data_marker)) {
		hub_pr_err("Error in read_reg response\n");
		goto err_read_reg_2;
	}

	*p_value =
		read_reg_response.read_reg_value_from_gard_at_offset_response.reg_value;

	hub_pr_dbg("SUCCESS!\n");

	return HUB_SUCCESS;

err_read_reg_2:
	hub_mutex_unlock(&gard->control_bus->bus_mutex);
err_read_reg_1:
	return HUB_FAILURE_READ_REG;
}