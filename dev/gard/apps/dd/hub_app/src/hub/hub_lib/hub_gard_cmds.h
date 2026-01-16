/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_GARD_CMDS_H__
#define __HUB_GARD_CMDS_H__

#include "hub.h"
#include "gard_info.h"
#include "gard_hub_iface.h"
#include "hub_globals.h"

/**
 * TBD-DPN: Move all GARD commands such as discovery to this file
 */

/**
 * Send the resume pipeline command to the GARD
 */
enum hub_ret_code hub_send_resume_pipeline(gard_handle_t p_gard_handle,
										   uint8_t       camera_id);

#endif /* __HUB_GARD_CMDS_H__ */