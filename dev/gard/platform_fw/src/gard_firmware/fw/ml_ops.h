/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef ML_OPS_H
#define ML_OPS_H

#include "gard_types.h"

/**
 * get_info_of_last_executed_network() is used by the FW Core to retrieve the
 * network_info structure of the last executed network.
 */
struct network_info *get_info_of_last_executed_network(void);

/**
 * ml_engine_done_isr() is the ISR that is triggered when the ML engine
 * completes processing the current network.
 */
extern void ml_engine_done_isr(void *ctx);

/**
 * get_ml_engine_status() returns the status of the specified ML engine.
 */
extern uint32_t get_ml_engine_status(uint32_t engine_id);

#endif /* ML_OPS_H */
