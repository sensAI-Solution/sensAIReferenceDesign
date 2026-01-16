/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_gard_cmds.h"

/**
 * Send the resume pipeline command to the GARD
 *
 * @param p_gard_handle GARD handle
 * @param camera_id Camera ID

 * @return HUB_SUCCESS if successful,
 *         HUB_FAILURE_SEND_RESUME_PIPELINE if failed
 */
enum hub_ret_code hub_send_resume_pipeline(gard_handle_t p_gard_handle,
										   uint8_t       camera_id)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard = (struct hub_gard_info *)p_gard_handle;

	struct _host_requests   resume_pipeline_cmd      = {0};
	struct _host_responses  resume_pipeline_response = {0};

	/* Pathological cases */
	if (NULL == p_gard_handle) {
		hub_pr_err("Error: p_gard_handle is NULL\n");
		goto err_send_resume_pipeline_1;
	}

	resume_pipeline_cmd.command_id                        = RESUME_PIPELINE;
	resume_pipeline_cmd.resume_pipeline_request.camera_id = camera_id;
	resume_pipeline_cmd.resume_pipeline_request.end_of_data_marker =
		END_OF_DATA_MARKER;

	/**
	 * Send the resume pipeline command to the GARD
	 */
	bus_type = gard->data_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		bus_hdl = gard->data_bus->i2c.bus_hdl;
		break;
	case HUB_GARD_BUS_UART:
		bus_hdl = gard->data_bus->uart.bus_hdl;
		break;
	case HUB_GARD_BUS_USB:
		hub_pr_err("Bus not supported for resume_pipeline!\n");
		goto err_send_resume_pipeline_1;
		break;
	default:
		hub_pr_err("%s: Bus not supported for resume_pipeline!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_send_resume_pipeline_1;
	}

	/* Lock the data bus mutex before bus operations */
	hub_mutex_lock(&gard->data_bus->bus_mutex);

	/* We now assume that the bus is open! */

	/* Send the command to the GARD */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &resume_pipeline_cmd.command_id,
		sizeof(resume_pipeline_cmd.command_id));
	if (sizeof(resume_pipeline_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending resume_pipeline cmd\n");
		goto err_send_resume_pipeline_2;
	}

	nread = gard->data_bus->fops.device_write(
		bus_hdl, &resume_pipeline_cmd.command_body,
		sizeof(resume_pipeline_cmd.resume_pipeline_request));
	if (sizeof(resume_pipeline_cmd.resume_pipeline_request) != nread) {
		hub_pr_err("Error sending resume_pipeline request\n");
		goto err_send_resume_pipeline_2;
	}

	/* Receive the response from the GARD */
	nread = gard->data_bus->fops.device_read(
		bus_hdl, &resume_pipeline_response.resume_pipeline_response,
		sizeof(resume_pipeline_response.resume_pipeline_response));
	if (sizeof(resume_pipeline_response.resume_pipeline_response) != nread) {
		hub_pr_err("Error receiving resume_pipeline response\n");
		goto err_send_resume_pipeline_2;
	}

	/* Unlock the data bus mutex after bus operations */
	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	if (ACK_BYTE !=
		resume_pipeline_response.resume_pipeline_response.ack_or_nak) {
		hub_pr_err("Error sending resume_pipeline cmd\n");
		goto err_send_resume_pipeline_1;
	}

	return HUB_SUCCESS;

err_send_resume_pipeline_2:
	ret = gard->data_bus->fops.device_close(bus_hdl);
	(void)ret;
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_send_resume_pipeline_1:
	return HUB_FAILURE_SEND_RESUME_PIPELINE;
}