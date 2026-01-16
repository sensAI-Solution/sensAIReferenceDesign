/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef HOST_CMDS_H
#define HOST_CMDS_H

/**
 * This file is shared between GARD firmware and HUB running on Host.
 * Make sure this file contents are the ones that are common between both GARD
 * and HUB or else compile errors will result for the other peer.
 * This file defines the commands and responses exchanged between the GARD
 * firmware and the Host (HUB) over UART and I2C.
 */
#include "gard_types.h"
#include "iface_support.h"

/**
 * host_request_init() initializes the internal variables needed by the host
 * request service module for its operation.
 */
bool host_request_init(void);

/**
 * service_host_requests() services the host requests arriving over all the
 * valid interfaces.
 */
bool service_host_requests(void);

/**
 * set_rx_done() sets the flag rx_done to true to indicate that data
 * reception is complete.
 */
void set_rx_done(struct iface_instance *inst);

/**
 * set_tx_done() sets the flag tx_done to true to indicate that data
 * transmission is complete.
 */
void set_tx_done(struct iface_instance *inst);

#endif  // HOST_CMDS_H
