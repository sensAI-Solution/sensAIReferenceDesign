/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef GARD_TYPES_H
#define GARD_TYPES_H

/**
 * This file defines the types used in the GARD firmware. Any new types and
 * handles should be added here to ensure consistency across the firmware.
 */
#include "types.h"

/* Handle used to point to network. */
typedef cpu_size_t ml_network_handle_t;

/**
 * App Module provided Handler function to be called by FW Core when data is
 * received from Host for App Module. As evident this data is sent by the App
 * Module's counterpart running on the Host and hence this data is opaque for
 * FW Core.
 */
typedef bool (*rx_handler_t)(uint8_t *data, uint32_t data_length);

/**
 * The app_handle_t is a handle used by the App Module to maintain its context.
 * This handle is returned by the app_preinit() function and is passed to
 * other App Module functions which can use it to access the App Module's
 * context.
 */
typedef void *app_handle_t;

#endif /* GARD_TYPES_H */
