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
#include "EveErrors.h"
#include "EveCallbackReturnData.h"
#include "EveConfigurationParameters.h"
#include "EveImageStructs.h"
#include "CCameraStructs.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Creates the EVE processing pipeline.
	 * 
	 * This function must be called before any other function. If it's the first time EVE is created, 
	 * it may take ~1 minute for this function to return because various OpenCL kernels need to be
	 * compiled and cached.
	 * 
	 * @param startupParameters EVE's startup parameters
	 * 
	 * @return An EVE error representing the status of trying to create EVE's processing pipeline.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_ERROR_CAMERA_MANAGER_NOT_FOUND and EVE_ERROR_PIPELINE_NOT_FOUND.
	 */
	enum EveError EveSDK_EXPORT CreateEve( struct EveStartupParameters startupParameters );

	/**
	 * Registers the callback to access EVE's data.
	 *
	 * This is how your application gets notified that data is available. Your application is provided with a pointer to an 
	 * EveProcessingCallbackReturnData structure, which contains an EveRequestedProcessingState, to continue or stop processing,
	 * so that everything may shut down gracefully. The pointer should not be deleted or otherwise managed from your application, 
	 * only its structure members should be updated accordingly.
	 *
	 * @param callback Function that takes and EveProcessingCallbackReturnData and returns nothing.
	 *
	 * @return An EVE error representing the status of trying to register the callback to EVE.
	 *		   Possible errors are EVE_ERROR_NO_ERROR and EVE_ERROR_NO_CALLBACK.
	 */
	enum EveError EveSDK_EXPORT EveRegisterDataCallback( void ( *callback )( struct EveProcessingCallbackReturnData * ) );

	/**
	 * Starts the EVE processing pipeline.
	 *
	 * EVE's processing thread starts, and the callback (if EVE connected to a camera) will be called on every frame.
	 *
	 * @return An EVE error representing the status of trying to start EVE's processing pipeline.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_ERROR_NOT_CREATED and EVE_ERROR_NO_CALLBACK.
	 */
	enum EveError EveSDK_EXPORT StartEve();

	/**
	 * Sends an image to EVE for processing.
	 *
	 * Results can be accessed after this function returns. To use this function, make sure that EVE was 
	 * started with the EVE_CLIENT_PROVIDED for the imageProvider option of EveStartupParameters.
	 * 
	 * The IDs are incremented in order that they are reported by the OS. When you run out of cameras
	 * past a certain ID value, the error EVESDK_NO_MORE_DATA will be reported.
	 *
	 * @param image Input image to be processed by EVE.
	 *
	 * @return An EVE error representing the status of trying to send the image to EVE for processing.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_ERROR_NOT_CREATED, EVE_CALLBACK_WITHOUT_CONNECTING_TO_CAMERA and EVE_INVALID_IMAGE_ENCODING.
	 */
	enum EveError EveSDK_EXPORT EveSendImageForProcessing( struct EveInputImage image );
	enum EveError EveSDK_EXPORT EveSendImageForProcessingWithParams( struct EveInputImage image, struct CCameraParameters parameters );

	/**
	 * Shutdowns the EVE processing pipeline.
	 *
	 * This will close the connection to the camera, and stop the processing thread.
	 *
	 * @return An EVE error representing the status of trying to shutdown EVE's processing pipeline.
	 *		   Possible errors are EVE_ERROR_NO_ERROR and EVE_ERROR_NOT_CREATED.
	 */
	enum EveError EveSDK_EXPORT ShutdownEve();

#ifdef __cplusplus
}
#endif
