//=============================================================================
//
// Copyright(c) 2025 Lattice Semiconductor Corp. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by LSCC and are protected 
// by copyright law. They may not be disclosed to third parties or copied 
// or duplicated in any form, in whole or in part, without the prior 
// written consent of LSCC.
//
//=============================================================================

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Represents the state of EVE processing.
	 *
	 * Returning EVE_REQUESTED_PROCESSING_STATE_STOP in the callback will shut 
	 * the processing thread down, meaning that EVE will then be ready to exit.
	 */
	enum EveRequestedProcessingState
	{
		EVE_REQUESTED_PROCESSING_STATE_CONTINUE, /**< EVE continues processing images */
		EVE_REQUESTED_PROCESSING_STATE_STOP /**< EVE stops processing images */
	};

	/**
	 * Data returned from EVE processing callback to EVE.
	 * 
	 * This is how the client application can communicate with EVE in the processing thread.
	 */
	struct EveProcessingCallbackReturnData
	{
		enum EveRequestedProcessingState requestedState; /**< EVE requested processing state */
	};

#ifdef __cplusplus
}
#endif
