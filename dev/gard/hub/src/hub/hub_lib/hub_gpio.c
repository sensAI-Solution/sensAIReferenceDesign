/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_gpio.h"
#include "hub_threading.h"

/* Static functions listing */
static inline void gpiod_line_bulk_remove(struct gpiod_line_bulk *bulk,
										  int                     num_line);
static void       *hub_gpio_worker_thread_func(void *hub_worker_params);
static int64_t
	hub_get_appdata_on_event(gard_handle_t              p_gard_handle,
							 struct hub_gpio_event_ctx *p_hub_gpio_event_ctx,
							 void                      *buffer,
							 uint32_t                   size);

/**
 * gpiod_line_bulk_remove is a helper function to remove a
 * line from a gpiod_line_bulk.
 * This is a utility to aid in error handling during setup.
 *
 * @param bulk The bulk object to modify.
 * @param num_line The offset of the line to remove.
 */
static inline void gpiod_line_bulk_remove(struct gpiod_line_bulk *bulk,
										  int                     num_line)
{
	int i, found_idx = -1;

	/* Find the index of the line to remove */
	for (i = 0; i < bulk->num_lines; i++) {
		if (gpiod_line_offset(bulk->lines[i]) == num_line) {
			found_idx = i;
			break;
		}
	}

	/* If the line was found, shift subsequent lines to fill the gap */
	if (found_idx != -1) {
		for (i = found_idx; i < bulk->num_lines - 1; i++) {
			bulk->lines[i] = bulk->lines[i + 1];
		}
		bulk->num_lines--;
		/**
		 * TBD-SSP:
		 * Null out the last element to avoid dangling pointers
		 * if not managed carefully elsewhere
		 * bulk->lines[bulk->num_lines] = NULL;
		 */
	}
}

/**
 * hub_gpio_worker_thread_func is a GPIO worker thread function.
 *
 * This thread waits for the monitor thread to signal it on event occurence
 * and calls the user callback function with data from GARD
 *
 * @param: hub_worker_params is the hub_gpio_worker_ctx passed in
 *
 * @return: A void pointer; currently NULL, and not used; may be used in future
 * releases for returning error codes, exit codes, etc. from the worker thread
 * function.
 */
static void *hub_gpio_worker_thread_func(void *hub_worker_params)
{
	struct hub_gpio_worker_ctx *p_hub_gpio_worker_ctx = NULL;
	struct hub_gpio_mon_ctx    *p_hub_gpio_mon_ctx    = NULL;
	struct hub_gpio_event_ctx  *p_hub_gpio_event_ctx  = NULL;
	int64_t                     ret;
	enum hub_ret_code           user_cb_ret;

	p_hub_gpio_worker_ctx = (struct hub_gpio_worker_ctx *)hub_worker_params;

	p_hub_gpio_mon_ctx    = p_hub_gpio_worker_ctx->p_hub_gpio_mon_ctx;
	p_hub_gpio_event_ctx  = p_hub_gpio_worker_ctx->p_hub_gpio_event_ctx;

	hub_pr_dbg("Starting worker thread for GPIO %d.\n",
			   p_hub_gpio_event_ctx->gpio_pin);

	while (1) {
		ret = hub_get_appdata_on_event(
			p_hub_gpio_worker_ctx->p_gard_handle, p_hub_gpio_event_ctx,
			p_hub_gpio_worker_ctx->buffer, p_hub_gpio_worker_ctx->size);

		if (p_hub_gpio_mon_ctx->terminate_flag) {
			break;
		}

		if (ret <= 0) {
			hub_pr_err("Failed to monitor and receive streaming app data from "
					   "GARD: %ld\n",
					   ret);
		}

		/* Handle occured event now */
		if (p_hub_gpio_event_ctx->event_callback_setup) {
			if (ret <= 0) {
				/**
				 * NOTE : Call user callback with 0 size to indicate error
				 */
				user_cb_ret = (enum hub_ret_code)p_hub_gpio_event_ctx->user_cb(
					p_hub_gpio_event_ctx->p_user_cb_ctx,
					p_hub_gpio_worker_ctx->buffer, 0);
				(void)user_cb_ret;
			} else {
				if (p_hub_gpio_event_ctx->user_cb) {
					/* Call the user callback function with actual data size */
					user_cb_ret =
						(enum hub_ret_code)p_hub_gpio_event_ctx->user_cb(
							p_hub_gpio_event_ctx->p_user_cb_ctx,
							p_hub_gpio_worker_ctx->buffer,
							(uint32_t)ret);
					(void)user_cb_ret;
				}
			}
		} else {
			/* callback function is not setup, ignore the event */
			hub_pr_dbg("User callback not setup. Ignoring events on GPIO %d.\n",
					   p_hub_gpio_event_ctx->gpio_pin);
			p_hub_gpio_event_ctx->event_callback_setup = false;
		}
	}

	hub_pr_dbg("Shutting down worker thread of GPIO %d.\n",
			   p_hub_gpio_event_ctx->gpio_pin);

	return NULL;
}

