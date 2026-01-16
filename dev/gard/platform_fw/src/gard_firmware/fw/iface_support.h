/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef IFACE_SUPPORT_H
#define IFACE_SUPPORT_H

#include "gard_types.h"
#include "uart.h"
#include "i2c_slave.h"
#include "gard_hub_iface.h"
#include "gard_hub_iface_unpacked.h"

/**
 * Currently we support maximum of 2 interfaces, those could be all UARTs, all
 * I2Cs or one UART and one I2C.
 */
#define MAX_IFACES_SUPPORTED 2

/**
 * This structure iface_instance is used to hold all the variables related to
 * handling the communication with interface.
 *
 * This structure is divided into sections which contain data and functions for
 * 1) host_cmd handling functions,
 * 2) interface support functions, and
 * 3) the actual interface driver (UART or I2C).
 */
struct iface_instance {
	bool is_active;

	/**
	 * Following data is hosted for the module host_cmds.
	 */
	struct {
		// Flag to indicate if requested data is available to host_cmds module
		// for reading.
		// This flag is read and written by the routines within host_cmds module
		// and should not be accessed directly by other modules; instead the
		// routine set_rx_done() should be used to set this flag.
		bool rx_done;

		// Flag to indicate if requested data to be sent by the host_cmds module
		// has been dispatched by the driver. The data could still be present in
		// the FIFO which the hardware will send at its tuned frequency.
		// This flag is read and written by the routines within host_cmds module
		// and should not be accessed directly by other modules; instead the
		// routine set_tx_done() should be used to set this flag.
		bool tx_done;

		// Current state of the host request service state machine.
		uint32_t                      host_request_service_state;

		struct _host_requests_unpked  host_req;
		struct _host_responses_unpked host_resp;
		struct _host_requests         iface_host_req;
	} hc_data;

	/**
	 * Following data is hosted for the module iface_support
	 */
	struct {
		bool     (*read_data_async_call)(struct iface_instance *inst,
                                     uint32_t               byte_count,
                                     uint8_t               *p_data_buffer);
		bool     (*send_data_async_call)(struct iface_instance *inst,
                                     uint32_t               byte_count,
                                     const uint8_t         *p_data_buffer);

		uint32_t rx_handler_state;
		uint32_t bytes_read;
		uint32_t bytes_requested;
		uint8_t *p_rx_data_buffer;

		uint32_t tx_handler_state;
		uint32_t bytes_to_send;
		uint32_t bytes_sent;
		uint8_t *p_tx_data_buffer;
	};

	/**
	 * 	Following data is hosted for the bsp driver controlling the interface.
	 */
	/**
	 * The following data is used to hold the actual interface instance (UART or
	 * I2C) and their function calls. Only one of them is valid in each
	 * instance. The interface instance is defined in the bsp files and is used
	 * to interact with the hardware.
	 */
	struct {
		uint32_t (*iface_getchars)(void    *this_uart,
								   uint8_t *buffer,
								   uint32_t count);

		uint32_t (*iface_putchars)(void    *this_uart,
								   uint8_t *buffer,
								   uint32_t count);

		union {
			uint8_t iface_inst[0];  // Interface instance (UART or I2C).
			struct uart_instance      uart_inst;  // UART instance.
			struct i2c_slave_instance i2c_inst;   // I2C slave instance.
		};
	} bsp_data;
};

/**
 * read_data_async collects the parameters passed by the caller to capture
 * data from IFACE.
 */
bool read_data_async(struct iface_instance *inst,
					 uint32_t               byte_count,
					 uint8_t               *p_data_buffer);

/**
 * rx_handler runs as a state machine to handle IFACE data reception.
 */
bool execute_rx_handlers(void);

/**
 * send_data_async collects the parameters passed by the caller to send
 * data over IFACE.
 */
bool send_data_async(struct iface_instance *inst,
					 uint32_t               byte_count,
					 const uint8_t         *p_data_buffer);

/**
 * tx_handler runs as a state machine to handle IFACE data transmission.
 */
bool execute_tx_handlers(void);

/**
 * ifaces_init initializes the interfaces (UART or I2C)
 */
bool ifaces_init(void);

/**
 * iface_get_count returns the number of interfaces available for use.
 */
uint32_t iface_get_count(void);

/**
 * get_iface_handle returns the handle to the interface instance at index
 * iface_index.
 */
struct iface_instance *get_iface_handle(uint32_t iface_index);

#endif  // IFACE_SUPPORT_H
