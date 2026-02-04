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
#include "EveFpgaStructs.h"
#include "CFpgaData.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Gets the FPGA data from EVE.
	 *
	 * @return The FPGA data, check EveError in EveFpgaData for any errors.
	 */
	struct EveFpgaData EveSDK_EXPORT EveGetFpgaData();

	/**
	 * Configures EVE's connection and parameters to be passed to the FPGA.
	 *
	 * @param options FPGA options.
	 *
	 * @return An EVE error representing the status of trying to set EVE's FPGA options.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_CAMERA_INTERACTION_WITHOUT_CAMERA and EVE_ERROR_NOT_STARTED.
	 */
	struct EveFpgaOptions EveSDK_EXPORT EveConfigureFpga( struct EveFpgaOptions options );

	/**
	* Sets some optional features for debugging purposes
	*/
	struct EveFpgaDebugOptions EveSDK_EXPORT EveConfigureFpgaDebug( struct EveFpgaDebugOptions options );

	enum EveError EveSDK_EXPORT QueryFpgaSetting( struct pipeline_config_t command, bool notify );
	enum EveError EveSDK_EXPORT QueryFpgaSettings( uint16_t typeMask,
		uint32_t settingsMask, bool notify );
	enum EveError EveSDK_EXPORT SendSetSetting( struct pipeline_config_t command );
	struct CFpgaGetSetting EveSDK_EXPORT PopQueuedSetting();
	struct EveFpgaJsonMetadata EveSDK_EXPORT FpgaReadJson();


	/**
	 * Sends FPGA metadata to EVE for processing.
	 *
	 * Results will be annotated on the processed image.
	 * 
	 * Only if the Fpga connection mode is MANUAL.
	 * 
	 * @param binary input data to be processed by EVE.
	 *
	 * @return An EVE error representing the status of trying to send the data to EVE for processing.
	 *		   Possible errors are EVE_ERROR_NO_ERROR, EVE_ERROR_NOT_CREATED and EVE_CALLBACK_WITHOUT_CONNECTING_TO_CAMERA.
	 */
	enum EveError EveSDK_EXPORT EveSendFpgaDataManually( struct EveFpgaManualData data );


#ifdef __cplusplus
}
#endif
