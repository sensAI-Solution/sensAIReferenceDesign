/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
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
 * Write a given 32-bit value to a register address in GARD memory
 * represented by the gard handle.
 */
enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
									 uint32_t       reg_addr,
									 const uint32_t value);

/**
 * Read the 32-bit value of a register address in GARD memory
 * represented by the gard handle.
 */
enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
									uint32_t      reg_addr,
									uint32_t     *p_value);

#endif /* __HUB_REG_OPS_H__ */