/**
 * hub_get_appdata_on_event monitors GPIO event and
 * gets application data from GARD on event occurence.
 *
 * @param: p_gard_handle is a GARD handle returned from hub_get_gard_handle()
 * @param: p_hub_gpio_event_ctx is the GPIO event context pointer
 * @param: buffer is a user-allocated buffer of the proper size
 * @param: size is the number of bytes to fetch from the GARD FW
 *
 * @return: HUB_SUCCESS on success
 * 			HUB_FAILURE_* on failure
 */
static int64_t
	hub_get_appdata_on_event(gard_handle_t              p_gard_handle,
							 struct hub_gpio_event_ctx *p_hub_gpio_event_ctx,
							 void                      *buffer,
							 uint32_t                   size)
{
	enum hub_ret_code        ret;
	struct hub_ctx          *p_hub              = NULL;
	struct hub_gard_info    *p_gard             = NULL;
	struct hub_gpio_mon_ctx *p_hub_gpio_mon_ctx = NULL;

	p_gard             = (struct hub_gard_info *)p_gard_handle;
	p_hub              = (struct hub_ctx *)p_gard->hub;

	p_hub_gpio_mon_ctx = p_hub->p_gpio_mon_ctx;

	hub_pr_dbg("Waiting for event in worker thread on GPIO %d.\n",
			   p_hub_gpio_event_ctx->gpio_pin);

	/* TBD-SSP: handle locking failures */
	hub_mutex_lock(&p_hub_gpio_event_ctx->event_mutex);
	ret = hub_cond_var_wait(&p_hub_gpio_event_ctx->event_cond_var,
							&p_hub_gpio_event_ctx->event_mutex);
	hub_mutex_unlock(&p_hub_gpio_event_ctx->event_mutex);

	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to wait on event_cond_var: %d\n", ret);
		/* returning as couldn't wait for monitoring */
		return (int64_t)ret;
	}

	if (p_hub_gpio_mon_ctx->terminate_flag) {
		return (int64_t)ret;
	}

	hub_pr_dbg("Event - GPIO : %d, Type : %s edge\n",
			   p_hub_gpio_event_ctx->gpio_pin,
			   p_hub_gpio_event_ctx->event_type == GPIOD_LINE_EVENT_RISING_EDGE
				   ? "rising"
				   : "falling");

	hub_pr_dbg("Gathering data from GARD.\n");

	/**
	 * Get streaming data of size specified by user
	 * from GARD into user's buffer
	 *
	 * hub_recv_app_data_from_gard() returns the actual data size received.
	 * Returns data_size (> 0) on success, or error code (< 0) on failure.
	 */
	int64_t received_size = hub_recv_app_data_from_gard(
		p_gard_handle, buffer,
		0xDEADBEEF, /* give random address, GARD doesn't care */
		size);

	/* Return the data size directly (positive on success, negative on failure) */
	return received_size;
}

