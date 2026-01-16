/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "types.h"
#include "assert.h"
#include "fw_core.h"
#include "fw_globals.h"
#include "memmap.h"
#include "utils.h"
#include "pipeline_ops.h"

/**
 * This file defines the ML operations related interfaces
 *
 * These helpers provide a hardware-agnostic way to pause and resume
 * image acquisition and inference scheduling so the HUB can safely access
 * the rescaled image buffer when needed.
 */

/**
 * GARD__TRACE_COMPLETION_STAGE_POST_PAUSE() converts the externally requested
 * pause trigger into the concrete pipeline stage the hardware will report upon
 * completion.
 */
#define GARD__TRACE_COMPLETION_STAGE_POST_PAUSE(pause_request)                 \
	({                                                                         \
		enum pipeline_stage_id mapped_stage = PIPELINE_STAGE_UNKNOWN;          \
		switch (pause_request) {                                               \
		case PIPELINE_PAUSE_ON_CAPTURE_DONE:                                   \
			mapped_stage = PIPELINE_STAGE_CAPTURE_DONE;                        \
			break;                                                             \
		case PIPELINE_PAUSE_ON_RESCALE_DONE:                                   \
			mapped_stage = PIPELINE_STAGE_RESCALE_DONE;                        \
			break;                                                             \
		case PIPELINE_PAUSE_ON_ML_DONE:                                        \
			mapped_stage = PIPELINE_STAGE_ML_DONE;                             \
			break;                                                             \
		case PIPELINE_PAUSE_ON_ML_POST_PROCESSING_DONE:                        \
			mapped_stage = PIPELINE_STAGE_ML_POST_PROCESSING_DONE;             \
			break;                                                             \
		default:                                                               \
			break;                                                             \
		}                                                                      \
		mapped_stage;                                                          \
	})

/**
 * pause_pipeline_execution_async() records the requested pause trigger so the
 * pipeline can stall once the matching stage completes.
 *
 * @param pause_request Requested stage trigger that should cause the pipeline
 *                      to pause. Use values from enum pipeline_pause_trigger.
 *
 * @return None
 */
void pause_pipeline_execution_async(enum pipeline_pause_trigger pause_request)
{
	/* Pause Request invalid */
	GARD__DBG_ASSERT(PIPELINE_PAUSE_NONE != pause_request,
					 "Invalid pause request");

	/* Already paused at the requested stage; nothing to do. */
	if (PIPELINE_PAUSED == ml_pipeline_state) {
		return;
	}

	/* Record the desired trigger so the matching stage can honor it. */
	ml_pipeline_pause_request = pause_request;

	/* Transition into pending state until the requested stage completes. */
	ml_pipeline_state         = PIPELINE_PAUSE_PENDING;
	ml_pipeline_paused_at     = PIPELINE_STAGE_UNKNOWN;
}

/**
 * resume_pipeline_async() clears any outstanding pause request and transitions
 * the ML pipeline back to running state. If no stages are currently active,
 * this function kicks off a fresh image capture to resume processing.
 *
 * @param None
 *
 * @return None
 */
void resume_pipeline_async(void)
{
	bool restart_pipeline = false;

	/* Nothing to resume if the pipeline never paused. */
	if (PIPELINE_RUNNING == ml_pipeline_state) {
		return;
	}

	/* Check whether all stages are idle so we know if we must restart capture.
	 */
	restart_pipeline          = !GARD__IS_PIPELINE_ACTIVE();

	/* Clear the pause request/state regardless of whether we restart work. */
	ml_pipeline_pause_request = PIPELINE_PAUSE_NONE;
	ml_pipeline_paused_at     = PIPELINE_STAGE_UNKNOWN;
	ml_pipeline_state         = PIPELINE_RUNNING;

	/* If no work is currently active, kick the pipeline with a fresh capture.
	 */
	if (restart_pipeline) {
		capture_image_async();
	}
}

/**
 * is_pipeline_execution_paused() reports whether the ML pipeline is currently
 * paused at a stage boundary as requested by the application or HUB.
 *
 * @param None
 *
 * @return Returns true when the pipeline state is paused; otherwise false.
 */
bool is_pipeline_execution_paused(void)
{
	return (PIPELINE_PAUSED == ml_pipeline_state);
}

/**
 * pipeline_stage_completed() is invoked when a pipeline stage finishes
 * (capture, rescale, ML, or post-processing). It updates bookkeeping and
 * applies a pending pause request if the completed stage matches the trigger.
 *
 * @param completed_stage is the identifier for the stage that has completed.
 *
 * @return None
 */
void pipeline_stage_completed(enum pipeline_stage_id completed_stage)
{
	enum pipeline_stage_id requested_stage;

	GARD__DBG_ASSERT(PIPELINE_STAGE_UNKNOWN != completed_stage,
					 "Invalid pipeline stage completion");

	/* Track most recent completion so diagnostics can see where we stopped. */
	ml_pipeline_last_completed_stage = completed_stage;

	/* Puase not requested, continue with next stage */
	if (PIPELINE_PAUSE_PENDING != ml_pipeline_state) {
		return;
	}

	/* ASAP pauses latch on the very next stage that reports completion. */
	if (PIPELINE_PAUSE_ASAP == ml_pipeline_pause_request) {
		ml_pipeline_state     = PIPELINE_PAUSED;
		ml_pipeline_paused_at = completed_stage;
		return;
	}

	/* If the requested stage is valid and the completed stage matches the
	 * requested stage, then pause the pipeline */
	requested_stage =
		GARD__TRACE_COMPLETION_STAGE_POST_PAUSE(ml_pipeline_pause_request);

	if (completed_stage == requested_stage) {
		ml_pipeline_state     = PIPELINE_PAUSED;
		ml_pipeline_paused_at = completed_stage;
	}
}
