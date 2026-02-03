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
#include "EveCameraStructs.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Lists available formats for a given camera.
	 * 
	 * If the index is greater than the camera count, the error EVESDK_NO_MORE_DATA will be returned.
	 * The filter is optional, leave it zero initialized to get the first formats reported by the camera.
	 * For ease of memory management, a contiguous block of formats is allocated
	 * If this maximum amount is reached, EveCameraFormats.hadMoreFormats will be set to 1
	 * You can then specify a filter on the resolution, and/or encoding, and/or FPS to find a specific format.
	 * 
	 * @param camera Index of the camera.
	 * @param filter Filter to apply to the camera formats.
	 * 
	 * @return The camera formats for the camera. 
	 *		   Possible errors are EVE_CAMERA_INTERACTION_WITHOUT_CAMERA, EVE_ERROR_NOT_CREATED and EVE_NO_MORE_DATA.
	 */
	struct EveCameraFormats EveSDK_EXPORT EveGetFormats( unsigned int camera, struct CCameraFormat filter );

	/**
	 * Gets the camera information for the specified camera ID.
	 *
	 * The IDs are incremented in order that they are reported by the OS. When you run out of cameras 
	 * past a certain ID value, the error EVESDK_NO_MORE_DATA will be reported.
	 *
	 * @param camera Index of the camera.
	 *
	 * @return The camera information.
	 *		   Possible errors are EVE_CAMERA_INTERACTION_WITHOUT_CAMERA, EVE_ERROR_NOT_CREATED and EVE_INVALID_CAMERA_ID.
	 */
	struct EveCamera EveSDK_EXPORT EveGetCamera( unsigned int camera );

	/**
	 * Sets the camera and format to be used by EVE.
	 *
	 * Calling this method is optional. By default EVE will connect to the first reported camera by the system.
	 * The filter is also optional, you can leave any of it's members zero initialized to allow EVE to choose
	 * the rest by itself. You can for example set the FPS to 30, leave the other fields at 0, and EVE will connect 
	 * to the highest resolution that offers 30 FPS. By default we prefer YUY2 since these formats tend to not be messed with, 
	 * through upscaling or other means. Another example: if you set the resolution to 640 x 480, leave the other fields
	 * at 0, EVE will connect to a 640 x 480 YUY2 feed at the highest available FPS.
	 *
	 * @param camera Index of the camera.
	 * @param filter Filter to apply to the camera formats.
	 *
	 * @return An EVE error representing the status of trying to set the camera in EVE.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_CAMERA_INTERACTION_WITHOUT_CAMERA, EVE_ERROR_NOT_CREATED and EVE_INVALID_CAMERA_ID.
	 */
	enum EveError EveSDK_EXPORT EveSetCamera( unsigned int camera, struct CCameraFormat filter );

	/** Gets the number of camera detected by EVE.
	 *
	 * Note that a camera can sometime output in RGB and IR, and it will be reported as two separate cameras by the function.
	 * You can then use EveGetCamera() function to get information each camera.
	 *
	 * @return The number of cameras detected by EVE.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_CAMERA_INTERACTION_WITHOUT_CAMERA, EVE_ERROR_NOT_CREATED.
	 *
	 */
	struct EveNumberOfCameras EveSDK_EXPORT EveGetNumberOfCameras();

#ifdef __cplusplus
}
#endif
