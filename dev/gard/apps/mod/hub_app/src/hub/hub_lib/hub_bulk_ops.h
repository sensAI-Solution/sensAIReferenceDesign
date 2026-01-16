/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_BULK_OPS_H__
#define __HUB_BULK_OPS_H__

#include "hub.h"
#include "gard_info.h"
#include "gard_hub_iface.h"
#include "hub_globals.h"

/**
 * Write a data blob of a specified size from a given buffer
 * to an address in the SOM's HRAM represented by the gard handle.
 */
enum hub_ret_code hub_write_data_blob_to_gard(gard_handle_t p_gard_handle,
											  const void   *p_buffer,
											  uint32_t      addr,
											  uint32_t      count);

/**
 * Read a data blob of a specified size from a given buffer
 * from an address in the SOM's HRAM represented by the gard handle.
 */
enum hub_ret_code hub_read_data_blob_from_gard(gard_handle_t p_gard_handle,
											   void         *p_buffer,
											   uint32_t      addr,
											   uint32_t      count);

#endif /* __HUB_BULK_OPS_H__ */

