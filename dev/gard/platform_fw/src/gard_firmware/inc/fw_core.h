/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef FW_CORE_H
#define FW_CORE_H

#include "gard_types.h"
#include "network_info.h"
#include "image_info.h"
#include "app_module.h"

/**
 * This file defines the interfaces intended for use by the App Module.
 *
 * These APIs provide a consistent and hardware-agnostic way for the App Module
 * to interact with the underlying GARD hardware. By abstracting
 * hardware-specific details, this interface layer ensures that the same
 * application logic can be reused across different hardware implementations
 * without requiring changes.
 *
 * This design promotes modularity, portability, and ease of maintenance across
 * various GARD hardware platforms.
 */

/**
 * The following structure defines the list of callbacks that are implemented by
 * the App Module. These callbacks are invoked by the FW Core at various stages
 * of the application flow.
 * Additionally, in XIP mode the FW loader will relocate the addresses in this
 * structure to the actual XIP location of these callbacks on AXI bus.
 *
 * The App Module should use the macro DEFINE_APP_MODULE_CALLBACKS() which will
 * create a variable of this type and populate it with the App Module's
 * callback functions.
 */
struct app_module_cbs_table {
	app_handle_t      (*app_preinit_cb)(void);
	app_handle_t      (*app_init_cb)(app_handle_t app_context);
	enum app_ret_code (*app_preprocess_cb)(app_handle_t app_context,
										   void        *image_data);
	enum app_ret_code (*app_ml_done_cb)(app_handle_t app_context,
										void        *ml_results);
	enum app_ret_code (*app_image_processing_done_cb)(app_handle_t app_context);
	enum app_ret_code (*app_rescale_done_cb)(app_handle_t app_context);
};

extern struct app_module_cbs_table app_module_callbacks;

/**
 * App Module should call this macros outside of any function in one of its
 * C source files to define the callbacks that it implements. This macro will
 * create a variable of type struct app_module_cbs_table in a specific section
 * of memory.
 *
 * NOTE: Please make sure that only one C source file in the App Module calls
 * this macro to avoid multiple definitions of the same variable, that will be
 * a link time error.
 */
#define DEFINE_APP_MODULE_CALLBACKS(preinit_fn, init_fn, preprocess_fn,        \
									ml_done_fn, img_proc_done_fn,              \
									rescale_done_fn)                           \
	struct app_module_cbs_table app_module_callbacks                           \
		__attribute__((section(".fw_xip_reloc_table_section"))) = {            \
		.app_preinit_cb               = preinit_fn,                            \
		.app_init_cb                  = init_fn,                               \
		.app_preprocess_cb            = preprocess_fn,                         \
		.app_ml_done_cb               = ml_done_fn,                            \
		.app_image_processing_done_cb = img_proc_done_fn,                      \
		.app_rescale_done_cb          = rescale_done_fn}

/**
 * The following macro is used by the App Module to define the relocation table
 * that is used in XIP mode. This macro should be called globally in any one of
 * the App Module's C source files. This macro will create a variable that is
 * accessed by the FW core as well as
 */

/**
 * set_uart_parameters() is called by the App Module to configure UART
 * parameters such as baud rate, parity, and others.
 *
 * This function should only be called from within the app_preinit()
 * function of the App Module.
 */
bool set_uart_parameters(void    *uart_handle,
						 uint32_t baud_rate,
						 uint8_t  parity,
						 uint8_t  stop_bits);

/**
 * set_i2c_target_parameters() is called by the App Module to configure
 * I2C parameters such as clock speed, target address, and others.
 *
 * This function should only be called from within the app_preinit()
 * function of the App Module.
 */
bool set_i2c_target_parameters(void    *i2c_handle,
							   uint32_t clock_speed,
							   uint8_t  target_address);

/**
 * register_networks() is used by the App Module to register its ML networks
 * with the FW Core. This function should be called exclusively from within
 * the app_init() routine of the App Module.
 * Once this function is called the App Module should assume the networks
 * variable is owned by the FW Core. Hence the App Module should not
 * modify the contents of the networks variable after this call.
 */
bool register_networks(struct networks *list_of_networks);

