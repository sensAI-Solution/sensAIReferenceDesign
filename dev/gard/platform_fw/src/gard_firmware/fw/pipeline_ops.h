/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef PIPELINE_OPS_H
#define PIPELINE_OPS_H

#include "gard_types.h"

/**
 * GARD__IS_PIPELINE_ACTIVE() reports whether any capture/rescale/ML stages are
 * still running so we know if a resume must kick off a fresh capture.
 */
#define GARD__IS_PIPELINE_ACTIVE()                                             \
	({ (capture_started || rescaling_started || ml_engine_started); })

/**
 * enum pipeline_pause_trigger enumerates the points in the capture/rescale/ML
 * pipeline where execution can be paused.
 */
enum pipeline_pause_trigger {
	PIPELINE_PAUSE_NONE = 0,
	PIPELINE_PAUSE_ASAP,
	PIPELINE_PAUSE_ON_CAPTURE_DONE,
	PIPELINE_PAUSE_ON_RESCALE_DONE,
	PIPELINE_PAUSE_ON_ML_DONE,
	PIPELINE_PAUSE_ON_ML_POST_PROCESSING_DONE,
};

/**
 * enum pipeline_stage_id represents the physical stages that raise completion
 * events. This is similar to the pause triggers but does not include the ASAP
 * sentinel.
 */
enum pipeline_stage_id {
	PIPELINE_STAGE_UNKNOWN = 0,
	PIPELINE_STAGE_CAPTURE_DONE,
	PIPELINE_STAGE_RESCALE_DONE,
	PIPELINE_STAGE_ML_DONE,
	PIPELINE_STAGE_ML_POST_PROCESSING_DONE,
};

/**
 * enum pipeline_state captures the coarse pause state for the ML pipeline.
 */
enum pipeline_state {
	PIPELINE_RUNNING = 0,
	PIPELINE_PAUSE_PENDING,
	PIPELINE_PAUSED,
};

/**
 * pause_pipeline_execution_async() marks the requested stage at which the
 * pipeline should transition to a paused state. The hardware continues running
 * until the specified stage completes (or the next stage if ASAP is selected).
 */
void pause_pipeline_execution_async(enum pipeline_pause_trigger when);

/**
 * resume_pipeline_async() clears the pause request and schedules the pipeline
 * to restart when the hardware reaches a safe point (i.e. resumption is not
 * instantaneous if engines are still running).
 */
void resume_pipeline_async(void);

/**
 * is_pipeline_execution_paused() reports whether the pipeline has transitioned
 * to the paused state.
 */
bool is_pipeline_execution_paused(void);

/**
 * pipeline_stage_completed() must be invoked by each stage completion ISR so
 * the pause controller can record progress and, if the requested trigger has
 * been satisfied, transition into the PAUSED state.
 */
void pipeline_stage_completed(enum pipeline_stage_id completed_stage);

#endif /* PIPELINE_OPS_H */
