/******************************************************************************
 * Copyright (c) 2024 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_data_ops.h"
#include "hub_bulk_ops.h"

/**
 * TBD-DPN: The code for send_data and recv_data corresponds to features that
 * are presently supported by the GARD FW for non-USB data bus:
 * 1. No checksum support
 * 2. No MTU size support
 * 3. Truncated packet write/read if checksum is not supported
 *    - not the full eod structure
 *
 * Once these features are supported in GARD FW, this code will be modified
 * to take advantages of those features.
 */

/**
 * Send a data buffer of a specified size from HUB to an
 * address in the GARD memory map represented by the gard handle.
 *
 * @param: p_gard_handle is the GARD handle to use for sending data to
 * @param: p_buffer is a buffer containing data to write
 * @param: addr is an address in GARD's memory map to write to
 * @param: count is the number of bytes to write
 *
 * @return: hub_ret_code return code indicating success/failure
 * 		 HUB_SUCCESS for success
 * 		 HUB_FAILURE_SEND_DATA for failure
 */
enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
										const void   *p_buffer,
										uint32_t      addr,
										uint32_t      count)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard = (struct hub_gard_info *)p_gard_handle;

	struct _host_requests   send_data_cmd      = {0};
	struct _host_responses  send_data_response = {0};
	enum control_codes      cc;

	send_data_cmd.command_id = SEND_DATA_TO_GARD_FOR_OFFSET;
	cc                       = CC_SEND_ACK_AFTER_XFER;
	send_data_cmd.send_data_to_gard_for_offset_request.cmd.offset_address =
		addr;
	send_data_cmd.send_data_to_gard_for_offset_request.cmd.data_size    = count;
	send_data_cmd.send_data_to_gard_for_offset_request.cmd.control_code = cc;
	send_data_cmd.send_data_to_gard_for_offset_request.cmd.mtu_size     = 0;

	send_data_cmd.send_data_to_gard_for_offset_request.eod.end_of_data_marker =
		END_OF_DATA_MARKER;
	send_data_cmd.send_data_to_gard_for_offset_request.eod.opt_crc = 0;

	bus_type = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->data_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->data_bus->uart.bus_hdl;
		break;
	case HUB_GARD_BUS_USB:
		ret = hub_write_data_blob_to_gard(p_gard_handle, p_buffer, addr, count);
		return ret;
		break;
	default:
		hub_pr_err("%s: Bus not supported for send_data!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_send_data_1;
	}

	/* Lock the data bus mutex before bus operations */
	hub_mutex_lock(&gard->data_bus->bus_mutex);

	/* We now assume that the bus is open! */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &send_data_cmd.command_id, sizeof(send_data_cmd.command_id));
	if (sizeof(send_data_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending send_data cmd id\n");
		goto err_send_data_2;
	}

	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &send_data_cmd.command_body,
		sizeof(send_data_cmd.send_data_to_gard_for_offset_request.cmd));
	if (sizeof(send_data_cmd.send_data_to_gard_for_offset_request.cmd) !=
		nwrite) {
		hub_pr_err("Error sending send_data cmd\n");
		goto err_send_data_2;
	}

	nwrite = gard->data_bus->fops.device_write(bus_hdl, p_buffer, count);
	if (count != nwrite) {
		printf("Error sending send_data buffer\n");
		goto err_send_data_2;
	}

	/**
	 * Note: HUB writes a "truncated" packet (not the full eod) as
	 * part of send_data since that is what GARD FW expects
	 * when CRC is not enabled.
	 */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl,
		&(send_data_cmd.send_data_to_gard_for_offset_request.eod
			  .end_of_data_marker),
		sizeof(send_data_cmd.send_data_to_gard_for_offset_request.eod
				   .end_of_data_marker));
	if (sizeof(send_data_cmd.send_data_to_gard_for_offset_request.eod
				   .end_of_data_marker) != nwrite) {
		hub_pr_err("Error sending send_data eod\n");
		goto err_send_data_2;
	}

	/* Bus response collect */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&send_data_response.send_data_to_gard_for_offset_response,
		sizeof(send_data_response.send_data_to_gard_for_offset_response));
	if (sizeof(send_data_response.send_data_to_gard_for_offset_response) !=
		nread) {
		hub_pr_err("Error getting send_data response\n");
		goto err_send_data_2;
	}

	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	if (ACK_BYTE !=
		send_data_response.send_data_to_gard_for_offset_response.opt_ack) {
		hub_pr_err("Error in send_data ack\n");
		goto err_send_data_2;
	}

	hub_pr_dbg("SUCCESS!\n");

	return HUB_SUCCESS;

err_send_data_2:
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_send_data_1:
	return HUB_FAILURE_SEND_DATA;
}

