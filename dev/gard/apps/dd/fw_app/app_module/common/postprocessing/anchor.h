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

#ifndef POSTPROCESSING_ANCHOR_H
#define POSTPROCESSING_ANCHOR_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "box.h"
#include "fixed_point.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// In an anchor based detection system, each anchor is located in a grid cell 
// and uses an anchor template
typedef struct
{
    const int32_t row;    // row position in the grid
    const int32_t col;    // column position in the grid 
    const int32_t anchor; // index of the anchor template
} anchor_grid_coords_t ;

// Structure to hold Anchor Information
typedef struct
{
    const geometric_point_2d_t center; // Center point
    const fp_dim_t dimensions;         // anchor basis width and height
} anchor_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Creates an anchor coordinates struct propery
static inline anchor_grid_coords_t CreateAnchorCoordinates( 
    int32_t row,
    int32_t col,
    int32_t anchorTemplateIndex )
{
    anchor_grid_coords_t coords = {
        .row = row,
        .col = col,
        .anchor = anchorTemplateIndex
    };
    return coords;
}

// Creates an anchor struct properly
static inline anchor_t CreateAnchor( 
    geometric_point_2d_t center,
    fp_dim_t dimensions )
{
    anchor_t anchor = {
        .center = center,
        .dimensions = dimensions
    };
    return anchor;
}

// Returns the anchor dimensions
static inline fp_dim_t GetAnchorDim( anchor_t *anchor )
{
    return anchor->dimensions;
}

// Returns the anchor center
static inline geometric_point_2d_t GetAnchorCenter( anchor_t *anchor )
{
    return anchor->center;
}

// Anchors are indexed from 0 to (grid width x grid height x nb of anchors per cell) - 1 .
// This function takes the grid dimensions, an index value and returns the
// coordinates of the anchor in the grid and its template index. 
// The anchor positioned in cell i, j using template k index is 
// index = i + (j x grid width) + (k x grid width x grid height)
// Note that we don't need the number of anchors per cells to retrieve the 
// anchor position and anchor template index.
anchor_grid_coords_t IndexToGridCoordinates (
    size_t index,                 // index of the anchor
    const int32_dim_t gridDim ); // grid dimensions

#endif