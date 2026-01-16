/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_H__
#define __HUB_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

enum hub_ret_code {
	HUB_SUCCESS        = 0,
	/* HUB FAILURE CODES FOLLOW */
	HUB_FAILURE_MARKER = -256,
	HUB_FAILURE_PREINIT,
	HUB_FAILURE_GARD_DISCOVER,
	HUB_FAILURE_INIT,
	HUB_FAILURE_FINI,
	HUB_FAILURE_GARD_PROBE,
	HUB_FAILURE_READ_REG,
	HUB_FAILURE_WRITE_REG,
	HUB_FAILURE_SEND_DATA,
	HUB_FAILURE_RECV_DATA,
	HUB_FAILURE_RECV_APP_DATA,
	HUB_FAILURE_READ_FILE_INTO_MEM,
	HUB_FAILURE_PARSE_JSON,
	HUB_FAILURE_SENSOR_ERROR,
	HUB_FAILURE_THREAD_MONITOR,
	HUB_FAILURE_THREAD_CREATE,
	HUB_FAILURE_THREAD_JOIN,
	HUB_FAILURE_THREAD_CANCEL,
	HUB_FAILURE_MUTEX_INIT,
	HUB_FAILURE_MUTEX_LOCK,
	HUB_FAILURE_MUTEX_UNLOCK,
	HUB_FAILURE_MUTEX_DESTROY,
	HUB_FAILURE_CONDVAR_INIT,
	HUB_FAILURE_CONDVAR_WAIT,
	HUB_FAILURE_CONDVAR_SIGNAL,
	HUB_FAILURE_CONDVAR_DESTROY,
	HUB_FAILURE_SETUP_APPDATA_CB,
	HUB_FAILURE_CAPTURE_RESCALED_IMAGE,
	HUB_FAILURE_SEND_RESUME_PIPELINE,
};

/**
 * Bus listing for HUB <-> GARD communications
 * This will be used to create:
 * 1. An enum for the bus type
 * 2. An array of strings where the string matches the enum name
 */
#define FOR_EACH_BUS(BUS)                                                      \
	BUS(HUB_GARD_BUS_UNKNOWN)                                                  \
	BUS(HUB_GARD_BUS_I2C)                                                      \
	BUS(HUB_GARD_BUS_UART)                                                     \
	BUS(HUB_GARD_BUS_USB)                                                      \
	BUS(HUB_GARD_BUS_MIPI_CSI2)                                                \
	BUS(HUB_GARD_BUS_PCIE)                                                     \
	BUS(HUB_GARD_NR_BUSSES)

#define GEN_BUS_ENUM(BUS)      BUS,
#define GEN_BUS_STRING(STRING) #STRING,

/* Enum for all HUB <-> GARD busses */
enum hub_gard_bus_types {
	FOR_EACH_BUS(GEN_BUS_ENUM)
};

/* Opaque handle to a HUB instance, returned from the hub_init() call */
typedef void *hub_handle_t;

/* Opaque handle for each GARD listed in the hub handle */
typedef void *gard_handle_t;

/* Prototype of a HUB callback handler function */
typedef void *(*hub_cb_handler_t)(void    *p_cb_params,
								  void    *p_buffer,
								  uint32_t size);

/**
 * The following enum captures all the possible image formats. The binary image
 * data captured from the camera and sent to HUB is crafted in one of these
 * formats.

 * The enum is a copy of the one in gard_hub_iface.h and needs to be kept in
 * sync.
 */
enum hub_image_formats {
	/* This is invalid and should not be used. */
	HUB_IMAGE_FORMAT__INVALID        = 0x0u,

	/**
	 * The RGB values in the image are in non-planar fashion. In other words
	 * each pixel is represented by 3 consecutive bytes representing R, G and B
	 * values.
	 */
	HUB_IMAGE_FORMAT__RGB_NON_PLANAR = 0x1u,

	/**
	 * The RGB values in the image are in planar fashion. In other words the
	 * first 1/3rd of the buffer is the R values, the next 1/3rd is the G values
	 * and the last 1/3rd is the B values.
	 */
	HUB_IMAGE_FORMAT__RGB_PLANAR     = 0x2u,

	/**
	 * The image is a grayscale image. The image is represented by a single byte
	 * for each pixel.
	 */
	HUB_IMAGE_FORMAT__GRAYSCALE      = 0x3u,
};

/**
 * Context for image operations
 */
struct hub_img_ops_ctx {
	uint8_t                camera_id;
	void                  *p_image_buffer;
	uint32_t               image_buffer_size;
	uint32_t               image_buffer_address;
	uint16_t               h_size;
	uint16_t               v_size;
	enum hub_image_formats image_format;
};

/******************************************************************************
 * HUB versioning, initialization, display and clean up APis
 ******************************************************************************/
/**
 * Get a string containing the current version of HUB
 */
const char *hub_get_version_string(void);

/**
 * Pre-initialize HUB given a host_config and a GARD json dir,
 * and get a configured hub handle
 *
 * If p_config is NULL, perform a system scan on all busses.
 * [not supported in this release]
 *
 * If p_config is non-NULL, assume it is a valid host config file
 * and parse accordingly.
 */
enum hub_ret_code
	hub_preinit(void *p_config, char *p_gard_json_dir, hub_handle_t *hub);

/**
 * Given a HUB handle run GARD discovery commands and discover GARDs,
 * fill up the HUB context internals accordingly.
 */
