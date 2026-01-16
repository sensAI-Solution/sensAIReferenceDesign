//=============================================================================
//
// Copyright(c) 2024 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================

#ifndef PREPROCESSING_BOUNDING_BOX_H
#define PREPROCESSING_BOUNDING_BOX_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "box.h"

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Take a bounding box and returns a pixel_box enclosing the original bounding
// box. The center of both boxes is the same. The returned bounding box
// dimensions are a function of referenceFrameThickness and referenceBox
// dimensions to box dimensions ratio.
// widthRatio = box.dimensions.width / referenceBox.dimensions.width
// heightRatio = box.dimensions.height / referenceBox.dimensions.height
// largestRatio = Max(widdthRatio, heightRatio)
// New width = box.dimensions.width + 2*referenceFrameThickness * largestRatio
// New height = box.dimensions.height + 2*referenceFrameThickness * largestRatio
pixel_box_t ComputeRoIFromBoundingBox(
    const geometric_box_t *box,            // bounding box to enlarge
    int32_t referenceFrameThickness,       // number of pixels to add to each 
                                           // side if bestRatio is 1
    const geometric_box_t *referenceBox ); // box to compare to to establish ratio

#endif