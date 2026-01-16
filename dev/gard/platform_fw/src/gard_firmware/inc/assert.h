/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/
#ifndef __ASSERT_H__
#define __ASSERT_H__

#include "types.h"

/* Runtime assert to be used sparingly.*/
#define GARD__ASSERT(expr, ...)                                                \
	do {                                                                       \
		if (!(expr)) {                                                         \
			gard_assert_failed(#expr, __FILE__, __LINE__, ##__VA_ARGS__);      \
		}                                                                      \
	} while (0)

/* Runtime assert to be used for DEBUG builds.*/
#ifdef GARD_DEBUG
#define GARD__DBG_ASSERT(expr, ...) GARD__ASSERT(expr, __VA_ARGS__)
#else
#define GARD__DBG_ASSERT(expr, ...) /* Do nothing */
#endif

/* Compile-time asserts. */
#if (__STDC_VERSION__ >= 201112)
/**
 * This macro has to be defined here since the static assert function name is
 * different in C11 and C++11
 */
#define static_assert _Static_assert
#else
#ifndef __cplusplus
/* Currently we do not support. */
#error "Run compiler with flag -std=c11"
#endif
#endif

#define GARD__CASSERT(expr, msg) static_assert((expr), msg)

/* Logs the message to the log file and uart; never returns to the caller. */
/* The attribute noreturn is for code coverage */
void gard_assert_failed(const i8 *assert_expr,
						const i8 *filename,
						const u32 lineno,
						...) __attribute__((noreturn));

#endif /* __ASSERT_H__ */
