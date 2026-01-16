/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef APP_MODULE_H
#define APP_MODULE_H

#include "gard_types.h"
#include "memmap.h"

/**
 * The functions listed in this file are supposed to be implemented by the
 * Application Module. These prototypes ensure that the FW Core can call
 * these functions with the correct parameters.
 */

/**
 * Application shall define network parameters for applicable use case
 * Below parameters define Multi object detection network parameters
 */

/* Pre-input buffer sits in RISC-V region to avoid consuming ML IO space.
 * This buffer is used to hold the pre-processed / captured image before feeding
 * it to the ML engine.
 */
#define ML_APP_MOD_PREINPUT_START_ADDRESS_OFFSET (0x51000U)
#define ML_APP_MOD_PREINPUT_START_ADDRESS                                      \
	(HRAM_ML_IO_START_ADDR + ML_APP_MOD_PREINPUT_START_ADDRESS_OFFSET)

/* Location of ML input output buffers and size */
#define ML_APP_MOD_INPUT_START_ADDRESS_OFFSET (0x00U)
#define ML_APP_MOD_INPUT_START_ADDRESS                                         \
	(HRAM_ML_IO_START_ADDR + ML_APP_MOD_INPUT_START_ADDRESS_OFFSET)
#define ML_APP_MOD_INOUT_SIZE           HRAM_ML_IO_SIZE

/* Network Identifier */
#define ML_APP_MOD_NETWORK_ID           0x2001

/**
 * Application shall modify the placeholders with appropriate network handlers
 */
#define ML_APP_1_NETWORK_ID             ML_APP_MOD_NETWORK_ID
#define ML_APP_1_PREINPUT_START_ADDRESS ML_APP_MOD_PREINPUT_START_ADDRESS
#define ML_APP_1_INPUT_START_ADDRESS    ML_APP_MOD_INPUT_START_ADDRESS
#define ML_APP_1_INOUT_SIZE             ML_APP_MOD_INOUT_SIZE

/**
 * The enum app_module_t defines the return codes from some of the functions
 * implemented in the app module.
 */
enum app_ret_code {
	APP_CODE__SUCCESS = 0,     // Operation completed successfully
	APP_CODE__FAILURE = -100,  // Go to the previous step in the flow

	// Keep adding more error codes as needed.
};

/**
 * app_preinit() is called by the FW Core before the main application
 * initialization. It is an opportunity for App Module to perform any
 * pre-initialization tasks such as setting any parameters which could be used
 * by the FW Core initialization.
 *
 * The returned handle is passed back to the app_*() routines where the App
 * Module can use it to access the App Module's context.
 */
app_handle_t app_preinit(void);

/**
 * app_init() is called by the FW Core to initialize the App Module. Before this
 * function is called the FW Core has already initialized the hardware blocks
 * and its internal structures. This call is an opportunity for the App Module
 * to initialize its own structures and prepare for the application flow.
 * If the App Module wants to start camera capture then it should call
 * start_image_capture() from within this function.
 */
app_handle_t app_init(app_handle_t app_context);

/**
 * app_preprocess() is called after the captured image is available before the
 * ML engine starts to work on it. This function is an opportunity for the
 * App Module to perform any preprocessing on the image data.
 */
enum app_ret_code app_preprocess(app_handle_t app_context, void *image_data);

/**
 * app_ml_done() is called by the FW Core when the ML engine has finished
 * processing the image data. This function is an opportunity for the App Module
 * to perform any post-processing on the ML results.
 */
enum app_ret_code app_ml_done(app_handle_t app_context, void *ml_results);

/**
 * app_image_processing_done() is called by the FW Core when the image
 * processing operation has completed. Any further processing such as sending
 * the results to the Host will be done here. This function is called only when
 * the App Module explicitly requests FW Core by calling
 */
enum app_ret_code app_image_processing_done(app_handle_t app_context);

/**
 * app_rescale_done() is called by the FW Core when the image rescaling
 * operation has completed. This function is an opportunity for the App Module
 * to perform any actions needed after the image has been rescaled.
 */
enum app_ret_code app_rescale_done(app_handle_t app_context);

#endif /* APP_MODULE_H */
