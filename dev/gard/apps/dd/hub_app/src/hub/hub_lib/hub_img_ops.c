/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_img_ops.h"

/**
 * Capture a rescaled image from the connected GARD
 * and get the properties of the image.
 *
 * NOTE:
 * - This is an asynchronous call from HUB to GARD FW.
 * - On receiving this command, GARD FW pauses its pipelines, captures the
 * 	 rescaled image from the camera into HRAM, and returns the image properties
 * 	 to HUB via the struct hub_img_ops_ctx.
 * - HUB / Host Application should call hub_recv_data_from_gard() after this
 *   function to receive the image content from GARD FW, in the image_buffer
 *   field of the struct hub_img_ops_ctx.
 * - HUB / Host Applicaiton should also call hub_send_resume_pipeline() after
 *   receiving the image content to signal GARD FW to resume the paused AI
 *   workload.
 *
 * @param p_gard_handle GARD handle
 * @param p_img_ops_ctx HUB image operations context
 *
 * @return enum hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_CAPTURE_RESCALED_IMAGE on failure
 */
enum hub_ret_code
	hub_capture_rescaled_image_from_gard(gard_handle_t           p_gard_handle,
										 struct hub_img_ops_ctx *p_img_ops_ctx)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info   *gard = (struct hub_gard_info *)p_gard_handle;

	struct _host_requests   img_props_cmd      = {0};
	struct _host_responses  img_props_response = {0};

	/* Pathological cases */
	if (NULL == p_gard_handle) {
		hub_pr_err("Error: p_gard_handle is NULL\n");
		goto err_capture_rescaled_image_1;
	}

	if (NULL == p_img_ops_ctx) {
		hub_pr_err("Error: p_img_ops_ctx is NULL\n");
		goto err_capture_rescaled_image_1;
	}

	img_props_cmd.command_id = CAPTURE_RESCALED_IMAGE;
	img_props_cmd.capture_rescaled_image_request.camera_id =
		p_img_ops_ctx->camera_id;
	img_props_cmd.capture_rescaled_image_request.rsvd1 = 0;
	img_props_cmd.capture_rescaled_image_request.end_of_data_marker =
		END_OF_DATA_MARKER;

	/**
	 * Send the get image props command to the GARD
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
		hub_pr_err("Bus not supported for capture_rescaled_image!\n");
		goto err_capture_rescaled_image_1;
		break;
	default:
		hub_pr_err("%s: Bus not supported for capture_rescaled_image!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_capture_rescaled_image_1;
	}

	/* Lock the data bus mutex before bus operations */
	hub_mutex_lock(&gard->data_bus->bus_mutex);

	/* We now assume that the bus is open! */

	/* Send the command to the GARD */
	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &img_props_cmd.command_id, sizeof(img_props_cmd.command_id));
	if (sizeof(img_props_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending capture_rescaled_image cmd\n");
		goto err_capture_rescaled_image_2;
	}

	nwrite = gard->data_bus->fops.device_write(
		bus_hdl, &img_props_cmd.command_body,
		sizeof(img_props_cmd.capture_rescaled_image_request));
	if (sizeof(img_props_cmd.capture_rescaled_image_request) != nwrite) {
		hub_pr_err("Error sending capture_rescaled_image request\n");
		goto err_capture_rescaled_image_2;
	}

	/**
	 * If the bus_type is I2C, introduce a sleep of 1 second to give GARD FW
	 * some time to pause its pipelines and return with image props.
	 */
	if (HUB_GARD_BUS_I2C == bus_type) {
		sleep(1);
	}

	/* Receive the command response from the GARD */
	nread = gard->data_bus->fops.device_read(
		bus_hdl, (void *)&img_props_response.capture_rescaled_image_response,
		sizeof(img_props_response.capture_rescaled_image_response));
	if (sizeof(img_props_response.capture_rescaled_image_response) != nread) {
		hub_pr_err("Error receiving capture_rescaled_image response\n");
		goto err_capture_rescaled_image_2;
	}

	/* Unlock the data bus mutex after bus operations */
	hub_mutex_unlock(&gard->data_bus->bus_mutex);

	if ((START_OF_DATA_MARKER !=
		 img_props_response.capture_rescaled_image_response
			 .start_of_data_marker) ||
		(END_OF_DATA_MARKER !=
		 img_props_response.capture_rescaled_image_response.eod
			 .end_of_data_marker)) {
		hub_pr_err("Error in capture_rescaled_image markers\n");
		goto err_capture_rescaled_image_1;
	}

	/* Get the image buffer size from the response */
	p_img_ops_ctx->image_buffer_size =
		img_props_response.capture_rescaled_image_response.image_buffer_size;

	/* If the image_buffer_size is 0, then the image is not available */
	if (0 == p_img_ops_ctx->image_buffer_size) {
		hub_pr_err("Error in capture_rescaled_image buffer size\n");
		goto err_capture_rescaled_image_1;
	}

	p_img_ops_ctx->h_size =
		img_props_response.capture_rescaled_image_response.h_size;
	p_img_ops_ctx->v_size =
		img_props_response.capture_rescaled_image_response.v_size;
	p_img_ops_ctx->image_buffer_address =
		img_props_response.capture_rescaled_image_response.image_buffer_address;
	p_img_ops_ctx->image_format =
		img_props_response.capture_rescaled_image_response.image_format;

	hub_pr_dbg("CAPTURE RESCALED IMAGE SUCCESS!\n");

	return HUB_SUCCESS;

err_capture_rescaled_image_2:
	ret = gard->data_bus->fops.device_close(bus_hdl);
	(void)ret;
	hub_mutex_unlock(&gard->data_bus->bus_mutex);
err_capture_rescaled_image_1:
	return HUB_FAILURE_CAPTURE_RESCALED_IMAGE;
}