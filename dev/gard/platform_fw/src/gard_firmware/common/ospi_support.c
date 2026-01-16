/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "ospi_support.h"
#include "octal_spi_controller.h"

spix8_ctl_handle_t octal_spi_c0;

static void        spix8_param_init(void)
{
	uint32_t            sck_rate;
	spix8_ctl_handle_t *p_octal_spi_c0 = &octal_spi_c0;

#ifdef XIP_TEST
	sck_rate = QSPI_FLASH_C0_INST_SPI_SCKDIV;
#else
	sck_rate = 5U;
#endif

	p_octal_spi_c0->init_done = FAILURE;
	p_octal_spi_c0->base_addr =
		0x40006000U;  // SYSTEM_OSPI_FC_INST_OCTAL_SPI_CONTROLLER_MEM_MAP_BASE_ADDR;
	p_octal_spi_c0->max_num_lane      = 4U;
	p_octal_spi_c0->sys_clk_freq      = 50;
	p_octal_spi_c0->spi_io_width      = SPIX8_IO_X1;

	p_octal_spi_c0->sck_rate          = sck_rate & 0x1F;
	p_octal_spi_c0->autoclr_txstart   = 0;
	p_octal_spi_c0->autoclr_softrst   = 1;
	p_octal_spi_c0->lsb_first         = 0;
	p_octal_spi_c0->spi_mode          = 0; /*{cpol,cpha}*/
	p_octal_spi_c0->endianness        = 0;
	p_octal_spi_c0->en_tgtaddr_map    = 0;
	p_octal_spi_c0->use_ds_in_ddr     = 1;
	p_octal_spi_c0->non_block_txfifo  = 0;
	p_octal_spi_c0->non_block_rxfifo  = 0;

	p_octal_spi_c0->spi_dat_rate      = SPIX8_STR;
	p_octal_spi_c0->axi4_tgt_baddr    = 0x40006000U;
	p_octal_spi_c0->flash_addr_offset = 0x00U;

	p_octal_spi_c0->interrupt_enable  = SPIX8_INT_BUS_ACCESS_ERROR |
									   SPIX8_INT_FLASH_PROGRAM_FAIL |
									   SPIX8_INT_FLASH_ERASE_FAIL;

#ifdef _WINBOND_X4_FLASH_DEVICE_
	p_octal_spi_c0->flash_quad_en     = FLASH_EXTSPI_ENABLE;
	p_octal_spi_c0->flash_dummy_cycle = 0x00;
	p_octal_spi_c0->flash_wrap_cfg    = 0x00;
#else
	p_octal_spi_c0->flash_dummy_cycle = 0x1FU;
	p_octal_spi_c0->flash_wrap_cfg    = 256;
#endif /*_WINBOND_X4_FLASH_DEVICE_*/
	p_octal_spi_c0->flash_addr_mode = FLASH_ADDR_MODE_24B;
	p_octal_spi_c0->spi_actual_freq =
		(uint32_t)((sck_rate == 0) ? 50 : (50 / (2 * sck_rate)));
}

/**
 * ospi_init initializes the OSPI controller and returns a handle to it.
 *
 * This function sets up the OSPI controller with the necessary parameters
 * and returns a pointer to the initialized controller handle.
 *
 * @return Returns a pointer to the initialized OSPI controller handle,
 *         or NULL if initialization fails.
 */
void *ospi_init(void)
{
	op_param_t op_param;

	/* OSPI Interfacing */
	(void)spix8_param_init();

	if (spix8_flash_ctl_init(&octal_spi_c0, 0x40006000U, 0x00U)) {
		octal_spi_c0.init_done = SUCCESS;
		// printf("Done.\r\n");
	} else {
		return NULL;
	}

	op_param = (op_param_t){
		FLASH_CMD_FAST_READ, SPIX8_IO_X1, SPIX8_IO_X1, SPIX8_IO_X1, 0, 0, 0, 8};
	set_op_param(&op_param);

	return &octal_spi_c0;
}

/**
 * ospi_read_from_flash reads num_bytes count of data from the flash starting
 * from flash_addr. The read data is stored in dat_buf. The function returns the
 * number of bytes that were successfully read from the flash and written to the
 * data buffer.
 *
 * @param handle is the OSPI controller handle.
 * @param dat_buf is the pointer to the data buffer where the read data will be
 * stored.
 * @param flash_addr is the starting address in the flash memory from where the
 * data will be read.
 * @param num_bytes is the number of bytes to read from the flash memory.
 *
 * @return Returns the number of bytes read from the flash memory.
 *
 *
 */
uint32_t ospi_read_from_flash(void    *handle,
							  uint8_t *dat_buf,
							  uint32_t flash_addr,
							  uint32_t num_bytes)
{
	spix8_ctl_handle_t *p_handle    = handle;
	uint8_t             status      = SUCCESS;
	uint32_t            t_num_bytes = num_bytes;
	uint32_t            bytes_to_read;

	GARD__DBG_ASSERT(p_handle != NULL && num_bytes > 0,
					 "Invalid parameters provided to ospi_read_from_flash");

	/**
	 * Read the data in small chunks of 256 bytes as that is the one currently
	 * working. Later we will debug this limitation to remove it.
	 */
	do {
		bytes_to_read = (t_num_bytes > FLASH_READ_CHUNK_SIZE)
							? FLASH_READ_CHUNK_SIZE
							: t_num_bytes;
		status = flash_read(p_handle, bytes_to_read, (unsigned int *)dat_buf,
							flash_addr, SPIX8_GEN_CMD, 0 /*no_ds*/);
		t_num_bytes -= bytes_to_read;
		dat_buf     += bytes_to_read;
		flash_addr  += bytes_to_read;
	} while ((status == SUCCESS) && (t_num_bytes > 0));

	return (status == SUCCESS) ? num_bytes : 0;
}
