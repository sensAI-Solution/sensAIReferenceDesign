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

#define EVE_CAMERA_FORMATS_SIZE 20 /**< Maximum number of matching camera formats */

#include "EveErrors.h"
#include "CCameraStructs.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Groups camera formats and an error.
	 * 
	 * Some cameras support a huge amount of video formats, so we choose to return at most EVE_CAMERA_FORMATS_SIZE 
	 * matching a certain filter at a time, as a compromise between ease of memory management and size. You can specify 
	 * filters on the resolution, encoding and FPS to find something specific. We typically recommend grabbing the highest 
	 * YUY2 resolution at the FPS you need as a good starting point.
	 */
	struct EveCameraFormats
	{
		struct CCameraFormat formats[EVE_CAMERA_FORMATS_SIZE]; /**< Camera formats */
		unsigned int formatsCount; /**< Number of camera formats */
		unsigned int hadMoreFormats; /**< More formats are available if the maximum limit was hit */ 
		enum EveError error; /**< Reported error by EVE */
	};

	/**
	 * Groups a camera and an error.
	 */
	struct EveCamera
	{
		struct CCamera data; /**< Camera information */
		enum EveError error; /**< Reported error by EVE */
	};

	/**
	 * Groups a camera count and an error.
	 */
	struct EveNumberOfCameras
	{
		unsigned int count; /** Number of cameras */
		enum EveError error; /** Reported error by EVE */
	};

#ifdef __cplusplus
}
#endif
