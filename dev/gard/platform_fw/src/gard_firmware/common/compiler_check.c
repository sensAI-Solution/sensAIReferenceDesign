/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#include "types.h"
#include "assert.h"

/**
 * This file does not generate any code. Instead it is used to make sure that
 * the compiler assumptions match our requirements. Any compiler-specific code
 * is to be captured here instead of scattering across source files. This file
 * is to be compiled by every different compiler which is used in the
 * compilation of different modules running on different subsystems
 */

GARD__CASSERT(sizeof(i8) == 1, "Size of i8 is not 1-byte");
GARD__CASSERT(sizeof(i16) == 2, "Size of i16 is not 2-bytes");
GARD__CASSERT(sizeof(i32) == 4, "Size of i32 is not 4-bytes");
GARD__CASSERT(sizeof(i64) == 8, "Size of i64 is not 8-bytes");

GARD__CASSERT(sizeof(u8) == 1, "Size of u8 is not 1-byte");
GARD__CASSERT(sizeof(u16) == 2, "Size of u16 is not 2-bytes");
GARD__CASSERT(sizeof(u32) == 4, "Size of u32 is not 4-bytes");
GARD__CASSERT(sizeof(u64) == 8, "Size of u64 is not 8-bytes");

/* Most recent cpu architectures have the same data and addr width size */
GARD__CASSERT(sizeof(cpu_size_t) == sizeof(void *),
			  "Size of cpu_size_t is not equal to size of CPU data width");
