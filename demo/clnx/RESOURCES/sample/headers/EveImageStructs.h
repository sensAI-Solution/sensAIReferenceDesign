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

#include "CBasicStructs.h"
#include "EveErrors.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Represents the input image to be sent for processing in EVE.
	 */
	struct EveInputImage
	{
		unsigned char *data; /**< Image data */
		int width; /**< Image width */
		int height; /**< Image height */
		enum EveVideoFormat encoding; /**< Encoding of the input image */
	};

	/**
	 * Groups the processed image information the EVE error.
	 */
	struct EveProcessedImage
	{
		unsigned char* data; /**< Image data */
		int width; /**< Image width */
		int height; /**< Image height */
		int channels; /**< Number of channels of the image */
		long long timestamp; /**< Steady clock in nanoseconds, since epoch */
		enum EveError error; /**< Error reported by EVE */
	};

	/**
	 * Groups the processed frame time and the EVE error.
	 */
	struct EveProcessedFrameTime
	{
		double frameTime; /**< Frame time in seconds */
		enum EveError error; /**< Error reported by EVE */
	};

#ifdef __cplusplus
}
#endif