enum hub_ret_code hub_discover_gards(hub_handle_t hub);

/**
 * hub_init is a HUB initialization function.
 *
 * Given a HUB handle, this
 * 1. Starts a monitoring thread for GPIOs defined in the GARD json file.
 * 2. Allocates and initializes worker thread contexts.
 *
 * @param: hub is a HUB handle
 *
 * @return: hub_ret_code
		HUB_SUCCESS on success
		HUB_FAILURE_INIT on failure
 */
enum hub_ret_code hub_init(hub_handle_t hub);

/* Get the number of GARDS discovered, given a HUB handle */
uint32_t hub_get_num_gards(hub_handle_t hub);

/* Given a HUB handle, get the GARD handle corresponding to gard_num */
gard_handle_t hub_get_gard_handle(hub_handle_t hub, uint8_t gard_num);

/**
 * De-initialize the HUB library
 *
 * Frees up all memory allocated in the HUB library
 *
 * Closes all open busses found open, if any
 */
enum hub_ret_code hub_fini(hub_handle_t hub);

/******************************************************************************
 * HUB data movement APIs
 ******************************************************************************/
/**
 * Write a given 32-bit value to a register address in GARD memory
 * represented by the gard handle.
 */
enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
									 uint32_t       reg_addr,
									 const uint32_t value);

/**
 * Read the 32-bit value of a register address in GARD memory
 * represented by the gard handle.
 */
enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
									uint32_t      reg_addr,
									uint32_t     *p_value);

/**
 * Send a data buffer of a specified size from HUB to an
 * address in the GARD memory map represented by the gard handle.
 */
enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
										const void   *p_buffer,
										uint32_t      addr,
										uint32_t      count);

/**
 * Receive data of specified size from an address in the
 * GARD memory map represented	by the GARD handle, into a HUB
 * data buffer.
 * 
 * Note: If this function is called to receive image data from GARD,
 * say, after hub_capture_rescaled_image_from_gard() is called, then
 * HUB / Host Applicaiton should also call hub_send_resume_pipeline() after
 * receiving the image content to signal GARD FW to resume the paused AI
 * workload.
 */
enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
										  void         *p_buffer,
										  uint32_t      addr,
										  uint32_t      count);

/******************************************************************************
 * HUB sensor data collection APIs
 * Currently, only the LSCC_SOM on the Carrier board is supported
 ******************************************************************************/
/**
 * Read the temperature from the sensors with given sensor IDs.
 *
 * Assumption: The sensor is attached on the same I2C bus as the GARD
 */
uint32_t hub_get_temperature_from_onboard_sensors(hub_handle_t hub,
												  uint8_t     *p_sensor_ids,
												  float       *p_temp,
												  uint8_t      num_sensors);

/* Structure holding values that can be read from an energy sensor */
struct hub_energy_sensor_values {
	float voltage;
	float current;
	float power;
};

/**
 * Read the energy values (voltage, current and power) from the sensors
 * with given sensor IDs.
 *
 * Assumption: The sensor is attached on the same I2C bus as the GARD
 */
uint32_t hub_get_energy_from_onboard_sensors(
	hub_handle_t                     hub,
	uint8_t                         *p_sensor_ids,
	struct hub_energy_sensor_values *p_values,
	uint8_t                          num_sensors);

/******************************************************************************
 * App Metadata Streaming Feature related APIs
 ******************************************************************************/
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
 * hub_get_appdata_on_event_handler monitors GPIO event and
 * gets application data from GARD on event occurence.
 *
 * It is a wrapper that gets the GPIO event context from hub_ctx
 * given the gard handle and line offset.
 *
 * Note:
 * In case of success, data_size is returned as int64_t because
 * the data size received from GARD is uint32_t.
 * In case of failure, negative integer value is returned which corresponds to
 * the error codes defined in hub.h as hub_ret_code enum.
 * 	- HUB_FAILURE_RECV_APP_DATA - failure in receiving app data from GARD
 * 	- corresponding error codes - failure in other operations
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

/******************************************************************************
 * Image Operations related APIs
 ******************************************************************************/
/**
 * Capture a rescaled image from the connected GARD
 * and get the properties of the image.
 *
 * NOTE:
 * - This is an asynchronous call from HUB to GARD FW.
 * - On receiving this command, GARD FW pauses its pipelines, captures the
 * 	 rescaled image from the camera into HRAM, and returns the image properties
 * 	 to HUB via the struct hub_img_ops_ctx.
 * - HUB / Host Application should call hub_recv_data_from_gard() after this
 *   function to receive the image content from GARD FW, in the image_buffer
 *   field of the struct hub_img_ops_ctx.
 * - HUB / Host Applicaiton should also call hub_send_resume_pipeline() after
 *   receiving the image content to signal GARD FW to resume the paused AI
 *   workload.
 */
enum hub_ret_code
	hub_capture_rescaled_image_from_gard(gard_handle_t           p_gard_handle,
										 struct hub_img_ops_ctx *p_img_ops_ctx);

/******************************************************************************
 * HUB <-> GARD Control Command related APIs
 ******************************************************************************/
/**
 * Send the resume pipeline command to the GARD
 */
enum hub_ret_code hub_send_resume_pipeline(gard_handle_t p_gard_handle,
										   uint8_t       camera_id);

#endif /* __HUB_H__ */
