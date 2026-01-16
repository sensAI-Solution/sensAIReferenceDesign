/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "uart.h"
#include "i2c_slave.h"
#include "host_cmds.h"
#include "sys_platform.h"
#include "iface_support.h"
#include "fw_globals.h"

enum rx_states {
	RX_CHECK_DATA_AVAILABLE = 1,  // Check if data is available
	RX_DATA_FROM_INTERFACE,       // Read data from interface
};

enum tx_states {
	TX_CHECK_DATA_TO_SEND = 1,  // Check if data to send is available
	TX_DATA_TO_INTERFACE,       // Send data over interface
};

/**
 * iface_init initializes the interfaces (UART and I2C) used by the firmware.
 * It sets up the UART and I2C instances with the required parameters and
 * marks them as valid if initialization is successful.
 *
 * @return true if initialization was successful, false otherwise.
 */
bool ifaces_init(void)
{
	struct iface_instance *inst;
	uint32_t               idx;

	inst = iface_inst;
	for (idx = 0; idx < MAX_IFACES_SUPPORTED; idx++) {
		inst->is_active = false;
		inst++;
	}

	valid_ifaces = 0;

	inst         = iface_inst;
	// Initialize the INTERFACE instance with the required parameters.
	if (uart_init(&inst->bsp_data.uart_inst, UART_INST_BASE_ADDR,
				  UART_INST_SYS_CLK * 1000000, UART_INST_BAUD_RATE, 1,
				  8) == 0) {
		valid_ifaces++;
		inst->is_active               = true;

		// The following assignments should ideally be done in bsp driver. Just
		// to keep the changes to the driver minimal we do the assignments here.
		inst->bsp_data.iface_getchars = uart_getchars;
		inst->bsp_data.iface_putchars = uart_putchars;
		inst++;
	}

	// Initialize the I2C slave instance with the required parameters.
	if (i2c_slave_init(&inst->bsp_data.i2c_inst,
					   (uint32_t)I2C_SLAVE0_INST_BASE_ADDR,
					   I2C_SLAVE0_INST_SLAVE_ADDR,
					   I2C_SLAVE0_INST_ADDR_MODE) == I2C_SLV_SUCCESS) {
		valid_ifaces++;
		inst->is_active               = true;

		// The following assignments should ideally be done in bsp driver. Just
		// to keep the changes to the driver minimal we do the assignments here.
		inst->bsp_data.iface_getchars = i2c_getchars;
		inst->bsp_data.iface_putchars = i2c_putchars;
		inst++;
	}

	inst = iface_inst;
	for (idx = 0; idx < valid_ifaces; idx++) {
		// Assign the read/send data async functions
		inst->read_data_async_call = read_data_async;
		inst->send_data_async_call = send_data_async;

		inst->rx_handler_state     = RX_CHECK_DATA_AVAILABLE;
		inst->bytes_read           = 0U;
		inst->bytes_requested      = 0U;
		inst->p_rx_data_buffer     = NULL;

		inst->tx_handler_state     = TX_CHECK_DATA_TO_SEND;
		inst->bytes_to_send        = 0U;
		inst->bytes_sent           = 0U;
		inst->p_tx_data_buffer     = NULL;

		inst++;
	}

	return (valid_ifaces > 0);
}

/**
 * iface_get_count returns the number of interfaces are available for use.
 *
 * @return the number of valid interfaces available for use.
 */
uint32_t iface_get_count(void)
{
	return valid_ifaces;
}

/**
 * get_iface_handle returns the handle to the interface instance based on the
 * provided index.
 *
 * @param iface_index: Index of the interface instance.
 *
 * @return Pointer to the iface_instance structure if index is valid, NULL
 *         otherwise.
 */
struct iface_instance *get_iface_handle(uint32_t iface_index)
{
	if (iface_index < valid_ifaces) {
		return &iface_inst[iface_index];
	}

	return NULL;
}

/**
 * read_data_async collects the parameters passed by the caller to
 * capture data from interface. The rx_handler will use these parameters to read
 * data from the interface FIFO when it is called.
 *
 * @param inst: Pointer to the interface instance from where the data will be
 *				read.
 * @param byte_count: Number of bytes to read from interface.
 * @param p_data_buffer: Pointer to the buffer where the read data will be
 * 						 stored.
 *
 * @return true if the request was accepted, false if there was an error
 *         (e.g., invalid parameters or another request is already in progress).
 */
bool read_data_async(struct iface_instance *inst,
					 uint32_t               byte_count,
					 uint8_t               *p_data_buffer)
{
	if (byte_count == 0U || p_data_buffer == NULL) {
		return false;  // Invalid parameters, nothing to do.
	}

	if (inst->p_rx_data_buffer != NULL) {
		// If there is already a request in progress, we cannot accept a new
		// one.
		return false;
	}

	inst->bytes_requested  = byte_count;
	inst->p_rx_data_buffer = p_data_buffer;
	inst->bytes_read       = 0;

	return true;
}

/**
 * rx_handler runs as a state machine to handle interface data reception.
 * It checks if data is available in the interface FIFO and reads it into the
 * specified buffer. It also manages the state transitions based on whether
 * data is available or if the read operation is complete.
 *
 * @param handle: Pointer to the interface instance.
 *
 * @return true if data was read successfully, false if no data was received
 *         in this iteration or if no data was requested.
 */
