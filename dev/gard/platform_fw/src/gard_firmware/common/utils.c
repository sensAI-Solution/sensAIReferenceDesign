/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "assert.h"
#include "utils.h"
#include "cpu.h"

static uint64_t timer_set_for_time = 0;

/**
 * calculate_checksum() computes the checksum of the given data. The checksum is
 * simply the sum of all bytes in the data array. A 32-bit unsigned integer is
 * used to avoid overflow issues with larger data sizes.
 *
 * @param data points to the data array for which the checksum is to be
 *              calculated.
 * @param size is the size of data array in bytes.
 *
 * @return The computed checksum as a 32-bit unsigned integer.
 */
uint32_t calculate_checksum(uint8_t *data, uint32_t size)
{
	uint32_t checksum = 0;
	for (uint32_t i = 0; i < size; i++) {
		checksum += data[i];
	}
	return checksum;
}

/**
 * memcpy() copies a specified number of bytes from the source buffer to the
 * destination buffer. It performs a simple byte-by-byte copy.
 * The routine does not handle overlapping memory regions, so it is the
 * responsibility of the caller to ensure that the memory regions do not overlap
 * unless the overlap is intentional.
 *
 * @param dest points to the destination buffer where data will be copied.
 * @param src points to the source buffer from which data will be copied.
 * @param size is the count of bytes to copy from source to destination.
 *
 * @return Pointer to the destination buffer after copying the data.
 */
void *memcpy(void *dest, const void *src, uint32_t size)
{
	uint8_t *d = (uint8_t *)dest;
	uint8_t *s = (uint8_t *)src;

	GARD__DBG_ASSERT((NULL != d) && (NULL != s),
					 "Invalid parameters for memcpy");

	for (uint32_t i = 0; i < size; i++) {
		*d++ = *s++;
	}

	return dest;
}

/**
 * memcpy_w() copies a specified number of bytes from the source buffer to the
 * destination buffer. It performs a word-by-word copy. The word size is the
 * data size of the CPU architecture (typically 4 bytes for 32-bit systems).
 * The routine expects the src and dest pointers to be aligned to word
 * boundaries and size to be a multiple of the word size.
 * The routine does not handle overlapping memory regions, so it is the
 * responsibility of the caller to ensure that the memory regions do not overlap
 * unless the overlap is intentional.
 *
 * @param dest points to the destination buffer where data will be copied.
 * @param src points to the source buffer from which data will be copied.
 * @param size is the number of bytes to copy from source to destination.
 *
 * @return Pointer to the destination buffer after copying the data.
 */
uint32_t *memcpy_w(uint32_t *dest, const uint32_t *src, uint32_t size)
{
	/* Save original pointer to return it back from this function. */
	uint32_t *original_dest = dest;

	GARD__DBG_ASSERT((NULL != dest) && (NULL != src) &&
						 ((((uint32_t)dest) & (sizeof(uint32_t) - 1)) == 0) &&
						 ((((uint32_t)src) & (sizeof(uint32_t) - 1)) == 0) &&
						 ((size & (sizeof(uint32_t) - 1)) == 0),
					 "Invalid parameters for memcpy_w");

	for (uint32_t i = 0; i < (size / sizeof(uint32_t)); i++) {
		*dest++ = *src++;
	}

	return original_dest;
}

/**
 * memset() sets the memory pointed by buffer with byte character 'c' for
 * size number of bytes It performs a simple byte-copy operation.
 *
 * @param buffer points to the destination buffer where data will be set.
 * @param value is the value to set in each byte of the destination buffer.
 * @param size is the count of bytes to set in the destination buffer.
 *
 * @return Pointer to the destination buffer after setting the data.
 */
void *memset(void *buffer, int32_t value, uint32_t size)
{
	uint8_t *ret = (uint8_t *)buffer;

	GARD__DBG_ASSERT(NULL != ret, "Invalid buffer for memset");

	while (size--) {
		*ret++ = value;
	}

	return buffer;
}

/**
 * get_cpu_tsc() reads and returns the current value of the CPU's Time Stamp
 * Counter (TSC). The TSC implemented in Lattice RISC-V CPUs is a 64-bit counter
 * that is driven by a 32KHz clock source. The TSC is available as MTIME
 * register in the CLINT (Core Local Interruptor) memory-mapped region.
 *
 *@return The current value of the CPU's Time Stamp Counter (value in MTIME
		  register).
 */
uint64_t get_cpu_tsc(void)
{
	uint64_t tsc;

	tsc = *((volatile uint64_t *)&(CLINT_MTIME_L));
	return tsc;
}

/**
 * set_timer_for_time() sets a timer to expire after the specified time in
 * milliseconds. It calculates the absolute time (in CPU TSC units) when the
 */
uint64_t set_timer_for_time(uint32_t time_in_ms)
{
	GARD__DBG_ASSERT(timer_set_for_time == 0, "Previous timer not yet expired");
	uint64_t timer_expiry_delay =
		(uint64_t)((CLINT_TIMEBASE_FREQ * time_in_ms) / 1000U);

	timer_set_for_time = get_cpu_tsc() + timer_expiry_delay;
	return timer_set_for_time;
}

bool has_timer_expired(void)
{
	if (timer_set_for_time && get_cpu_tsc() >= timer_set_for_time) {
		timer_set_for_time = 0;
		return true;
	}

	return false;
}

/**
 * delay() creates a blocking delay for the specified number of milliseconds.
 * This delay is implemented as a simple busy-wait loop using the provided mtime
 * register values to determine the passage of time.
 * *** Note: This implementation is approximate and should not be used by any
 * real-time critical code.
 *
 * @param delay_in_ms is the number of milliseconds to delay.
 *
 * @return None
 */
void delay(uint32_t delay_in_ms)
{
	uint64_t start_time;
	uint64_t end_time;

	start_time = *((volatile uint64_t *)&(CLINT_MTIME_L));
	end_time =
		start_time + (uint64_t)((CLINT_TIMEBASE_FREQ / 1000U) * delay_in_ms);
	while (*((volatile uint64_t *)&(CLINT_MTIME_L)) < end_time) {
	}
}
