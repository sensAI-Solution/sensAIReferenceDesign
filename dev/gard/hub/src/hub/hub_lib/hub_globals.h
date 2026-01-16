/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_GLOBALS_H__
#define __HUB_GLOBALS_H__

#include "hub.h"

/* The version string for HUB - refer hub_version.h */
extern char hub_version_string[];

/**
 * Array of pointers to string representations of
 * various busses supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus's enum as defined in HUB (hub.h)
 *
 * Externed here so that any HUB code can include it for use.
 */
extern const char *hub_gard_bus_strings[];

#endif /* __HUB_GLOBALS_H__ */