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

#include "postprocessing/postprocessing_geometric_point.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void RawToGeometricPoint2D(
    const size_t *indices,
    size_t size, 
    const geometric_point_2d_postprocessing_config_t *config,
    geometric_point_2d_t *point )
{
    for(size_t i = 0; i < size; ++i )
    {
        fp_t rawX = InterpretIntAsFP( config->xPtr[indices[i]], config->fracBits );
        fp_t rawY = InterpretIntAsFP( config->yPtr[indices[i]], config->fracBits );
        point[i] = config->rawToGeometricPoint( rawX, rawY ); 
    }
}

//-----------------------------------------------------------------------------
//
void RawToAnchorBasedGeometricPoint2D(
    const size_t *indices,
    size_t size, 
    const anchor_geometric_point_2d_postprocessing_config_t *config,
    geometric_point_2d_t *point )
{
    for(size_t i = 0; i < size; ++i )
    {
        fp_t rawX = InterpretIntAsFP( config->xPtr[indices[i]], config->fracBits );
        fp_t rawY = InterpretIntAsFP( config->yPtr[indices[i]], config->fracBits );
        anchor_grid_coords_t coords = 
            IndexToGridCoordinates( indices[i], config->gridDim );
        anchor_t anchor = config->gridCoordinatesToAnchor( &coords );
        point[i] = config->rawToGeometricPoint( rawX, rawY, &anchor ); 
    }
}