/**
 * Receive data of specified size from an address in the
 * GARD memory map represented	by the GARD handle, into a HUB
 * data buffer.
 *
 * Note: If this function is called to receive image data from GARD,
 * say, after hub_capture_rescaled_image_from_gard() is called, then
 * HUB / Host Applicaiton should also call hub_send_resume_pipeline() after
 * receiving the image content to signal GARD FW to resume the paused AI
 * workload.
 *
 * @param: p_gard_handle is the GARD handle for preforming the read blob
 * @param: p_buffer is the buffer to be filled on successful read
 * @param: addr is an address in GARD's memory map to read from
 * @param: count is the number of bytes to read
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_RECV_DATA on failure
 */
enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
										  void         *p_buffer,
										  uint32_t      addr,
										  uint32_t      count)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;
	uint32_t                data_size = 0;

	struct hub_gard_info   *gard      = (struct hub_gard_info *)p_gard_handle;

	struct _host_requests   recv_data_cmd      = {0};
	struct _host_responses  recv_data_response = {0};
	enum control_codes      cc;

	recv_data_cmd.command_id = RECV_DATA_FROM_GARD_AT_OFFSET;
	cc                       = 0;

	recv_data_cmd.recv_data_from_gard_at_offset_request.offset_address = addr;
	recv_data_cmd.recv_data_from_gard_at_offset_request.data_size      = count;
	recv_data_cmd.recv_data_from_gard_at_offset_request.control_code   = cc;
	recv_data_cmd.recv_data_from_gard_at_offset_request.mtu_size       = 0;

	bus_type = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->data_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->data_bus->uart.bus_hdl;
		break;
	case HUB_GARD_BUS_USB:
		ret =
			hub_read_data_blob_from_gard(p_gard_handle, p_buffer, addr, count);
		return ret;
		break;
	default:
		hub_pr_err("%s: Bus not supported for recv_data!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_recv_data_1;
	}

	/* Lock the data bus mutex before bus operations */
	hub_mutex_lock(&gard->data_bus->bus_mutex);

	/* We now assume that the bus is open! */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &recv_data_cmd.command_id, sizeof(recv_data_cmd.command_id));
	if (sizeof(recv_data_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending recv_data cmd\n");
		goto err_recv_data_2;
	}

	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &recv_data_cmd.command_body,
		sizeof(recv_data_cmd.recv_data_from_gard_at_offset_request));
	if (sizeof(recv_data_cmd.recv_data_from_gard_at_offset_request) != nwrite) {
		hub_pr_err("Error sending recv_data request\n");
		goto err_recv_data_2;
	}

	/* Bus response collect */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response
			.start_of_data_marker,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .start_of_data_marker));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .start_of_data_marker) != nread) {
		hub_pr_err("Error getting recv_data response sod\n");
		goto err_recv_data_2;
	}

	/* Read data_size from response */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response
			.data_size,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .data_size));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .data_size) != nread) {
		hub_pr_err("Error getting recv_data response data_size\n");
		goto err_recv_data_2;
	}

	data_size =
		recv_data_response.recv_data_from_gard_at_offset_response.data_size;

	nread = gard->data_bus->fops.device_read(bus_hdl, p_buffer, data_size);
	if (data_size != nread) {
		hub_pr_err("Error getting recv_data buffer\n");
		goto err_recv_data_2;
	}

	/**
	 * Note: HUB reads a "truncated" packet (not the full eod) as
	 * part of send_data since that is what GARD FW expects
	 * when CRC is not enabled.
	 */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response.eod
			.end_of_data_marker,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response.eod
				   .end_of_data_marker));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response.eod
				   .end_of_data_marker) != nread) {
		hub_pr_err("Error getting recv_data response eod\n");
		goto err_recv_data_2;
	}

	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	if ((START_OF_DATA_MARKER !=
		 recv_data_response.recv_data_from_gard_at_offset_response
			 .start_of_data_marker) ||
		(END_OF_DATA_MARKER !=
		 recv_data_response.recv_data_from_gard_at_offset_response.eod
			 .end_of_data_marker)) {
		hub_pr_err("Error in recv_data markers\n");
		goto err_recv_data_2;
	}

	hub_pr_dbg("SUCCESS!\n");

	return HUB_SUCCESS;

err_recv_data_2:
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_recv_data_1:
	return HUB_FAILURE_RECV_DATA;
}

