/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef OSPI_SUPPORT_H
#define OSPI_SUPPORT_H

#include "octal_spi_controller.h"

/**
 * ospi_init initializes the OSPI controller and returns a handle to it.
 */
void *ospi_init(void) __attribute__((section(".lowmem")));

/**
 * ospi_read_from_flash reads num_bytes count of data from the flash starting
 * from flash_addr and writes that data to memory pointed by data_buf.
 */
uint32_t ospi_read_from_flash(void    *handle,
							  uint8_t *dat_buf,
							  uint32_t flash_addr,
							  uint32_t num_bytes);

#endif /* OSPI_SUPPORT_H */
