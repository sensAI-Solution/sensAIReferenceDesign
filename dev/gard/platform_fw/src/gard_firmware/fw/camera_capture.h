/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef CAMERA_CAPTURE_H
#define CAMERA_CAPTURE_H

#include "gard_types.h"
#include "image_info.h"

/**
 * setup_camera_capture_parameters() sets up the camera capture parameters such
 * as input image dimensions, crop dimensions, and output image dimensions.
 */
void setup_camera_capture_parameters(void);

/**
 * setup_image_rescale_parameters() sets up the image rescale parameters such
 * as input image dimensions, crop dimensions, and output image dimensions.
 */
void setup_image_rescale_parameters(void);

/**
 * capture_done_isr() is the ISR that is called when the 2 stage scaler engine
 * has completed the image capture process.
 *
 * @param ctxt is a pointer to the context passed to the ISR. It is not used in
 * this implementation.
 *
 * @return None
 */
void capture_done_isr(void *ctxt);

/**
 * rescaling_done_isr() is the ISR that is called when the rescaling engine has
 * completed the image capture and scaling process.
 */
void rescaling_done_isr(void *ctxt);

/**
 * capture_rescaled_image_async() initiates capture of an image intended for
 * HUB consumption.
 */
void capture_rescaled_image_async(void);

/**
 * is_rescaled_image_captured() reports whether the rescaled image buffer is
 * ready for HUB access.
 */
bool is_rescaled_image_captured(void);

/**
 * get_rescaled_image_info() returns metadata describing the rescaled image.
 */
void get_rescaled_image_info(struct image_info *rescaled_image);

#endif /* CAMERA_CAPTURE_H */