/**
 * schedule_network_to_run() is invoked by routines in the App Module to change
 * the ML network that will be executed next on the ML engine. If this routine
 * is not called by the App Module at initialization time, then the first
 * network listed in the registered networks list will be scheduled.
 *
 * The parameter `network` specifies the UID of the already registered network
 * that should to be run next on the ML engine.
 */
bool schedule_network_to_run(ml_network_handle_t network);

/**
 * get_uid_of_next_network_to_run() is called by the App Module routines to
 * find the UID of the ML network that will be run next on the ML engine.
 *
 *
 * If no networks have been registered, the function returns
 * INVALID_NETWORK_HANDLE.
 */
ml_network_handle_t get_uid_of_next_network_to_run(void);

/**
 * get_uid_of_currently_running_network() is called by the App Module routines
 * to retrieve the UID of the network currently running on the ML engine.
 *
 * If no netwworks are running on the ML engine, this function returns
 * INVALID_NETWORK_HANDLE.
 */
int32_t get_uid_of_currently_running_network(void);

/**
 * register_buffer_for_host_data() is used by the App Module to register a
 * buffer that will be used to receive data from the App Module's counterpart
 * running on the Host.
 *
 * The rx_handler_t is a callback function that will be invoked by FW Core when
 * data is received from the Host. The App Module should implement this handler
 * to process the incoming data.
 */
void *register_buffer_for_host_data(uint8_t     *buffer,
									uint32_t     buffer_size,
									rx_handler_t app_rx_handler);

/**
 * stream_data_to_host_async() is used by the App Module to send streaming data,
 * recurring data such as information generated by the pipeline, to the Host.
 *
 * The data is sent asynchronously, meaning the data transfer could complete
 * to the Host in the background and the App Module should populate this buffer
 * only when the *p_send_complete parameter is set to true. App Module could use
 * two buffers to accumulate new data in the mean time.
 */
void stream_data_to_host_async(uint8_t *data,
							   uint32_t count_of_data_bytes,
							   uint32_t timeout_ms,
							   uint8_t *p_send_complete);

/**
 * send_event_to_host() is used by the App Module to send asynchronous data to
 * the Host. This is used by the App Module to send 'unexpected' events to the
 * Host, such as errors or other notifications that are not part of the
 * regular command-response flow.
 */
bool send_event_to_host(uint8_t *event_data,
						uint32_t count_of_data_bytes,
						uint32_t timeout_ms);

/**
 * capture_image_async() is used by the App Module to start capturing the image
 * from the connected camera. This function sets the wheels in motion to capture
 * the image. Once the image has been captured, the FW Core will call the
 * app_preprocess() function to allow the App Module to perform any
 * preprocessing on the image data before it is sent to the ML engine for
 * processing.
 */
void capture_image_async(void);

/**
 * start_ml_engine() is used by the App Module to start the ML engine. This
 * function should be called after the App Module has registered its networks
 * and is ready to start processing images.
 *
 * The ML engine will start processing the image data trigger an interrupt when
 * the processing has completed.
 */
void start_ml_engine(void);

/**
 * crop_and_rescale_image() is used by the App Module to crop and rescale the
 * image data before it is sent to the ML engine for processing.
 *
 * The in_image parameter contains the original image data, while the
 * scaled_image parameter will contain the cropped and rescaled image data.
 * This routine starts the hardware operation to rescale the image which may
 * take time
 */
bool crop_and_rescale_image(struct image_info *in_image,
							struct image_info *scaled_image);

/**
 * schedule_image_processing_done_event() is used by the App Module to schedule
 * a callback from FW Core once any other waiting events have been serviced.
 * FW Core will call the app_image_processing_done() function as a result which
 * should be implemented by the App Module to perform any final processing
 * after the image processing has completed.
 */
void schedule_image_processing_done_event(void);

/**
 * read_module_data() is used by the App Module to read the data from
 * the module into the provided buffer starting from the specified offset and
 * reading the specified number of bytes.
 */
uint32_t read_module_data(uint32_t module_uid,
						  uint32_t module_read_offset,
						  uint32_t read_bytes,
						  uint8_t *buffer);

#endif /* FW_CORE_H */
