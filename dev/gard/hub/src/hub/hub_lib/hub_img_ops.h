/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_IMG_OPS_H__
#define __HUB_IMG_OPS_H__

#include <unistd.h>

#include "hub.h"
#include "gard_info.h"
#include "gard_hub_iface.h"
#include "hub_globals.h"

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
 */
enum hub_ret_code
	hub_capture_rescaled_image_from_gard(gard_handle_t           p_gard_handle,
										 struct hub_img_ops_ctx *p_img_ops_ctx);

#endif /* __HUB_IMG_OPS_H__ */