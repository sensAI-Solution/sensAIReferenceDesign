/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef ML_INFO_H
#define ML_INFO_H

#include "gard_types.h"

/**
 * This file contains any specifics related to the ML engine and its operations.
 */

/**
 * ML engine needs the ML networks and the data buffers to be aligned to 64-bit
 * boundaries. This is required for the DMA engine that transfers data between
 * the memory and the ML engine.
 */
#define ML_ENGINE_MEM_ALIGNMENT_BYTES 	8U


#endif /* ML_INFO_H */
