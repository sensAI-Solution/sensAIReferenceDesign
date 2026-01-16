/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "fw_globals.h"

/**
 * This file holds all the instances of global variables that are used
 * throughout the firmware. Holding the variables here avoids them to be
 * scattered across different source files.
 */

#include "gard_types.h"
#include "gpio.h"

/**
 * iface_inst[] holds the instances of all the interfaces connected to the Host
 * for servicing Host commands and other data transfers.
 */
struct iface_instance iface_inst[MAX_IFACES_SUPPORTED];

/**
 * valid_ifaces tells how many interfaces are currently valid.
 */
uint32_t valid_ifaces = 0;

/**
 * sd is a global pointer to the storage device instance. This pointer is used
 * as a parameter to the OSPI functions when accessing the storage device.
 */
void *sd;

/**
 * ml_engine_work_done is a flag that is set to true when the ML engine has
 * completed processing the image. This flag is used to notify the App Module
 * that the ML engine has finished processing the image and the results are
 * available for further processing.
 */
bool ml_engine_work_done;

/**
 * call_app_image_processing_done is a flag that is set to true when the
 * FW core's mainloop should call the app_image_processing_done() function.
 * This needs to be set everytime the callback should be done.
 */
bool call_app_image_processing_done;

/**
 * The next three variables are used to hold the information that is used when
 * App data has been received from the Host and this data needs to be sent to
 * the App Module for further processing.
 */
/* Buffer holding the data from App counterpart on Host. */
uint8_t *app_rx_buffer;

/* Size of the above buffer. */
uint32_t app_rx_buffer_size;

/* App Module provided callback to be called on receiving the data.*/
rx_handler_t app_rx_handler;

/* Flag which indicates if camera has started. */
bool camera_started        = false;

/* Flag which dictates if Auto Exposure should be done. */
bool auto_exposure_enabled = true;

/**
 * The next variables are used to hold the data that is to be sent to the Host
 * by the App Module. Since the data is sent asynchronously, FW Core uses its
 * variables to send the data in the backgroud while the App Module continues to
 * generate new data to be sent.
 */
/* Buffer containing the data to be sent to Host. */
uint8_t *app_tx_buffer;

/* Size of the above buffer. */
uint32_t app_tx_buffer_size;

/* Pointer to variable which should be set to true when tx completes. */
uint8_t *p_app_tx_complete;

#ifdef NO_CAPTURE_DONE_ISR
/* TBD-KDB: To be removed when Capture done ISR becomes available. */
bool capture_started = false;
#endif

#ifdef NO_ML_DONE_ISR
/* TBD-SRP: To be removed when ML Done ISR becomes available. */
bool ml_engine_started = false;
#endif

#ifdef NO_RESCALE_DONE_ISR
/* TBD-SRP: To be removed when ML Done ISR becomes available. */
bool rescaling_started = false;
#endif

#ifdef NO_BUFF_MOVE_DONE_ISR
/* TBD-SRP: To be removed when Buffer move done ISR becomes available. */
bool buffer_move_to_ml_started = false;
#endif

uint32_t waiting_to_copy_image                        = false;

/**
 * ml_pipeline_pause_request stores the trigger at which the pipeline should
 * transition into the paused state.
 */
enum pipeline_pause_trigger ml_pipeline_pause_request = PIPELINE_PAUSE_NONE;

/**
 * ml_pipeline_paused_at records the stage that actually honored the pause
 * request.
 */
enum pipeline_stage_id ml_pipeline_paused_at          = PIPELINE_STAGE_UNKNOWN;

/**
 * ml_pipeline_state reflects whether the capture/ML pipeline is running,
 * awaiting a pause trigger, or paused.
 */
enum pipeline_state ml_pipeline_state                 = PIPELINE_RUNNING;

/**
 * ml_pipeline_last_completed_stage records the most recent pipeline stage that
 * signaled completion.
 */
enum pipeline_stage_id ml_pipeline_last_completed_stage =
	PIPELINE_STAGE_UNKNOWN;