/******************************************************************************
 * Publicly exposed functions
 ******************************************************************************/
/**
 * hub_gpio_monitor_thread_func is a GPIO monitoring thread function.
 *
 * This thread monitors the GPIO lines requested for monitoring
 * and signals the corresponding worker thread on event occurence
 *
 * @param: hub_mon_params is the hub handle passed in
 *
 * @return: A void pointer; currently NULL, and not used; may be used in future
 * releases for returning error codes, exit codes, etc. from the worker thread
 * function.
 */
void *hub_gpio_monitor_thread_func(void *hub_mon_params)
{
	int                        ret, i, line_offset;
	unsigned int               num_events;
	struct gpiod_line_event    event;
	struct gpiod_line_bulk     event_bulk;
	struct gpiod_line         *line                 = NULL;

	struct hub_ctx            *p_hub                = NULL;
	struct hub_gpio_mon_ctx   *p_hub_gpio_mon_ctx   = NULL;
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx = NULL;

	p_hub              = (struct hub_ctx *)hub_mon_params;

	p_hub_gpio_mon_ctx = p_hub->p_gpio_mon_ctx;

	hub_pr_dbg("Launched monitoring thread.\n");

	while (1) {
		if (p_hub_gpio_mon_ctx->terminate_flag) {
			break;
		}

		if (!p_hub_gpio_mon_ctx->mon_bulk.num_lines) {
			/* waiting for app to set up monitoring on some GPIO lines */
			/* TBD-SSP: handle locking failures */
			hub_mutex_lock(&p_hub_gpio_mon_ctx->mon_mutex);
			hub_pr_info("Waiting for monitoring getting enabled via callback "
						"setup function.\n");

			ret = hub_cond_var_wait(&p_hub_gpio_mon_ctx->mon_cond_var,
									&p_hub_gpio_mon_ctx->mon_mutex);

			hub_mutex_unlock(&p_hub_gpio_mon_ctx->mon_mutex);
			if (HUB_SUCCESS != ret) {
				hub_pr_err("Failed to wait on mon_cond_var: %d\n", ret);
				/* Breaking out since we cannot wait on mon_cond_var */
				break;
			}

			p_hub_gpio_mon_ctx->mon_thread_initialized = true;

			if (p_hub_gpio_mon_ctx->terminate_flag) {
				break;
			}

			continue;
		}
		/* no error checks required */
		gpiod_line_bulk_init(&event_bulk);

		/**
		 * Release previously requested events. This is required as
		 * gpiod_line_request_bulk() fails if any lines used in mon_bulk
		 * are not free.
		 *
		 * TBD-SSP: Need to handle setup appdata where mon bulk is changed in
		 * real time. So request the changes as required for falling / rising
		 * edge. Compare old and new bulks, release bulk and request again.
		 */
		gpiod_line_release_bulk(&p_hub_gpio_mon_ctx->mon_bulk);

		/**
		 * Monitors events of rising and falling edge on mon_bulk
		 * TBD-SSP:
		 * Currently monitor thread is raising for rising edge only. In
		 * future, it's worker thread's responsibility to filter out and act
		 * on the required event type.
		 */
		ret = gpiod_line_request_bulk_both_edges_events(
			&p_hub_gpio_mon_ctx->mon_bulk, HUB_GPIO_MONITOR_STRING);
		if (ret < 0) {
			hub_pr_err("Failed to request events on monitoring lines : %s\n",
					   strerror(errno));
			break;
		}

		/**
		 * Monitoring wait for event to occur on any of the lines in mon_bulk
		 * with a timeout of 5 seconds.
		 * Return Codes :
		 * 0 - timeout
		 * >0 - number of lines with events
		 * <0 - error
		 * ERRNO is updated by gpiod_line_event_wait_bulk()
		 */
		ret = gpiod_line_event_wait_bulk(&p_hub_gpio_mon_ctx->mon_bulk,
										 &p_hub_gpio_mon_ctx->mon_timeout,
										 &event_bulk);
		if (!ret) {
			/* Timeout occurred - not an error */
			continue;
		} else if (ret < 0) {
			hub_pr_err(
				"Failed to wait for events on monitoring lines : %s (%d)\n",
				strerror(errno), ret);
			/**
			 * Genuine errors which are not handled hence
			 * breaking out of monitoring loop
			 * TBD-SSP: decide if need to continue on other errors?
			 */
			break;
		}

		/* TBD-SSP: Handle terminate_flag checks more elegantly in the monitor
		 * thread */
		if (p_hub_gpio_mon_ctx->terminate_flag) {
			break;
		}

		/* We got an event - parsing the event_bulk */
		num_events = gpiod_line_bulk_num_lines(&event_bulk);
		if (num_events) {
			hub_pr_dbg("Received %d GPIO event(s).\n", num_events);
			for (i = 0; i < num_events; i++) {
				line = gpiod_line_bulk_get_line(&event_bulk, i);
				if (NULL == line) {
					hub_pr_err("Failed to get line from event bulk [%d]\n", i);
					continue;
				}

				line_offset = gpiod_line_offset(line);
				hub_pr_dbg("Processing event occured on line offset %d\n",
						   line_offset);

				ret = gpiod_line_event_read(line, &event);
				if (ret) {
					hub_pr_err("Failed to read event for line %d: %s\n",
							   line_offset, strerror(errno));
					continue;
				}

				/**
				 * Signal event conditional variable of corresponding
				 *  GPIO event line
				 */
				if ((line_offset >= 0) &&
					(line_offset < p_hub_gpio_mon_ctx->num_chip_lines)) {
					p_hub_gpio_event_ctx =
						&p_hub->p_gpio_event_ctx[line_offset];
					if (p_hub_gpio_event_ctx->in_use) {
						hub_pr_dbg("Invoking worker thread of GPIO %d, event "
								   "type : %d.\n",
								   line_offset, event.event_type);
						p_hub_gpio_event_ctx->event_type = event.event_type;

						/* Signal only for rising edges per usecase */
						if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
							/* TBD-SSP: handle locking failures */
							hub_mutex_lock(&p_hub_gpio_event_ctx->event_mutex);
							ret = hub_cond_var_signal(
								&p_hub_gpio_event_ctx->event_cond_var);
							if (HUB_SUCCESS != ret) {
								hub_pr_err(
									"Failed to signal event_cond_var: %d\n",
									ret);
								/* TBD-SSP: decide what to do here */
							}
							hub_mutex_unlock(
								&p_hub_gpio_event_ctx->event_mutex);
						}
					} else {
						hub_pr_dbg("Received event for line offset %d which is "
								   "not in use\n",
								   line_offset);
					}
				} else {
					/* should never reach here, if reached it's corruption */
					hub_pr_err("Received event for unused line offset %d\n",
							   line_offset);
				}
			}

			/**
			 * Required to clear the events for populating anew.
			 * not putting outside of num_events check, as
			 * if num_events is 0, event_bulk is empty in itself
			 */
			gpiod_line_release_bulk(&event_bulk);
		}
	}

	hub_pr_dbg("Shutting down monitoring thread.\n");

	return NULL;
}

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
										 uint32_t      size)
{
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx = NULL;
	struct hub_ctx            *p_hub                = NULL;
	struct hub_gard_info      *p_gard               = NULL;

	p_gard               = (struct hub_gard_info *)p_gard_handle;
	p_hub                = (struct hub_ctx *)p_gard->hub;

	p_hub_gpio_event_ctx = &p_hub->p_gpio_event_ctx[line_offset];

	int64_t data_size = hub_get_appdata_on_event(p_gard_handle, p_hub_gpio_event_ctx,
												  buffer, size);

	/* Return data size on success, error code on failure */
	return data_size;
}

