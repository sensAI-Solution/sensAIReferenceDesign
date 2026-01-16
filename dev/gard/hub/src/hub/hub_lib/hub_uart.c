/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_uart.h"

/**
 * TBD-DPN: The current release does not have an OS abstraction layer (OSAL).
 * In subsequent releases, we will have a HUB OSAL to take care of all
 * operations performed by the underlying OS on behalf of / for HUB.
 */

/**
 * TBD-DPN: Currently, we support only 1 UART.
 * This logic will be changed when we add support for multiple UARTs
 */
static struct hub_gard_bus_uart_props *p_uart_ctx = NULL;

/**
 * Open a UART bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 *
 * Any further operation on the said UART bus has to use this bus handle.
 *
 * @param: param is an opaque pointer to the bus's properties.
 *
 * @return: 0/positive integer indicates success, and is the bus's handle
 * 		 -1 for failure
 */
int32_t hub_uart_device_open(void *param)
{
	int            bus_hdl;
	struct termios uart_tty;
	int            ret;
	speed_t        bus_speed;

	p_uart_ctx       = (struct hub_gard_bus_uart_props *)param;

	char *p_uart_bus = p_uart_ctx->bus_dev;

	if (p_uart_ctx->is_open) {
		/* Check if the file descriptor is still valid */
		if (p_uart_ctx->bus_hdl >= 0) {
			/* TBD-DPN: Revisit this logic to make it OS agnostic */
			/* Try to verify the fd is still open by checking with fcntl */
			if (fcntl(p_uart_ctx->bus_hdl, F_GETFD) >= 0) {
				hub_pr_dbg("Bus already open with bus_hdl:%d\n", p_uart_ctx->bus_hdl);
				return p_uart_ctx->bus_hdl;
			} else {
				/* File descriptor is invalid, reset state */
				hub_pr_dbg("Previous bus_hdl %d is invalid, reopening\n", p_uart_ctx->bus_hdl);
				p_uart_ctx->is_open = false;
				p_uart_ctx->bus_hdl = -1;
			}
		} else {
			/* Invalid handle, reset state */
			p_uart_ctx->is_open = false;
		}
	}

	/* TBD-DPN: Remove this restriction on baudrate */
	switch (p_uart_ctx->baudrate) {
	case 9600:
		bus_speed = B9600;
		break;
	case 115200:
		bus_speed = B115200;
		break;
	case 921600:
		bus_speed = B921600;
		break;
	default:
		hub_pr_err("Baudrate %d not supported\n", p_uart_ctx->baudrate);
		goto err_uart_open_1;
	}

	bus_hdl = open(p_uart_bus, O_RDWR);
	if (bus_hdl < 0) {
		hub_pr_err("Error opening %s\n", p_uart_bus);
		goto err_uart_open_1;
	}

	if (tcgetattr(bus_hdl, &uart_tty)) {
		hub_pr_err("Error in tcgetattr\n");
		goto err_uart_open_2;
	}

	/* TBD-DPN: Set up other flags in struct termios as pertinent */

	ret = cfsetispeed(&uart_tty, bus_speed);
	if (0 != ret) {
		hub_pr_err("Error in setispeed\n");
		goto err_uart_open_2;
	}

	ret = cfsetospeed(&uart_tty, bus_speed);
	if (0 != ret) {
		hub_pr_err("Error in setospeed\n");
		goto err_uart_open_2;
	}

	/**
	 * Termios settings matched exactly with minicom for consistency.
	 * Minicom uses completely raw mode with all input flags disabled.
	 */
	/* All input flags disabled - completely raw mode (matches minicom) */
	uart_tty.c_iflag = 0x0;
	/* Raw output - no processing (matches minicom) */
	uart_tty.c_oflag = 0x0;
	/* Configure c_cflag to match minicom: CS8, no parity, 1 stop bit */
	/* Clear size, parity, stop bits, hardware flow control */
	uart_tty.c_cflag &= ~(CSIZE | PARENB | CSTOPB | CRTSCTS | HUPCL);
	/* Set 8 data bits */
	uart_tty.c_cflag |= CS8;
	/* Note: minicom shows -cread -clocal when device is closed,
	 * but typically enables CREAD when opened for reading.
	 * We enable CREAD for receiver functionality. */
	uart_tty.c_cflag |= CREAD;
	/* Raw input - no echo, no canonical mode (matches minicom) */
	uart_tty.c_lflag = 0x0;

	/* VMIN=0, VTIME=0: Non-blocking mode (matches minicom) */
	/* We use select() in the read function to wait for data with timeout,
	 * matching minicom's approach */
	uart_tty.c_cc[VMIN] = 0;
	uart_tty.c_cc[VTIME] = 0;
	
	tcsetattr(bus_hdl, TCSANOW, &uart_tty); 

	hub_pr_dbg("Opened %s with bus_hdl:%d\n", p_uart_bus, bus_hdl);

	p_uart_ctx->bus_hdl = bus_hdl;
	p_uart_ctx->is_open = true;

	return bus_hdl;

err_uart_open_2:
	close(bus_hdl);
err_uart_open_1:
	p_uart_ctx = NULL;
	return -1;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 * Perform a write operation on the UART device on the given bus
 * handle.
 *
 * @param: uart_bus_hdl is the handle to the UART bus
 * @param: p_buffer is a pointer to a buffer with data to write
 * @param: count is the number of bytes to be written
 *
 * @return: Number of bytes written.
 * We also print a debug error if this number is not equal to the count
 */
int32_t hub_uart_device_write(int         uart_bus_hdl,
							  const void *p_buffer,
							  uint32_t    count)
{
	ssize_t nwrite;
	int ret;

	nwrite = write(uart_bus_hdl, p_buffer, count);
	if (nwrite != count) {
		hub_pr_err("Error writing to %d\n", uart_bus_hdl);
		goto err_write;
	}

	/* Ensure all data is transmitted before returning */
	ret = tcdrain(uart_bus_hdl);
	if (ret != 0) {
		hub_pr_err("Error draining UART output\n");
		goto err_write;
	}

	hub_pr_dbg("Wrote %ld bytes\n", nwrite);

err_write:
	return nwrite;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 * Also, change the bus_hdl to a bus context for supporting multiple
 * instances of the same bus.
 *
 * Perform a read operation on the UART device on the given bus
 * handle.
 *
 * @param: uart_bus_hdl is the handle to the UART bus
 * @param: p_buffer is a pointer to a buffer to be filled by the read
 * @param: count is the number of bytes to be read
 *
 * @return: Number of bytes read by the call.
 * We also print a debug error if this number is not equal to the count
 */
int32_t hub_uart_device_read(int uart_bus_hdl, void *p_buffer, uint32_t count)
{
	int ret;

	uint32_t bytes_left = count;
	ssize_t nread, to_read;
	ssize_t total_bytes = 0;
	
	fd_set read_fds;
	
	struct timeval timeout;
	const int timeout_seconds = 2;  /* 2 second timeout per select call */
	
	const int max_retries = 3;
	int retry_count = 0;
	
	do {
		to_read = hub_min_uint32(bytes_left, HUB_GARD_UART_READ_CHUNK_SZ);

		/* Use select() to wait for data availability (matching minicom approach) */
		FD_ZERO(&read_fds);
		FD_SET(uart_bus_hdl, &read_fds);
		timeout.tv_sec = timeout_seconds;
		timeout.tv_usec = 0;
		
		ret = select(uart_bus_hdl + 1, &read_fds, NULL, NULL, &timeout);
		
		if (ret == -1) {
			hub_pr_err("UART select error: %s\n", strerror(errno));
			goto err_read;
		}
		
		if (ret == 0) {
			/* Timeout */
			retry_count++;
			if (retry_count >= max_retries) {
				hub_pr_err("UART read timeout after %d retries (no data available)\n", max_retries);
				goto err_read;
			}
			/* Retry the select */
			continue;
		}
		
		/* Data is available, perform the read */
		if (FD_ISSET(uart_bus_hdl, &read_fds)) {
			nread = read(uart_bus_hdl, p_buffer, to_read);
			
			if (nread == -1) {
				hub_pr_err("UART read error: %s\n", strerror(errno));
				goto err_read;
			}
			
			if (nread == 0) {
				/* EOF or no data (shouldn't happen after select, but handle it) */
				retry_count++;
				if (retry_count >= max_retries) {
					hub_pr_err("UART read returned 0 after select (EOF)\n");
					goto err_read;
				}
				continue;
			}
			
			/* Reset retry count on successful read */
			retry_count = 0;
			
			total_bytes += nread;
			bytes_left -= nread;
			p_buffer += nread;
		}
	} while (total_bytes < count);

	hub_pr_dbg("Read %zd bytes\n", total_bytes);

err_read:
	return total_bytes;
}

/**
 * TBD-DPN: Revisit the return code for this call.
 * Also, change the bus_hdl to a bus context for supporting mutiple
 * instances of the same bus.
 *
 * Close the UART bus opened by the open call.
 *
 * @param: uart_bus_hdl is the handle to the open UART bus.
 *
 * @return: 0
 */
int32_t hub_uart_device_close(int uart_bus_hdl)
{
	/* Always try to close the file descriptor, even if p_uart_ctx is NULL */
	/* This handles cases where cleanup is called after an interrupt */
	if (uart_bus_hdl >= 0) {
		close(uart_bus_hdl);
		hub_pr_dbg("Closed bus_hdl %d\n", uart_bus_hdl);
	}

	if (NULL != p_uart_ctx) {
		p_uart_ctx->bus_hdl = -1;
		p_uart_ctx->is_open = false;
		p_uart_ctx          = NULL;
	}

	return 0;
}
