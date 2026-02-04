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

/**
 * Represents the possible errors to be returned by EVE.
 */
enum EveError
{
	EVE_ERROR_NO_ERROR, /**< EVE has reported no errors */

	EVE_ERROR_NOT_CREATED, /**< EVE was not created, call CreateEve() */

	EVE_ERROR_NOT_STARTED, /**< EVE was not started, call StartEve() */

	EVE_ERROR_PIPELINE_NOT_FOUND, /**< EVE could not load the pipeline, usually an installation issue */

	EVE_ERROR_CAMERA_MANAGER_NOT_FOUND, /**< EVE could not load the camera manager, usually an installation issue */

	EVE_ERROR_NO_CALLBACK, /**< EVE has no registered callbacks */

	EVE_ERROR_NOT_ACCESSED_FROM_CALLBACK, /**< EVE's data must be accessed from the callback thread */

	EVE_INVALID_IMAGE_ENCODING, /**< EVE's input image has an unsupported encoding */

	EVE_CALLBACK_WITHOUT_CONNECTING_TO_CAMERA, /**< EVE should not have a callback registered when it is not connected to a camera */

	EVE_CAMERA_INTERACTION_WITHOUT_CAMERA, /**< EVE was created without a camera connection, but you are attempting to interact with the camera manager */

	EVE_NO_MORE_DATA, /**< End of available data has been reached */

	EVE_INVALID_CAMERA_ID, /**< The camera ID is invalid, there is no camera with the specified ID */

	EVE_FACE_ID_INVALID_THRESHOLD, /**< The threshold specified in the face id options is invalid. Must be [0.0, 1.0] */

	EVE_ERROR_NO_CAMERA_INTERACTION_WITH_CAMERA /**< You are attempting to use a function intended for use without a camera, but you have a camera active*/
};