/**
 * hub_setup_appdata_cb registers a user callback function for application data
 * retrieval from GARD on GPIO event
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
 */
enum hub_ret_code hub_setup_appdata_cb(gard_handle_t    gard,
									   hub_cb_handler_t cb_handler,
									   void            *p_cb_ctx,
									   void            *p_buffer,
									   uint32_t         size)
{
	enum hub_ret_code           ret;
	int                         i, num_gpio_inputs, pin_to_check;
	int                         current_gpio_pin      = HUB_INVALID_GPIO_PIN;

	struct hub_gard_info       *p_gard                = NULL;
	struct hub_ctx             *p_hub                 = NULL;
	struct hub_gpio_mon_ctx    *p_hub_gpio_mon_ctx    = NULL;
	struct hub_gpio_event_ctx  *p_hub_gpio_event_ctx  = NULL;
	struct hub_gpio_worker_ctx *p_hub_gpio_worker_ctx = NULL;

	p_gard             = (struct hub_gard_info *)gard;
	p_hub              = (struct hub_ctx *)p_gard->hub;

	p_hub_gpio_mon_ctx = p_hub->p_gpio_mon_ctx;

	/* Sanity checks */
	if (NULL == gard) {
		hub_pr_err("Invalid GARD handle.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	if (NULL == cb_handler) {
		hub_pr_err("Invalid callback handle.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	if (NULL == p_cb_ctx) {
		hub_pr_warn("Callback context is not given.\n");
		/* not returning error, as cb_ctx can be NULL */
	}

	if (NULL == p_buffer) {
		hub_pr_err("Invalid user buffer handle.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	if (!size) {
		hub_pr_err("Invalid user buffer size.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	if (!p_hub->p_gpio_mon_ctx->p_chip) {
		hub_pr_err("Uninitialized GPIO chip.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	/* TBD-SSP: Check this condition */
	num_gpio_inputs = p_hub->p_gards->num_gpio_inputs;
	if (!num_gpio_inputs) {
		hub_pr_err("GARD configuration doesn't have GPIO inputs.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	/**
	 * Iterates to find the first available pin for GPIO monitoring.
	 * p_hub->p_gards->gpio_inputs has the GPIO pins configured for
	 * given GARD. Essentially, these pins are available in gard.json file.
	 *
	 * This for loop checks for the first available pin.
	 * This might happen when multiple GPIO pins are being used.
	 *
	 * TBD-SSP: Need to find a method to map GPIO pin to a specific app_data_cb
	 * in case of more than one GPIO pins used for monitoring.
	 */
	for (i = 0; i < num_gpio_inputs; i++) {
		pin_to_check = p_hub->p_gards->gpio_inputs[i];

		if ((pin_to_check >= 0) &&
			(pin_to_check < p_hub_gpio_mon_ctx->num_chip_lines)) {
			/* Choosing gpio event array entry for the chosen pin */
			p_hub_gpio_event_ctx = &p_hub->p_gpio_event_ctx[pin_to_check];
			if (!p_hub_gpio_event_ctx->in_use) {
				/* first not IN USE pin found */
				current_gpio_pin = pin_to_check;
				hub_pr_dbg("Chosen GPIO pin %d for appdata callback.\n",
						   current_gpio_pin);
				break;
			}
		} else {
			hub_pr_err("Desired GPIO pin %d is out of range of GPIO chipset.\n",
					   pin_to_check);
			/* continue to check for other pins */
		}
	}

	if (HUB_INVALID_GPIO_PIN == current_gpio_pin) {
		hub_pr_err("No pins are available for monitoring.\n");
		goto hub_setup_appdata_cb_err_1;
	}

	/* Acquire the GPIO line for the chosen pin */
	p_hub_gpio_event_ctx->p_line =
		gpiod_chip_get_line(p_hub_gpio_mon_ctx->p_chip, current_gpio_pin);
	if (!p_hub_gpio_event_ctx->p_line) {
		hub_pr_err("Failed to acquire GPIO line for pin %d : %s\n",
				   current_gpio_pin, strerror(errno));
		goto hub_setup_appdata_cb_err_1;
	}

	/* Set up all the variables of the current GPIO event ctx */
	p_hub_gpio_event_ctx->gpio_pin      = current_gpio_pin;
	p_hub_gpio_event_ctx->event_type    = 0;
	p_hub_gpio_event_ctx->in_use        = true;
	p_hub_gpio_event_ctx->p_user_cb_ctx = p_cb_ctx;
	/* Always setup a function pointer after filling it's variables
	 * as it can be called anytime */
	p_hub_gpio_event_ctx->user_cb       = cb_handler;

	if (cb_handler) {
		/* Set this flag to true, when user has given callback func.
		 * Worker thread will check the flag and only act on func */
		p_hub_gpio_event_ctx->event_callback_setup = true;
	}

	p_hub_gpio_worker_ctx = (struct hub_gpio_worker_ctx *)calloc(
		1, sizeof(struct hub_gpio_worker_ctx));
	if (NULL == p_hub_gpio_worker_ctx) {
		hub_pr_err(
			"Failed to allocate memory for hub_gpio_worker_ctx for pin %d.\n",
			p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_err_2;
	}

	/* Assigning worker ctx variables */
	p_hub_gpio_worker_ctx->p_gard_handle        = gard;
	p_hub_gpio_worker_ctx->p_hub_gpio_mon_ctx   = p_hub->p_gpio_mon_ctx;
	p_hub_gpio_worker_ctx->p_hub_gpio_event_ctx = p_hub_gpio_event_ctx;
	p_hub_gpio_worker_ctx->buffer               = p_buffer;
	p_hub_gpio_worker_ctx->size                 = size;

	ret = hub_mutex_init(&p_hub_gpio_event_ctx->event_mutex);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize event_mutex for pin %d.\n",
				   p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_err_3;
	}

	ret = hub_cond_var_init(&p_hub_gpio_event_ctx->event_cond_var);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize event_cond_var for pin %d.\n",
				   p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_err_4;
	}

	/* Updating the mon_bulk with new monitoring line */
	gpiod_line_bulk_add(&p_hub_gpio_mon_ctx->mon_bulk,
						p_hub_gpio_event_ctx->p_line);

	/* Launch the worker thread for this GPIO pin */
	ret = hub_thread_create(&p_hub_gpio_event_ctx->event_thread_hdl,
							&p_hub_gpio_event_ctx->event_thread_attr,
							hub_gpio_worker_thread_func,
							(void *)p_hub_gpio_worker_ctx);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to create thread for GPIO line of pin %d.\n",
				   p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_err_5;
	}
	hub_pr_dbg("Creating worker thread for GPIO pin %d. Handle - %ld\n",
			   p_hub_gpio_event_ctx->gpio_pin,
			   p_hub_gpio_event_ctx->event_thread_hdl);

	/* Signal the monitor thread to start monitoring on added lines */
	/* TBD-SSP: handle locking failures */
	hub_mutex_lock(&p_hub_gpio_mon_ctx->mon_mutex);
	ret = hub_cond_var_signal(&p_hub_gpio_mon_ctx->mon_cond_var);
	hub_mutex_unlock(&p_hub_gpio_mon_ctx->mon_mutex);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to signal mon_cond_var: %d\n", ret);
		goto hub_setup_appdata_cb_err_6;
	}

	hub_pr_dbg("Launched worker thread for GPIO %d.\n", current_gpio_pin);

	return HUB_SUCCESS;

hub_setup_appdata_cb_err_6:
	hub_thread_cancel(p_hub_gpio_event_ctx->event_thread_hdl);
	hub_thread_join(p_hub_gpio_event_ctx->event_thread_hdl, NULL);
	p_hub_gpio_event_ctx->event_thread_hdl = 0;
hub_setup_appdata_cb_err_5:
	hub_cond_var_destroy(&p_hub_gpio_event_ctx->event_cond_var);
	gpiod_line_bulk_remove(&p_hub_gpio_mon_ctx->mon_bulk, current_gpio_pin);
	/* TBD-SSP: can we just release just this line */
	p_hub_gpio_event_ctx->p_line = NULL;
hub_setup_appdata_cb_err_4:
	hub_mutex_destroy(&p_hub_gpio_event_ctx->event_mutex);
hub_setup_appdata_cb_err_3:
	free(p_hub_gpio_worker_ctx);
hub_setup_appdata_cb_err_2:
	p_hub_gpio_event_ctx->in_use        = false;
	p_hub_gpio_event_ctx->user_cb       = NULL;
	p_hub_gpio_event_ctx->p_user_cb_ctx = NULL;
hub_setup_appdata_cb_err_1:
	return HUB_FAILURE_SETUP_APPDATA_CB;
}

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
int32_t hub_setup_appdata_cb_for_pyhub(gard_handle_t gard)
{
	enum hub_ret_code          ret;
	int                        i, num_gpio_inputs, pin_to_check;
	int                        current_gpio_pin     = HUB_INVALID_GPIO_PIN;

	struct hub_gard_info      *p_gard               = NULL;
	struct hub_ctx            *p_hub                = NULL;
	struct hub_gpio_mon_ctx   *p_hub_gpio_mon_ctx   = NULL;
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx = NULL;

	p_gard             = (struct hub_gard_info *)gard;
	p_hub              = (struct hub_ctx *)p_gard->hub;

	p_hub_gpio_mon_ctx = p_hub->p_gpio_mon_ctx;

	/* Sanity checks */
	if (NULL == gard) {
		hub_pr_err("Invalid GARD handle.\n");
		goto hub_setup_appdata_cb_py_err_1;
	}

	if (!p_hub->p_gpio_mon_ctx->p_chip) {
		hub_pr_err("Uninitialized GPIO chip.\n");
		goto hub_setup_appdata_cb_py_err_1;
	}

	/* TBD-SSP: Check this condition */
	num_gpio_inputs = p_hub->p_gards->num_gpio_inputs;
	if (!num_gpio_inputs) {
		hub_pr_err("GARD configuration doesn't have GPIO inputs.\n");
		goto hub_setup_appdata_cb_py_err_1;
	}

	/**
	 * Iterates to find the first available pin for GPIO monitoring.
	 * p_hub->p_gards->gpio_inputs has the GPIO pins configured for
	 * given GARD. Essentially, these pins are available in gard.json file.
	 *
	 * This for loop checks for the first available pin.
	 * This might happen when multiple GPIO pins are being used.
	 *
	 * TBD-SSP: Need to find a method to map GPIO pin to a specific app_data_cb
	 * in case of more than one GPIO pins used for monitoring.
	 */
	for (i = 0; i < num_gpio_inputs; i++) {
		pin_to_check = p_hub->p_gards->gpio_inputs[i];

		if ((pin_to_check >= 0) &&
			(pin_to_check < p_hub_gpio_mon_ctx->num_chip_lines)) {
			/* Choosing gpio event array entry for the chosen pin */
			p_hub_gpio_event_ctx = &p_hub->p_gpio_event_ctx[pin_to_check];
			if (!p_hub_gpio_event_ctx->in_use) {
				/* first not IN USE pin found */
				current_gpio_pin = pin_to_check;
				hub_pr_dbg("Chosen GPIO pin %d for appdata callback.\n",
						   current_gpio_pin);
				break;
			}
		} else {
			hub_pr_err("Desired GPIO pin %d is out of range of GPIO chipset.\n",
					   pin_to_check);
			/* continue to check for other pins */
		}
	}

	if (HUB_INVALID_GPIO_PIN == current_gpio_pin) {
		hub_pr_err("No pins are available for monitoring.\n");
		goto hub_setup_appdata_cb_py_err_1;
	}

	/* Acquire the GPIO line for the chosen pin */
	p_hub_gpio_event_ctx->p_line =
		gpiod_chip_get_line(p_hub_gpio_mon_ctx->p_chip, current_gpio_pin);
	if (!p_hub_gpio_event_ctx->p_line) {
		hub_pr_err("Failed to acquire GPIO line for pin %d : %s\n",
				   current_gpio_pin, strerror(errno));
		goto hub_setup_appdata_cb_py_err_1;
	}

	/* Set up all the variables of the current GPIO event ctx */
	p_hub_gpio_event_ctx->gpio_pin      = current_gpio_pin;
	p_hub_gpio_event_ctx->event_type    = 0;
	p_hub_gpio_event_ctx->in_use        = true;
	p_hub_gpio_event_ctx->p_user_cb_ctx = NULL;

	/** Always setup a function pointer after filling it's variables
	 * as it can be called anytime
	 * setting NULL as default cb func exists
	 */
	p_hub_gpio_event_ctx->user_cb       = NULL;

	if (p_hub_gpio_event_ctx->user_cb) {
		/**
		 * Set this flag to true, when user has given callback func.
		 * Worker thread will check the flag and only act on func
		 */
		p_hub_gpio_event_ctx->event_callback_setup = true;
	}

	ret = hub_mutex_init(&p_hub_gpio_event_ctx->event_mutex);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize event_mutex for pin %d.\n",
				   p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_py_err_2;
	}

	ret = hub_cond_var_init(&p_hub_gpio_event_ctx->event_cond_var);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize event_cond_var for pin %d.\n",
				   p_hub_gpio_event_ctx->gpio_pin);
		goto hub_setup_appdata_cb_py_err_2;
	}

	/* Updating the mon_bulk with new monitoring line */
	gpiod_line_bulk_add(&p_hub_gpio_mon_ctx->mon_bulk,
						p_hub_gpio_event_ctx->p_line);

	/* Signal the monitor thread to start monitoring on added lines */
	/* TBD-SSP: handle locking failures */
	hub_mutex_lock(&p_hub_gpio_mon_ctx->mon_mutex);
	ret = hub_cond_var_signal(&p_hub_gpio_mon_ctx->mon_cond_var);
	hub_mutex_unlock(&p_hub_gpio_mon_ctx->mon_mutex);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to signal mon_cond_var: %d\n", ret);
		goto hub_setup_appdata_cb_py_err_3;
	}

	hub_pr_dbg("Launched worker thread for GPIO %d.\n", current_gpio_pin);

	return pin_to_check;

hub_setup_appdata_cb_py_err_3:
	p_hub_gpio_event_ctx->event_thread_hdl = 0;
	hub_cond_var_destroy(&p_hub_gpio_event_ctx->event_cond_var);
	gpiod_line_bulk_remove(&p_hub_gpio_mon_ctx->mon_bulk, current_gpio_pin);
	/* TBD-SSP: can we just release just this line */
	p_hub_gpio_event_ctx->p_line = NULL;
hub_setup_appdata_cb_py_err_2:
	hub_mutex_destroy(&p_hub_gpio_event_ctx->event_mutex);
	p_hub_gpio_event_ctx->in_use        = false;
	p_hub_gpio_event_ctx->user_cb       = NULL;
	p_hub_gpio_event_ctx->p_user_cb_ctx = NULL;
hub_setup_appdata_cb_py_err_1:
	return HUB_INVALID_GPIO_PIN;
}
