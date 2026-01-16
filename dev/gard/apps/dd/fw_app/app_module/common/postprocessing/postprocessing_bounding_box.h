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

#ifndef POSTPROCESSING_BOUNDING_BOX_H
#define POSTPROCESSING_BOUNDING_BOX_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "anchor.h"
#include "box.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Function to map detection model output grid coordinates to the corresponding
// anchor. The function has to take care of finding the anchor center and
// dimensions.
typedef anchor_t (*GridCoordinatesToAnchorFunction) (
    const anchor_grid_coords_t *gridCoordinates // Coordinates to map
);

// Function to compute a bounding box from the X, Y, W and H delta and an anchor
typedef geometric_box_t (*RawToBoundingBoxFunction) ( 
        fp_t rawDeltaX, // deltaX read from the model output
        fp_t rawDeltaY, // deltaY read from the model output
        fp_t rawDeltaW, // deltaW read from the model output
        fp_t rawDeltaH, // deltaH read from the model output
        const anchor_t *anchor ); // anchor to apply the deltas to

// Contains all data required to interpret 4 arrays of int16 values as an array
// of bounding boxes.
typedef struct{
    const int16_t *const deltaXPtr; // delta X values raw data
    const int16_t *const deltaYPtr; // delta Y values raw data
    const int16_t *const deltaWPtr; // delta W values raw data
    const int16_t *const deltaHPtr; // delta H values raw data
    const int32_t fracBits;         // Number of fractional bits to interpret
                                    // raw data as fixed point numbers.
    const int32_dim_t gridDim;            // Size of the output grid of the detection
                                    // model.
    
    // Grid coordinate to anchor mapping
    const GridCoordinatesToAnchorFunction gridCoordinatesToAnchor;
    
    // Raw deltas + anchor to bounding box
    RawToBoundingBoxFunction rawToBoundingBox;
} bounding_boxes_postprocessing_config_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Create bounding boxes for the elements in indices using config.
// It is assumed array boxes is at least size large.
void RawToBoundingBoxes(
    const size_t *indices, // Indices of elements to convert to bounding boxes
    size_t size,           // Size of the indices array
    const bounding_boxes_postprocessing_config_t *config, // Configuration for
                                                          // bounding box 
                                                          // processing
    geometric_box_t *boxes );                             // Output boxes

#endif