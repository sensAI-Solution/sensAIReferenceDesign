/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/
#ifndef TYPES_H
#define TYPES_H

/**
 * The typedef's defined here are confirmed to work with gcc/g++ on 32-bit and
 * 64-bit CPU architectures. Still this needs to be validated when a new
 * compiler is tried or the same compiler is used on a different CPU
 * architecture.
 *
 * Currently we have validated this file working with:
 * 1) gcc version 10.2.0 outputting code for 32-bit riscv processor
 */

#ifdef CPU_32BIT
/* CPU's with 32-bit data width */
typedef char               i8;
typedef signed short       i16;
typedef signed long        i32;
typedef signed long long   i64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned long      u32;
typedef unsigned long long u64;

typedef unsigned long      cpu_size_t;
#else /* #ifdef CPU_32BIT */
/* CPU's with 64-bit data width */
typedef char           i8;
typedef signed short   i16;
typedef signed int     i32;
typedef signed long    i64;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;

typedef unsigned long  cpu_size_t;
#endif

typedef u8 byte;

#ifndef __cplusplus
typedef u8 bool;
#define true  1
#define false 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/**
 * Some code bases use different typedef names. The common ones are listed
 * below.
 */

/**
 * Some dev env installations provide header files that define these
 * already. We try to use gcc extensions for discovering the presence of
 * stdint.h header file and including it. If not present then we provide our own
 * typedefs
 */

#if defined(__has_include)
#if __has_include(<stdint.h>)
#include <stdint.h>
#endif
#endif

/* Conditionally define fixed-width unsigned and signed types */

#ifndef UINT8_MAX
typedef u8 uint8_t;
#endif

#ifndef UINT16_MAX
typedef u16 uint16_t;
#endif

#ifndef UINT32_MAX
typedef u32 uint32_t;
#endif

#ifndef UINT64_MAX
typedef u64 uint64_t;
#endif

#ifndef INT8_MAX
typedef i8 int8_t;
#endif

#ifndef INT16_MAX
typedef i16 int16_t;
#endif

#ifndef INT32_MAX
typedef i32 int32_t;
#endif

#ifndef INT64_MAX
typedef i64 int64_t;
#endif

#endif /* TYPES_H */
