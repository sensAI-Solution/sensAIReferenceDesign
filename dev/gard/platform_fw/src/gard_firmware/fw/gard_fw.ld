/* Lattice Generated linker script, for normal executables */

/**
 * This file needs to be cleaned up later. This version is enough to generate
 * the elf and bin files for running on the riscv soft-IP.
 */

#include "memmap.h"
/**
 * The following includes disabled inclusion of files gard_types.h and
 * app_module.h as the contents within those files cannot be parsed by the
 * linker.
 */
#define GARD_TYPES_H
#define APP_MODULE_H
#include "fw_info.h"

ENTRY (_reset_vector)

/* Define the stack size */
STACK_END_ADDR = TCM_SIZE;
STACK_START_ADDR = STACK_END_ADDR - STACK_SIZE;

_HEAP_SIZE = DEFINED(_HEAP_SIZE) ? _HEAP_SIZE : 0x0100;

MEMORY
{
    sys_mem0_inst (rwx) : org = 0x0, len = TCM_SIZE
    HRAM          (rwx) : org = HRAM_START_ADDR, len = HRAM_SIZE
}

SECTIONS
{
  /* RESET VECTOR and XCHG DATA */
/**
 * The following .reset section has to match the same one in GARD FW loader
 * linker script.
 */
  .reset : ALIGN(4)
  {

    . = FW_START_ADDR;
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
  }

  .lowmem : ALIGN(4)
  {
    KEEP (*(SORT(.crt*)))
    *main.o(.text)
    PROVIDE (_sprof = .);
    . = ALIGN(4);
    _etext = .;
  } >sys_mem0_inst

  /* DATA */
  .rodata : ALIGN(4)
  {
    *(.rdata)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } >sys_mem0_inst

  .data : ALIGN(4)
  {
    *(.data .data.*)
    . = ALIGN(4);
    PROVIDE( __global_pointer$ = . + 0x800 );
    . = ALIGN(4);
  } >sys_mem0_inst

  .bss (NOLOAD) : ALIGN(4)
  {
    /* This is used by the startup in order to initialize the .bss secion */
    _bss_start = .;
    *(.sbss*)
    *(.bss .bss.*)
    *(COMMON)
    . = ALIGN(4);
    _bss_end = .;
  } >sys_mem0_inst

  .himem : ALIGN(4)
  {
    KEEP (*(.text))
    . = ALIGN(4);
  } >sys_mem0_inst

  _end_of_code_and_data_in_tcm = .;

  .hram HRAM_START_ADDR_FOR_FW : AT (_end_of_code_and_data_in_tcm) ALIGN(4)
  {
    _hram_start = .;
    KEEP(*(.text_in_hram))
    KEEP(*(.data_in_hram))
    . = ALIGN(4);
    _hram_end = .;
    ASSERT((_hram_end <= HRAM_END_ADDR_FOR_FW), "GARD Firmware code in HRAM is more than the size reserved for it in HRAM.");
  } >HRAM

/* Set aside space for the stack section */
  . = ORIGIN(sys_mem0_inst) + LENGTH(sys_mem0_inst) - STACK_SIZE;
  _stack_end = .;
  . = . + STACK_SIZE;
  _stack_start = .;
  /* END */
}
