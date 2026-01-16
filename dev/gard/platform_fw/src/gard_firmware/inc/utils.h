/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include "gard_types.h"

// Utility macros.
#define STR_HELPER(x)          #x
#define STR(x)                 STR_HELPER(x)

#define MIN(a, b)              ((a) < (b) ? (a) : (b))
#define MAX(a, b)              ((a) > (b) ? (a) : (b))

#define GET_ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

#define GET_MEMBER_OFFSET(container_type, member)                              \
	(cpu_size_t)(&(((container_type *)0)->member))

#define GET_MEMBER_SIZE(container_type, member)                                \
	sizeof(((container_type *)0)->member)

/* Macros for bit-manipulation. */
#define GARD__GET_BITS(var, mask)   ((var) & (mask))
#define GARD__SET_BITS(var, mask)   (var) |= (mask)
#define GARD__RESET_BITS(var, mask) (var) &= ~(mask)

/**
 * calculate_checksum() returns the 32-bit checksum of the given data.
 */
uint32_t calculate_checksum(uint8_t *data, uint32_t size);

/**
 * memcpy() copies 'size' bytes from 'src' to 'dest'.
 * The function does not handle overlapping memory regions.
 */
void *memcpy(void *dest, const void *src, uint32_t size);

/**
 * memcpy_w() copies 'size' bytes from 'src' to 'dest' in word (4-byte)
 * increments. The function expects 'src' and 'dest' to be word-aligned and
 * 'size' to be a multiple of the word size. The function does not handle
 * overlapping memory regions.
 */
uint32_t *memcpy_w(uint32_t *dest, const uint32_t *src, uint32_t size);

/**
 * memset() sets 'size' bytes in 'buffer' to the specified 'value'.
 */
void *memset(void *buffer, int32_t value, uint32_t size);

/**
 * get_cpu_tsc() reads and returns the current value of the CPU's Time Stamp
 * Counter (TSC).
 */
uint64_t get_cpu_tsc(void);

/**
 * delay() creates a blocking delay for the specified number of milliseconds.
 */
void delay(uint32_t delay_in_ms);

/**
 * set_timer_for_time() sets a timer to expire after the specified time in
 * ms. It returns the absolute time (in CPU TSC units) when the timer will
 * expire.
 */
uint64_t set_timer_for_time(uint32_t time_in_ms);

/**
 * has_timer_expired() checks if the previously set timer has expired.
 */
bool has_timer_expired(void);

#endif /* UTILS_H */