bool rx_handler(struct iface_instance *inst)
{
	uint32_t temp_count;
	uint32_t bytes_read_orig;

	switch (inst->rx_handler_state) {
	case RX_CHECK_DATA_AVAILABLE:
		if (inst->bytes_requested == 0) {
			break;  // No data requested, nothing to do.
		}

		inst->rx_handler_state = RX_DATA_FROM_INTERFACE;
		// Fall through to check if data is available in the interface FIFO.

	case RX_DATA_FROM_INTERFACE:

		bytes_read_orig = inst->bytes_read;

		// Read interface FIFO till the requested number of bytes are received.
		while ((inst->bytes_read < inst->bytes_requested) &&
			   (temp_count = inst->bsp_data.iface_getchars(
					inst->bsp_data.iface_inst, inst->p_rx_data_buffer,
					inst->bytes_requested - inst->bytes_read)) != 0) {
			// Read as many bytes as available in the interface FIFO.
			inst->p_rx_data_buffer += temp_count;
			inst->bytes_read       += temp_count;
		}

		if (inst->bytes_read == inst->bytes_requested) {
			// Read completed, now notify the caller that data read from
			// interface is done.
			set_rx_done(inst);

			// Reset the request after reading data.
			inst->bytes_read = inst->bytes_requested = 0U;
			inst->p_rx_data_buffer                   = NULL;
			inst->rx_handler_state                   = RX_CHECK_DATA_AVAILABLE;
		} else if (inst->bytes_read == bytes_read_orig) {
			// No data read, in this iteration. In poll-mode we return true
			// but in interrupt mode we will return false so that
			// the CPU can enter low-power mode if no activity is detected on
			// interface.
			return true;
		}

		return true;  // Data read successfully.

	default:
		GARD__DBG_ASSERT(
			false, "Invalid RX state: %d",
			((enum rx_states)inst->rx_handler_state));  // Invalid state.

		// Reset to the initial state.
		inst->rx_handler_state = RX_CHECK_DATA_AVAILABLE;
	}

	return false;
}

/**
 * send_data_async collects the parameters passed by the caller to send
 * data over interface. The tx_handler will use these parameters to send data
 * to the interface FIFO when it is called.
 *
 * @param byte_count: Number of bytes to send over interface.
 * @param p_data_buffer: Pointer to the buffer containing the data to be sent.
 *
 * @return true if the request was accepted, false if there was an error
 *         (e.g., invalid parameters or another request is already in progress).
 */
bool send_data_async(struct iface_instance *inst,
					 uint32_t               byte_count,
					 const uint8_t         *p_data_buffer)
{
	if (byte_count == 0U || p_data_buffer == NULL) {
		return false;  // Invalid parameters, nothing to do.
	}

	if (inst->p_tx_data_buffer != NULL) {
		// If there is already a request in progress, we cannot accept a new
		// one.
		return false;
	}

	inst->bytes_to_send    = byte_count;
	inst->p_tx_data_buffer = (uint8_t *)p_data_buffer;
	inst->bytes_sent       = 0U;

	return true;
}

/**
 * tx_handler runs as a state machine to handle interface data transmission.
 * It checks if there is data to send and sends it over the interface FIFO.
 * It also manages the state transitions based on whether data is available
 * to send or if the send operation is complete.
 *
 * @param handle: Pointer to the interface instance.
 *
 * @return true if data was sent, false if no data was sent in this iteration.
 */
bool tx_handler(struct iface_instance *inst)
{
	uint32_t temp_count;
	uint32_t bytes_sent_orig = inst->bytes_sent;

	switch (inst->tx_handler_state) {
	case TX_CHECK_DATA_TO_SEND:
		if (inst->bytes_to_send == 0) {
			break;  // No data to send, nothing to do.
		}

		// Fall through to send data.

	case TX_DATA_TO_INTERFACE:
		bytes_sent_orig = inst->bytes_sent;

		// Send data over interface till the requested number of bytes are sent.
		while ((temp_count = inst->bsp_data.iface_putchars(
					inst->bsp_data.iface_inst, inst->p_tx_data_buffer,
					inst->bytes_to_send - inst->bytes_sent)) != 0) {
			// Send as many bytes as available in the interface FIFO.
			inst->p_tx_data_buffer += temp_count;
			inst->bytes_sent       += temp_count;
		}

		if (inst->bytes_sent == inst->bytes_to_send) {
			// All data sent, notify the caller that data send is done.
			set_tx_done(inst);

			inst->bytes_to_send = inst->bytes_sent = 0U;
			inst->p_tx_data_buffer                 = NULL;
			inst->tx_handler_state                 = TX_CHECK_DATA_TO_SEND;
		} else if (inst->bytes_sent == bytes_sent_orig) {
			// No data sent, in this iteration. In poll-mode we return true
			// but in interrupt mode we will return false so that
			// the CPU can enter low-power mode if no activity is detected on
			// interface.
			return true;
		}
		break;
	}

	return false;
}

/**
 * execute_rx_handlers runs the RX handlers for all active interfaces.
 * It checks if data is available on each interface and processes it.
 *
 * @return true if any work was done (data read), false otherwise.
 */
bool execute_rx_handlers(void)
{
	bool                   work_done = false;
	struct iface_instance *inst;

	inst = &iface_inst[0];
	for (uint32_t idx = 0; idx < valid_ifaces; idx++) {
		if (inst->is_active) {
			if (rx_handler(inst)) {
				work_done = true;
			}
		}
		inst++;
	}

	return work_done;
}

/**
 * execute_tx_handlers runs the TX handlers for all active interfaces.
 * It checks if there is data to send on each interface and processes it.
 *
 * @return true if any work was done (data sent), false otherwise.
 */
bool execute_tx_handlers(void)
{
	bool                   work_done = false;
	struct iface_instance *inst;

	inst = &iface_inst[0];
	for (uint32_t idx = 0; idx < valid_ifaces; idx++) {
		if (inst->is_active) {
			if (tx_handler(inst)) {
				work_done = true;
			}
		}
		inst++;
	}

	return work_done;
}
