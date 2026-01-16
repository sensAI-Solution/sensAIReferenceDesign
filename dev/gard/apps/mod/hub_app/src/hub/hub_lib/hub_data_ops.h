/******************************************************************************
 * Copyright (c) 2024 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_REG_OPS_H__
#define __HUB_REG_OPS_H__

#include "hub.h"
#include "gard_info.h"
#include "gard_hub_iface.h"
#include "hub_globals.h"

/**
 * Send a data buffer of a specified size from HUB to an
 * address in the GARD memory map represented by the gard handle.
 */
enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
										const void   *p_buffer,
										uint32_t      addr,
										uint32_t      count);

/**
 * Receive data of specified size from an address in the
 * GARD memory map represented	by the GARD handle, into a HUB
 * data buffer.
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
enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
										   void         *p_buffer,
										   uint32_t      addr,
										   uint32_t      count);

/**
 * Receive data of specified size in the GARD memory map
 * represented by the GARD handle, into a HUB data buffer.
 *
 * Note that the addr variable here is ignored by GARD_FW.
 * It has APP_DATA control code set to receive application data.
 */
int64_t hub_recv_app_data_from_gard(gard_handle_t p_gard_handle,
									 void         *p_buffer,
									 uint32_t      addr,
									 uint32_t      count);

#endif /* __HUB_REG_OPS_H__ */