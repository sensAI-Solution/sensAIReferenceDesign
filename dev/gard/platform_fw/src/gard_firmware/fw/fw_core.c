/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "fw_core.h"
#include "ml_ops.h"
#include "hw_regs.h"
#include "ospi_support.h"
#include "rfs.h"
#include "utils.h"
#include "assert.h"
#include "app_module.h"
#include "fw_globals.h"
#include "gpio_mapper.h"

/**
 * schedule_image_processing_done_event() is used by the App Module to indicate
 * that the image processing is done and the FW Core should call the
 * app_image_processing_done() function.
 * This function sets a flag that is checked in the FW Core's main loop to
 * determine if the app_image_processing_done() function should be called.
 *
 * @return None
 */
void schedule_image_processing_done_event(void)
{
	/**
	 * This function sets a flag to indicate that the App Module provided
	 * function app_image_processing_done() should be called from the main loop.
	 */
	call_app_image_processing_done = true;
}

/**
 * register_buffer_for_host_data() is used by the App Module to register a
 * buffer that will be used to receive data from the App Module's counterpart
 * running on the Host. Additionally, it also registers a callback
 * function that will be invoked by the FW Core when data is received from the
 * Host.
 *
 * @param buffer points to the buffer that will be used to receive data.
 * @param buffer_size is the size of the buffer in bytes.
 * @param rx_handler is the callback function to handle received data.
 *
 * @return Pointer to the registered buffer.
 */
void *register_buffer_for_host_data(uint8_t     *buffer,
									uint32_t     buffer_size,
									rx_handler_t rx_handler)
{
	GARD__DBG_ASSERT((NULL != buffer) && (buffer_size > 0U) &&
						 (NULL != rx_handler),
					 "Invalid buffer or buffer size or rx_handler");

	/**
	 * Store the buffer and its size in the global variables so that FW Core
	 * can use them to receive data from the Host.
	 */
	app_rx_buffer      = buffer;
	app_rx_buffer_size = buffer_size;
	app_rx_handler     = rx_handler;

	return app_rx_buffer;
}

/**
 * stream_data_to_host_async() is used by the App Module to send streaming data
 * to the Host. Since the data will be sent asynchronously, the App Module
 * should ideally have another buffer to accumulate new data while the previous
 * data is still being sent. The variable pointed by an optional p_send_complete
 * will be set to true once all the data from this buffer has been sent to the
 * Host. The App Module should have set this variable to false before calling
 * this function.
 *
 * @param data points to the data to be sent.
 * @param count_of_data_bytes is the number of bytes of data to be sent.
 * @param timeout_ms is the timeout in milliseconds for the operation.
 * @param p_send_complete points to a boolean that will be set to true when
 *                        the data has been sent successfully. This is optional
 *                        and can be NULL if the App Module does not want to
 * know when the data has been sent.
 *
 * @return None.
 */
void stream_data_to_host_async(uint8_t *data,
							   uint32_t count_of_data_bytes,
							   uint32_t timeout_ms,
							   uint8_t *p_send_complete)
{
	GARD__DBG_ASSERT((NULL != data) && (count_of_data_bytes > 0U),
					 "Invalid data or count_of_data_bytes or p_send_complete");
	GARD__DBG_ASSERT(
		(NULL == p_send_complete) || (!*p_send_complete),
		"*p_send_complete should be set to false before calling this function");

	/**
	 * The current implementation notes the buffer address and its size in
	 * global variables so that FW Core can send this data to the Host in the
	 * background. The background sending of data is done by the
	 * RECV_DATA_FROM_GARD_AT_OFFSET command handler that is part of the
	 * interface support module. For this we need to let HUB know about the
	 * availability of this data, we do this by toggling a GPIO pin that HUB is
	 * monitoring.
	 */
	app_tx_buffer      = data;
	app_tx_buffer_size = count_of_data_bytes;

	SET_GPIO_HIGH_TO_HOST_IRQ();
	delay(1);
	SET_GPIO_LOW_TO_HOST_IRQ();
	/* TBD-SRP - Currently we ignore the timeout value. */

	p_app_tx_complete = p_send_complete;
}

/**
 * read_module_data() reads the data from the module into the provided buffer.
 *
 * @param module_uid parameter specifies the UID of the module to read.
 * @param module_read_offset parameter specifies the offset from the start of
 * 							the module to read the data from. module_read_offset
 * 							should 32-bit aligned.
 * @param read_bytes parameter specifies the number of bytes to read from the
 * 							module. read_bytes should be multiple of 4.
 * @param buffer parameter specifies the buffer to read the data into.
 *
 * @return count of bytes of the module read from flash into the buffer. In case
 * 		   of error returns 0.
 */
uint32_t read_module_data(uint32_t module_uid,
						  uint32_t module_read_offset,
						  uint32_t read_bytes,
						  uint8_t *buffer)
{
	/**
	 * Function API call to read the data from the module into the provided
	 * buffer.
	 */
	return read_module_from_rfs(sd, module_uid, module_read_offset, read_bytes,
								buffer);
}
