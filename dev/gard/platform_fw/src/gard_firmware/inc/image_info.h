/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef IMAGE_INFO_H
#define IMAGE_INFO_H

#include "gard_types.h"
#include "gard_hub_iface.h"

/**
 * This file defines the structures and enumerations used to represent image
 * information in the GARD firmware. These structures are used to pass image
 * data between the App Module and the FW Core.
 *
 * The App Module uses these structures to pack image information for the
 * FW Core when scaling the image according to his requirements.
 */

/**
 * This file contains structures that define the image information used by the
 * App Module and the FW Core.
 * These structures are used to pass image data between the App Module and the
 * FW Core.
 */
struct image_info {
	/**
	 * Pointer to the image data buffer.
	 * This buffer contains the raw image data captured from the camera.
	 */
	void *image_data;

	/**
	 * Width of the image in pixels.
	 */
	uint16_t width;

	/**
	 * Height of the image in pixels.
	 */
	uint16_t height;

	/**
	 * Format of the image (e.g., RGB, YUV, etc.).
	 */
	enum image_formats format;

	/**
	 * Size of the image data in bytes.
	 */
	uint32_t size;
};

#endif /* IMAGE_INFO_H */
