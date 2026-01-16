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

#define CAMERA_PID_VID_SIZE 8 /**< PID and VID are usually 4 characters each */
#define CAMERA_NAME_SIZE 64 /**< Maximum buffer size for the camera name */

#include "CBasicStructs.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	/**
	 * Represents the comparison operations in EVE.
	 */
	enum EveCompare
	{
		EVE_EQUAL, /**< Exact match for the specified value */
		EVE_AT_MOST, /**< Upper bound for the specified value */
		EVE_AT_LEAST /**< Lower bound for the specified value */
	};

	/**
	 * Represents a camera format.
	 */
	struct CCameraFormat
	{
		struct CResolution resolution; /**< Resolution of the camera */
		enum EveVideoFormat format; /**< Video encoding of the camera */
		float fps; /**< Frames per second of the camera */
		enum EveCompare compareResolution; /**< Filter to apply on the resolution */
		enum EveCompare compareFps; /**< Filter to apply on the fps */
	};
	
	/**
	 * Represents the camera in EVE.
	 */
	struct CCamera
	{
		int id; /**< Numerical ID, starts at 0 for the first detected camera */ 
		char pid[CAMERA_PID_VID_SIZE]; /**< PID of the camera */
		char vid[CAMERA_PID_VID_SIZE]; /**< VID of the camera */
		char name[CAMERA_NAME_SIZE]; /**< Name of the camera */
		unsigned int isHardwareCamera; /**< 1 for a physical camera, 0 for a software camera */
		unsigned int isFpgaCamera; /**< 1 if the camera is a Lattice Semiconductor FPGA, 0 if not */
		unsigned int isIrCamera; /**< 1 if the camera is an infrared camera, 0 if not */
	};

	struct CCameraParameters
	{
		int width;
		int height;
		double focalLength;
		double pixelSizeX;
		double pixelSizeY;
		double principalPointX;
		double principalPointY;
		double depthMin;
		double depthMax;
		float screenLocationXinMM;
		float screenLocationYinMM;
		int isInfrared;
	};

#ifdef __cplusplus
}
#endif // __cplusplus
