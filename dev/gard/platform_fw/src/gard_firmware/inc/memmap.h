/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef MEMMAP_H
#define MEMMAP_H

/**
 * This file defines the memory map for the code loaded on the GARD CPU. Since
 * we have GARD firmware and GARD firmware loader execute on this same CPU,
 * this file only contains only the subset of the memory map that is of interest
 * both these modules.
 */

/**
 *
 * GARD firmware when completely loaded in memory:
 * +---------------------+ Top of memory  @ 64KB
 * |                     |
 * |      Stack          |
 * |                     |
 * +---------------------+ Start of Stack @ 56KB
 * |                     |
 * |   Unused Space      |
 * |                     |
 * +---------------------+  End of Himem Code
 * |                     |
 * |   Himem Code        |
 * |                     |
 * +---------------------+  Start of Himem Code
 * +---------------------+  End of BSS (Uninitialized Data) section
 * |                     |
 * |      .BSS           |
 * |                     |
 * +---------------------+  Start of BSS (Uninitialized Data) section
 * +---------------------+  End of Initialized Data section
 * |                     |
 * |      .DATA          |
 * |                     |
 * +---------------------+  Start of Initialized Data section
 * +---------------------+  End of Lowmem Code
 * |                     |
 * |   Lowmem Code       | OSPI code to load Himem code is present here.
 * |                     |
 * +---------------------+  Start of Lowmem Code @0x0000000C
 * |  Xchange Space      |  fw_part1_load_size variable @0x00000004 and
 * |                     |  fw_part2_load_addr_in_flash variable @0x00000008
 * +---------------------+
 * |  Reset Vector       |  4-byte Reset vector jump instruction @0x00000000
 * +---------------------+  0x00000000
 *
 *
 * GARD firmware loader when completely loaded in memory. This firmware loader
 * is loaded to TCM as a part of bitstream load:
 * +---------------------+ Top of memory  @ 64KB
 * |                     |
 * |      Stack          |
 * |                     |
 * +---------------------+ Start of Stack @ 56KB
 * |                     |
 * |   Himem Code        | Code containing OSPI module to load firmware up to
 * |                     | fw_part1_load_size.
 * |                     |
 * +---------------------+  End of BSS section
 * |      .BSS           |
 * |                     |
 * +---------------------+  Start of BSS section
 * +---------------------+  End of Initialized Data section
 * |                     |
 * |      .DATA          |
 * |                     |
 * +---------------------+  Start of Initialized Data section
 * |                     |
 * |   Unused Space      |
 * |                     |
 * +---------------------+  End of Lowmem Code
 * |                     |
 * |   Lowmem Code       | Init code to initialize firmware loader and hardware.
 * |                     |
 * +---------------------+  Start of Lowmem Code @0x00000000
 *
 * From both the memory maps above we can see that the firmware loader has the
 * Lowmem code and data at the lowest address while the Himem code is moved to
 * extreme top, right below the stack. On the other hand the GARD firmware has
 * the Lowmem, Himem and data sections compacted at the lower addresses while
 * the stack is at the top of memory.
 * This arrangement is done to ensure the GARD firmware can be loaded in stages
 * thus allowing the firmware loading to be shared by both the firmware loader
 * and the GARD firmware.
 */

#define TCM_START                   0x00000000U

#define TCM_SIZE                    (128 * 1024)

#define TCM_END                     (TCM_START + TCM_SIZE - 1)

#define STACK_SIZE                  (8 * 1024)  // 8KB of stack space.

/* Set the value of this define the start address of the firmware. */
#define FW_START_ADDR               0x00

/* TBD-SRP This should be taken from GARD Profile eventually. */
#define HRAM_START_ADDR             0x80000000

/* TBD-SRP This should be taken from GARD Profile eventually. */
#define HRAM_SIZE                   (16 * 1024 * 1024)

/* Metadata buffer start address */
#define METADATA_BUFFER_START_ADDR  0x0C000000

/* Metadata buffer size */
#define METADATA_BUFFER_START_SIZE  (16 * 1024)

/*
 * HRAM region layout keeps ML networks, ML IO buffers, scratch, and RISC-V
 * code/data in fixed partitions so the FW allocator can reason about each
 * segment independently.
 *
 * Size 	Module				Memory Offset
 * -----------------------------------------
 * 8MB		RISCV				0x00800000U
 * 2MB		ML IO + Scratch		0x00600000U
 * 6MB		ML Networks			0x00000000U
 */
#define HRAM_ML_NETWORKS_START_ADDR (HRAM_START_ADDR)
#define HRAM_ML_NETWORKS_SIZE       (6U * 1024U * 1024U)

/* ML IO window reserved for network input/output buffers and scratch. */
#define HRAM_ML_IO_START_ADDR                                                  \
	(HRAM_ML_NETWORKS_START_ADDR + HRAM_ML_NETWORKS_SIZE)
#define HRAM_ML_IO_SIZE            (2U * 1024U * 1024U)

/* 8MB assigned to the RISC-V subsystem. */
#define HRAM_RISCV_START_ADDR      (HRAM_ML_IO_START_ADDR + HRAM_ML_IO_SIZE)
#define HRAM_RISCV_SIZE            (8 * 1024 * 1024)

/**
 *
 * HRAM layout when firmware executes entirely out of HRAM:
 * +---------------------+ Top of HRAM @ HRAM_START_ADDR + HRAM_SIZE
 * |   PREINPUT Buffer   | 1 MB reserved for capture buffer (PREINPUT_SIZE)
 * +---------------------+ Start of PREINPUT buffer
 * +---------------------+ FW end address
 * |                     |
 * |   FW Code + Data    | Firmware text/data/bss occupying
 * |                     | from FW start up to (PREINPUT start - 1)
 * +---------------------+ FW start address
 * +---------------------+ Bottom of HRAM @ HRAM_START_ADDR
 *
 * With this arrangement all HRAM space from firmware start to firmware end
 * is available to hold executable code and data, while the final 1 MB remains
 * dedicated to the ML pre-input capture buffer.
 */

/* Start address for RISCV FW in HRAM */
#define HRAM_START_ADDR_FOR_FW     HRAM_START_ADDR

#define ML_APP_CAPTURE_BUFFER_SIZE (1 * 1024 * 1024)

#define HRAM_END_ADDR_FOR_FW                                                   \
	(HRAM_START_ADDR_FOR_FW + HRAM_RISCV_SIZE - ML_APP_CAPTURE_BUFFER_SIZE - 1)

/* Sanity check to ensure the HRAM partitions match the total size */
#define HRAM_LAYOUT_TOTAL_SIZE                                                 \
	(HRAM_ML_NETWORKS_SIZE + HRAM_ML_IO_SIZE + HRAM_RISCV_SIZE)

#if HRAM_LAYOUT_TOTAL_SIZE != HRAM_SIZE
#error "HRAM region sizes do not add up to total HRAM size"
#endif

#endif  // MEMMAP_H
