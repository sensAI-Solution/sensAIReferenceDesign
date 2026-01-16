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

#include "anchor.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

anchor_grid_coords_t IndexToGridCoordinates ( 
    size_t index,
    int32_dim_t gridDim )
{
    // Total number of cells in the grid
    int32_t gridSize = gridDim.width * gridDim.height;
    
    // Get anchor template index
    int32_t anchorIndex = index / gridSize;
    
    // Get row position
    int32_t row = ( index - anchorIndex * gridSize ) / gridDim.width;
    
    // Get column position
    int32_t col = ( index - anchorIndex * gridSize ) - row * gridDim.width;
    
    anchor_grid_coords_t coords = 
        CreateAnchorCoordinates( row, col, anchorIndex );
    
    return coords;
}