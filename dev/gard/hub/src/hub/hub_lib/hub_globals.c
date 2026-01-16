/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_globals.h"

/* The version string for HUB - refer hub_version.h */
char hub_version_string[PATH_MAX]  = {0};

/**
 * Array of pointers to string representations of
 * various busses supported by HUB.
 *
 * This array is auto-populated by a macro such that the string
 * equals the name of the bus's enum as defined in HUB (hub.h)
 */
const char *hub_gard_bus_strings[] = {FOR_EACH_BUS(GEN_BUS_STRING)};
