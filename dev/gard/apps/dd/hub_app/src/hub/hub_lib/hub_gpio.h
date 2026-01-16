/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_GPIO_H__
#define __HUB_GPIO_H__

#include "gard_info.h"

/**
 * Note:
 * 1. types.h included from gard_info.h already defines a bool.
 * 2. gpiod.h also includes stdbool.h leading to a define clash.
 * 3. We make sure that stdbool.h is not included by the following #define
 *
 * stdbool.h for CM5 Raspbian OS is at:
 * /usr/lib/gcc/aarch64-linux-gnu/12/include/stdbool.h
 *
 * Other libgpiod function headers and defines are obtained from a regular
 * include of gpiod.h
 */
#define _STDBOOL_H
#include <gpiod.h>

/* For sleep */
#include <unistd.h>

#define HUB_GPIO_MONITOR_STRING           "HUB_GPIO_EVENT_MONITOR"

#define HUB_GPIO_MON_THREAD_TIMEOUT_SECS  (1)  // seconds
#define HUB_GPIO_MON_THREAD_TIMEOUT_NSECS (0)  // nanoseconds

#define HUB_INVALID_GPIO_PIN              (-1)

/* Context of a HUB GPIO monitor */
struct hub_gpio_mon_ctx {
	volatile bool          terminate_flag;
	volatile bool          mon_thread_initialized;
	struct timespec        mon_timeout;
	struct gpiod_chip     *p_chip;
	uint32_t               num_chip_lines;
	struct gpiod_line_bulk mon_bulk;
	hub_thread_hdl_t       mon_thread_hdl;
	hub_thread_attr_t      mon_thread_attr;
	hub_mutex_t            mon_mutex;
	hub_cond_var_t         mon_cond_var;
};

/* Context of a HUB GPIO event */
struct hub_gpio_event_ctx {
	int                gpio_pin;
	struct gpiod_line *p_line;
	hub_thread_hdl_t   event_thread_hdl;
	hub_thread_attr_t  event_thread_attr;
	hub_mutex_t        event_mutex;
	hub_cond_var_t     event_cond_var;
	int                event_type;
	bool               in_use;
	hub_cb_handler_t   user_cb;
	void              *p_user_cb_ctx;
	bool               event_callback_setup;
};

/* Context of a HUB GPIO worker */
struct hub_gpio_worker_ctx {
	gard_handle_t              p_gard_handle;
	struct hub_gpio_mon_ctx   *p_hub_gpio_mon_ctx;
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx;
	void                      *buffer;
	uint32_t                   size;
};

/**
 * hub_gpio_monitor_thread_func is a GPIO monitoring thread function.
 *
 * This thread monitors the GPIO lines requested for monitoring
 * and signals the corresponding worker thread on event occurence
 *
 * @param: hub_mon_params is the hub handle passed in
 *
 * @return: NULL
 */
void *hub_gpio_monitor_thread_func(void *hub_mon_params);

/**
 * hub_setup_appdata_cb is a function will be called by HUB whenever GARD FW
 * signals that it has application data available to be shipped across to the
 * application running on the Host.
 *
 * Register a user-supplied callback function
 * with HUB, which fetches application-specific data from the GARD FW over a
 * pertinent communication interface, and fills a user-supplied buffer up to the
 * user-supplied size (in bytes).
 *
 * Notes:
 * 1. HUB will call the supplied callback function in a separate thread,
 * in an asynchronous manner. The user is expected to handle this nuance in
 * their code appropriately.
 * 2. The user is also expected to manage the allocation of the buffer supplied
 * to this registration API appropriately.
 * 3. HUB may make multiple calls to the callback function supplied, and the
 * user is expected to handle this scenario to prevent the callback function
 * from overwriting or otherwise corrupting the supplied buffer.
 * Subsequent releases will include the option to have one-shot calls, or
 * repeated calls, or facilities to disable the callback from user perspective.
 * 4. The size variable used by the callback may be equal to / less than the
 * size given whilst registering. It will reflect the amount of data that
 * was actually obtained from the GARD FW. The user application is supposed
 * to take note of this and adapt accordingly.
 * 5. Failures during the execution of the callback function are expected to be
 * handled by the user.
 *
 * @param: gard is a GARD handle returned from hub_get_gard_handle()
 * @param: cb_handler is a callback function to be called, the API for this
 * function is defined as a hub_cb_handler_t datatype
 * @param: p_cb_ctx is an opaque callback context (can be used by the user-app)
 * @param: p_buffer is a user-allocated buffer of the proper size
 * @param: size is the number of bytes to fetch from the GARD FW
 *
 * @return: HUB_SUCCESS on successful registration
 *			HUB_FAILURE_SETUP_APPDATA_CB on failure
 *
 * NOTE : This is the only callback function to be used for callbacks in C.
 */
enum hub_ret_code hub_setup_appdata_cb(gard_handle_t    gard,
									   hub_cb_handler_t cb_handler,
									   void            *p_cb_ctx,
									   void            *p_buffer,
									   uint32_t         size);


/**
 * hub_recv_app_data_from_gard is a function that receives application data
 * from the GARD FW.
 *
 * @param: p_gard_handle is the GARD handle
 * @param: p_buffer is the user buffer pointer to be filled with app data from
 * @param: addr is the address of the data to be received
 * @param: count is the size of the user buffer
 *
 * @return: data_size (> 0) on success
 * 			error code (<= 0) on failure (HUB_FAILURE_RECV_APP_DATA)
 */
int64_t hub_recv_app_data_from_gard(gard_handle_t p_gard_handle,
									  void         *p_buffer,
									  uint32_t      addr,
									  uint32_t      count);

/**
 * hub_get_appdata_on_event_handler monitors GPIO event and
 * gets application data from GARD on event occurence.
 *
 * It is a wrapper that gets the GPIO event context from hub_ctx
 * given the gard handle and line offset.
 *
 * @param: p_gard_handle is the GARD handle
 * @param: line_offset is the GPIO line offset
 * @param: buffer is the user buffer pointer to be filled with app data from
 * GARD
 * @param: size is the size of the user buffer
 *
 * @return: integer : data_size (> 0) on success
 * 					  error code (<= 0) on failure
 */
int64_t hub_get_appdata_on_event_handler(gard_handle_t p_gard_handle,
										 int           line_offset,
										 void         *buffer,
										 uint32_t      size);

/**
 * hub_setup_appdata_cb_for_pyhub is an alternate version of
 * hub_setup_appdata_cb which doesn't take user callback function and context.
 *
 * It sets up the GPIO event context for monitoring and
 * leaves the user callback function pointer as NULL.
 *
 * This doesn't launch the worker thread. It sets up the event context
 * for the user to launch the worker thread as per their requirement.
 *
 * @param: gard is the GARD handle
 *
 * @return: positive integer : GPIO pin on which monitoring happens (on success)
 *			HUB_INVALID_GPIO_PIN (-1) (on failure)
 *
 * NOTE : This function is only for PyHub (Python Library) internal usage and IS
 * NOT MEANT to be used for callback usage in C-based application(s).
 */
 int32_t hub_setup_appdata_cb_for_pyhub(gard_handle_t gard);

#endif /* __HUB_GPIO_H__ */
