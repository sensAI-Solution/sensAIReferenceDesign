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

#include "EveSdkExports.h"
#include "EveImageStructs.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Gets the processed image data from EVE. 
	 * 
	 * Image data needs to be copied out of EveProcessedImage if the data will be used outside of the callback 
	 * otherwise, threading issue may occur as EVE will overwrite the data on next frame.
	 *
	 * @return The processed image data, check EveError in EveProcessedImage for any errors.
	 */
	struct EveProcessedImage EveSDK_EXPORT EveGetProcessedImage();

	/**
	 * Gets the processed frame time data from EVE.
	 *
	 * @return The processed frame time data, check EveError in EveProcessedFrameTime for any errors.
	 */
	struct EveProcessedFrameTime EveSDK_EXPORT EveGetProcessedFrameTime();

#ifdef __cplusplus
}
#endif
