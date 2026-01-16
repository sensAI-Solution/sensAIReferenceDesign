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
#endif // __cplusplus

	/**
	 * Represents a 2D integer point.
	 */
	struct CPoint2i
	{
		int x; /**< Coordinate x of the point */
		int y; /**< Coordinate y of the point */
	};

	/**
	 * Represents a 2D floating point point (lol).
	 */
	struct CPoint2f
	{
		float x; /**< Coordinate x of the point */
		float y; /**< Coordinate y of the point */
	};

	/**
	 * Represents a 3D integer point.
	 */
	struct CPoint3i
	{
		int x; /**< Coordinate x of the point */
		int y; /**< Coordinate y of the point */
		int z; /**< Coordinate z of the point */
	};

	/**
	 * Represents a 3D floating point point (lol, but in 3d).
	 */
	struct CPoint3f
	{
		float x; /**< Coordinate x of the point */
		float y; /**< Coordinate y of the point */
		float z; /**< Coordinate z of the point */
	};

	/**
	 * Represents a 3D float angle.
	 */
	struct CAngles3f
	{
		float pitch; /**< Angle pitch value */
		float yaw; /**< Angle yaw value */
		float roll; /**< Angle roll value */
	};

	/**
	 * Represents the coordinates of an integer rectangle.
	 */
	struct CRect2i
	{
		int left; /**< Coordinate x of the top-left corner */
		int top; /**< Coordinate y of the top-left corner */
		int right; /**< Coordinate x of the bottom-right corner */
		int bottom; /**< Coordinate y of the bottom-right corner */
	};

	/**
	 * Represents the coordinates of an integer rectangle
	 * with width and height instead of right and bottom
	 */
	struct CRect2iWH
	{
		int x; /**< Coordinate x of the top-left corner */
		int y; /**< Coordinate y of the top-left corner */
		int width; /**< Width of the rectangle */
		int height; /**< Height of the rectangle */
	};

	/**
	 * Represents the coordinates of a float rectangle
	 * with width and height instead of right and bottom
	 */
	struct CRect2fWH
	{
		float x; /**< Coordinate x of the top-left corner */
		float y; /**< Coordinate y of the top-left corner */
		float width; /**< Width of the rectangle */
		float height; /**< Height of the rectangle */
	};

	/**
	 * Represents an image's resolution.
	 */
	struct CResolution
	{
		unsigned int width; /**< Width of the image */
		unsigned int height; /**< Height of the image */
	};

	/**
	 * Represents the supported video encodings by EVE.
	 */
	enum EveVideoFormat
	{
		EVE_NONE, /**< No format is set */
		EVE_BGRA, /**< Video format is BGRA (4 channels, 8 bits per channel) */
		EVE_YUY2, /**< Video format is YUY2 (2 channels, 8 bits per channel) */
		EVE_NV12, /**< Video format is NV12 (1 channel, 8 bits per channel) */
		EVE_MJPG, /**< Video format is MJPG */
		EVE_BGR, /**< Video format is BGR (3 channels, 8 bits per channel) */
		EVE_GRAYSCALE /**< Video format is Grayscale (1 channel, 8 bits per channel) */
	};

#ifdef __cplusplus
}
#endif // __cplusplus
