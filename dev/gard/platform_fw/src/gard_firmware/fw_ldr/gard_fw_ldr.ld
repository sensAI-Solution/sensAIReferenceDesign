/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/


#include "memmap.h"
#define GARD_TYPES_H
#define APP_MODULE_H
#include "fw_info.h"

/**
 * FW loader script is used to define the memory layout for the firmware loader.
 * The firmware loader is split into two parts:
 * 1. LowMem region which is locted at the lowest address range of the TCM.
 * 2. HighMem region which is located at the farthest address range of the TCM.
 *
 * Data and Stack along with some code are placed in the HighMem region.
 */

ENTRY (_reset_vector)

/* Define the stack size */
STACK_END_ADDR = TCM_SIZE;
STACK_START_ADDR = STACK_END_ADDR - STACK_SIZE;

_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x0100;

MEMORY
{
    sys_mem0_low_mem (rwx) : org = 0x0, len = TCM_SIZE - 22K
    sys_mem0_high_mem (rwx) : org = LENGTH(sys_mem0_low_mem), len = TCM_SIZE - LENGTH(sys_mem0_low_mem)
    HRAM           (rwx) : org = HRAM_START_ADDR, len = HRAM_SIZE
}

SECTIONS
{
  ASSERT((LENGTH(sys_mem0_low_mem) + LENGTH(sys_mem0_high_mem) == TCM_SIZE), "Memory Regions not declared correctly.")
  . = ORIGIN(sys_mem0_low_mem);

  /* RESET VECTOR and XCHG DATA */
/**
 * The following .reset section has to match the same one in GARD FW linker
 * script.
 */
  .reset : ALIGN(4)
  {
    KEEP (*(.rst_vect))

    . = (FW_START_ADDR + FW_PART1_SIZE_OFFSET);
    KEEP (*(.fw_part1_sz_section))

    . = (FW_START_ADDR + FW_PART2_START_ADDR_IN_FLASH_OFFSET);
    KEEP (*(.fw_part2_flash_addr_section))

    . = (FW_START_ADDR + FW_XIP_MODE_FLAG_OFFSET);
    KEEP (*(.fw_run_in_xip_flag_section))

    . = (FW_START_ADDR + FW_RELOC_TABLE_ENTRY_COUNT_OFFSET);
    KEEP (*(.fw_reloc_table_entry_count_section))

    . = (FW_START_ADDR + FW_RELOC_TABLE_OFFSET);
    KEEP (*(.fw_xip_reloc_table_section))

  } >sys_mem0_low_mem
  .lowmem : ALIGN(4)
  {
    KEEP (*(SORT(.crt*)))
    *main.o(.text)
    KEEP(*(.lowmem)) // Any code+data marked as part of .lowmem section is retained here.
    . = ALIGN(4);
  } >sys_mem0_low_mem

  /* DATA */
  .rodata : ALIGN(4)
  {
    _start_of_himem_area = ORIGIN(sys_mem0_high_mem);
    KEEP(*(.rdata))
    KEEP(*(.rodata .rodata.*))
    . = ALIGN(4);
  } >sys_mem0_high_mem

  .data : ALIGN(4)
  {
    *(.data .data.*)
    . = ALIGN(4);
    PROVIDE( __global_pointer$ = . + 0x800 );
    . = ALIGN(4);
  } >sys_mem0_high_mem

  .bss (NOLOAD) : ALIGN(4)
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _bss_start = .;
    *(.sbss*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } >sys_mem0_high_mem

  /* HI-MEM TEXT */
  .himem : ALIGN(4)
  {
    KEEP (*(.text))
    . = ALIGN(4);
  } >sys_mem0_high_mem


/* Set aside space for the stack section */
  ASSERT((. <= STACK_START_ADDR), "Himem is hitting the Stack section. Lower the Himem start by decreasing the size of sys_mem0_low_mem in the linker script. For exact overlap please consult the generated map file.")
  . = STACK_START_ADDR;
  _stack_end = .;
  . = . + STACK_SIZE;
  _stack_start = .;
  /* END */
}
