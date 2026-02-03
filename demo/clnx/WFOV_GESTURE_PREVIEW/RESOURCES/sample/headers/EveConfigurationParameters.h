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
	 * Represents the way EVE gets images.
	 * 
	 * These modes are mutually exclusive. If you start EVE and connect to the camera, you will not be
	 * able to call EveSendImageForProcessing(). If you start EVE to provide the images yourself,
	 * then the callback will never be called.
	 */
	enum EveImageProvider
	{
		EVE_CAMERA, /**< EVE will receive images by connecting to the camera */
		EVE_CLIENT_PROVIDED /**< EVE will receive images from the client (call the EveSendImageForProcessing() function) */
	};

	/**
	 * Represents the prefered GPU to be used by EVE.
	 * 
	 * Note that this is a preference, if low power is prefered, but there is no low power
	 * GPU on the system, and only a high performance one, EVE will select the latter. Also,
	 * EVE is optimized for integrated GPUs (i.e. low power), so selecting high performance
	 * does not always lead to better frame time.
	 */
	enum EveGpuPreference
	{
		EVE_GPU_LOW_POWER, /**< Integrated GPU */
		EVE_GPU_HIGH_PERFORMANCE, /**< Dedicated GPU */
		EVE_NO_GPU /**< Skips initializing an OpenCL context, will disable most algorithms*/
	};

	/**
	 * Represents EVE's configurable startup parameters.
	 * 
	 * Please zero initialize this structure.
	 */
	struct EveStartupParameters
	{
		enum EveGpuPreference gpuPreference; /**< EVE's prefered GPU */
		enum EveImageProvider imageProvider; /**< EVE's image provider */
		char pathOverride[512]; /**< Where EVE should look for it's libraries, leave zero initialized for default behavior*/
	};

#ifdef __cplusplus
}
#endif