/**
 *
 * TBD-DPN: Temporary function for getting app_data from GARD FW
 * We will revisit this approach and explore if recv_* functions
 * can be merged into a single function.
 *
 * Receive data of specified size in the GARD memory map
 * represented by the GARD handle, into a HUB data buffer.
 *
 * Note that the addr variable here is ignored by GARD_FW.
 * It has APP_DATA control code set to receive application data.
 *
 * @param: p_gard_handle is the GARD handle for preforming the read blob
 * @param: p_buffer is the buffer to be filled on successful read
 * @param: addr is an address in GARD's memory map to read from
 * @param: count is the number of bytes to read
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_RECV_APP_DATA on failure
 */
int64_t hub_recv_app_data_from_gard(gard_handle_t p_gard_handle,
									void         *p_buffer,
									uint32_t      addr,
									uint32_t      count)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;
	uint32_t                data_size = 0;

	struct hub_gard_info   *gard      = (struct hub_gard_info *)p_gard_handle;

	struct _host_requests   recv_data_cmd      = {0};
	struct _host_responses  recv_data_response = {0};
	enum control_codes      cc;

	recv_data_cmd.command_id  = RECV_DATA_FROM_GARD_AT_OFFSET;
	cc                        = 0;

	/* We set CC_APP_DATA flag to receive app data from unspecified address */
	cc                       |= CC_APP_DATA;

	recv_data_cmd.recv_data_from_gard_at_offset_request.offset_address = addr;
	recv_data_cmd.recv_data_from_gard_at_offset_request.data_size      = count;
	recv_data_cmd.recv_data_from_gard_at_offset_request.control_code   = cc;
	recv_data_cmd.recv_data_from_gard_at_offset_request.mtu_size       = 0;

	bus_type = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->data_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->data_bus->uart.bus_hdl;
		break;
	case HUB_GARD_BUS_USB:
		ret =
			hub_read_data_blob_from_gard(p_gard_handle, p_buffer, addr, count);
		/* For USB, return count on success, error code on failure */
		return (ret == HUB_SUCCESS) ? (int64_t)count : (int64_t)ret;
		break;
	default:
		hub_pr_err("%s: Bus not supported for recv_data!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_recv_app_data_1;
	}

	/* Lock the data bus mutex before bus operations */
	hub_mutex_lock(&gard->data_bus->bus_mutex);

	/* We now assume that the bus is open! */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &recv_data_cmd.command_id, sizeof(recv_data_cmd.command_id));
	if (sizeof(recv_data_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending recv_data cmd\n");
		goto err_recv_app_data_2;
	}

	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &recv_data_cmd.command_body,
		sizeof(recv_data_cmd.recv_data_from_gard_at_offset_request));
	if (sizeof(recv_data_cmd.recv_data_from_gard_at_offset_request) != nwrite) {
		hub_pr_err("Error sending recv_data request\n");
		goto err_recv_app_data_2;
	}

	/* Bus response collect */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response
			.start_of_data_marker,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .start_of_data_marker));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .start_of_data_marker) != nread) {
		hub_pr_err("Error getting recv_data response sod\n");
		goto err_recv_app_data_2;
	}

	/* Read data_size from response */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response
			.data_size,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .data_size));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response
				   .data_size) != nread) {
		hub_pr_err("Error getting recv_data response data_size\n");
		goto err_recv_app_data_2;
	}

	data_size =
		recv_data_response.recv_data_from_gard_at_offset_response.data_size;

	nread = gard->data_bus->fops.device_read(bus_hdl, p_buffer, data_size);
	if (0 == nread) {
		hub_pr_err("Error getting recv_data buffer\n");
		goto err_recv_app_data_2;
	}

	/**
	 * Note: HUB reads a "truncated" packet (not the full eod) as
	 * part of send_data since that is what GARD FW expects
	 * when CRC is not enabled.
	 */
	nread = gard->data_bus->fops.device_read(
		bus_hdl,
		(void *)&recv_data_response.recv_data_from_gard_at_offset_response.eod
			.end_of_data_marker,
		sizeof(recv_data_response.recv_data_from_gard_at_offset_response.eod
				   .end_of_data_marker));
	if (sizeof(recv_data_response.recv_data_from_gard_at_offset_response.eod
				   .end_of_data_marker) != nread) {
		hub_pr_err("Error getting recv_data response eod\n");
		goto err_recv_app_data_2;
	}

	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	if ((START_OF_DATA_MARKER !=
		 recv_data_response.recv_data_from_gard_at_offset_response
			 .start_of_data_marker) ||
		(END_OF_DATA_MARKER !=
		 recv_data_response.recv_data_from_gard_at_offset_response.eod
			 .end_of_data_marker)) {
		hub_pr_err("Error in recv_data markers\n");
		goto err_recv_app_data_2;
	}

	hub_pr_dbg("SUCCESS!\n");

	return (int64_t)data_size;

err_recv_app_data_2:
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_recv_app_data_1:
	return (int64_t)HUB_FAILURE_RECV_APP_DATA;
}

