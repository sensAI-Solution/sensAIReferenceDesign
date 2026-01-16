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

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "postprocessing_bounding_box.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void RawToBoundingBoxes(
    const size_t *indices,
    size_t size,
    const bounding_boxes_postprocessing_config_t *config,
    geometric_box_t *boxes )
{
    for( size_t i = 0; i < size; ++i )
    {
        anchor_grid_coords_t coords = 
            IndexToGridCoordinates( indices[i], config->gridDim);
        anchor_t anchor = config->gridCoordinatesToAnchor( &coords );
        
        fp_t rawDeltaX =
            InterpretIntAsFP( config->deltaXPtr[indices[i]], config->fracBits );
        fp_t rawDeltaY =
            InterpretIntAsFP( config->deltaYPtr[indices[i]], config->fracBits );
        fp_t rawDeltaW =
            InterpretIntAsFP( config->deltaWPtr[indices[i]], config->fracBits );
        fp_t rawDeltaH =
            InterpretIntAsFP( config->deltaHPtr[indices[i]], config->fracBits );
        
        boxes[i] = config->rawToBoundingBox(
            rawDeltaX, rawDeltaY, rawDeltaW, rawDeltaH, &anchor );
    }
}