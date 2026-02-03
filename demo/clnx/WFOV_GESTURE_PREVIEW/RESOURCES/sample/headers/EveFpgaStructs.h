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

#include "EveErrors.h"
#include "CFpgaData.h"

#ifdef __cplusplus
extern "C"
{
#endif

	/**
	 * Represents the options for FPGA configuration.
	 */
	struct EveFpgaOptions
	{
		struct CFpgaParameters parameters; /**< FPGA parameters. */
		enum EveError error; /**< Check this error code when configuring the feature */
	};

	/**
	* Some configurable options for debugging purposes
	* turning the drawing on will enable the image YUY2 -> BGR decoder
	* on systems without a configured GPU
	*/
	struct EveFpgaDebugOptions
	{
		unsigned int enableDrawingOnImage; /**< turns the drawing feature on */
		enum EveError error; //**< error code for anything that may have gone wrong*/
	};

	/**
	 * Groups the FPGA data and the EVE error.
	 */
	struct EveFpgaData
	{
		struct CFpgaData *data; /**< FPGA data */
		enum EveError error; /**< Error reported by EVE */
	};

#ifdef __cplusplus
}
#endif
