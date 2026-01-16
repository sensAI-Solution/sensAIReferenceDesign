/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_UART_H__
#define __HUB_UART_H__

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/select.h>

#include "gard_info.h"

/**
 * Max chunk size in bytes to be read from GARD UART @ 921600.
 * Empirical value found via trial and error.
 */
#define HUB_GARD_UART_READ_CHUNK_SZ	(64)

/**
 * Open a UART bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 *
 * Any further operation on the said UART bus has to use this bus handle.
 */
int32_t hub_uart_device_open(void *param);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 * Perform a read operation on the UART device on the given bus
 * handle.
 */
int32_t hub_uart_device_read(int uart_bus_hdl, void *p_buffer, uint32_t count);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 * Perform a write operation on the UART device on the given bus
 * handle.
 */
int32_t hub_uart_device_write(int         uart_bus_hdl,
							  const void *p_buffer,
							  uint32_t    count);

/**
 * TBD-DPN: Revisit the return code for this call.
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Close the UART bus opened by the open call.
 */
int32_t hub_uart_device_close(int uart_bus_hdl);

#endif /* __HUB_UART_H__ */
