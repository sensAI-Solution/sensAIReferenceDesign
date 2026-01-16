/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef FW_GLOBALS_H
#define FW_GLOBALS_H

#include "gard_types.h"
#include "iface_support.h"
#include "pipeline_ops.h"

/* iface_inst holds interface contexts to Host */
extern struct iface_instance iface_inst[MAX_IFACES_SUPPORTED];

/* valid_ifaces tells how many iface_inst[*] are valid. */
extern uint32_t valid_ifaces;

/* sd is pointer to storage device. */
extern void *sd;

/* ml_engine_work_done tells the status of ML engine completion. */
extern bool ml_engine_work_done;

/* Flag which indicates if camera has started. */
extern bool camera_started;

/* Flag which dictates if Auto Exposure should be done. */
extern bool auto_exposure_enabled;

/**
 * call_app_image_processing_done when true triggers call to
 * app_image_processing_done() function from FW Core's main loop.
 */
extern bool call_app_image_processing_done;

/* Buffer holding the data from App counterpart on Host. */
extern uint8_t *app_rx_buffer;

/* Size of the above buffer. */
extern uint32_t app_rx_buffer_size;

/* App Module provided callback to be called on receiving the data.*/
extern rx_handler_t app_rx_handler;

/* Buffer containing the data to be sent to Host. */
extern uint8_t *app_tx_buffer;

/* Size of the above buffer. */
extern uint32_t app_tx_buffer_size;

/* Pointer to variable which should be set to true when tx completes. */
extern uint8_t *p_app_tx_complete;

#ifdef NO_CAPTURE_DONE_ISR
/* TBD-KDB: To be removed when Capture done ISR becomes available. */
extern bool capture_started;
#endif

#ifdef NO_ML_DONE_ISR
/* TBD-SRP: To be removed when ML Done ISR becomes available. */
extern bool ml_engine_started;
#endif

#ifdef NO_RESCALE_DONE_ISR
/* TBD-SRP: To be removed when ML Done ISR becomes available. */
extern bool rescaling_started;
#endif

#ifdef NO_BUFF_MOVE_DONE_ISR
/* TBD-SRP: To be removed when Buffer move done ISR becomes available. */
extern bool buffer_move_to_ml_started;
#endif

extern uint32_t waiting_to_copy_image;

/**
 * ml_pipeline_pause_request stores the trigger at which the pipeline should
 * transition into the paused state.
 */
extern enum pipeline_pause_trigger ml_pipeline_pause_request;

/**
 * ml_pipeline_paused_at records the stage that actually honored the pause
 * request.
 */
extern enum pipeline_stage_id ml_pipeline_paused_at;

/**
 * ml_pipeline_state reflects whether the capture/ML pipeline is running,
 * awaiting a pause trigger, or paused.
 */
extern enum pipeline_state ml_pipeline_state;

/**
 * ml_pipeline_last_completed_stage records the most recent pipeline stage that
 * signaled completion.
 */
extern enum pipeline_stage_id ml_pipeline_last_completed_stage;

#endif /* FW_GLOBALS_H */
