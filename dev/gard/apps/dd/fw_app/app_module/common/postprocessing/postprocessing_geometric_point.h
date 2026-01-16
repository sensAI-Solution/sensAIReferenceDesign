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

#ifndef POSTPROCESSING_GEOMETRIC_POINT_H
#define POSTPROCESSING_GEOMETRIC_POINT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "anchor.h"
#include "box.h"
#include "postprocessing_bounding_box.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Function interpreting two raw fixed point numbers as a geometric point.
typedef geometric_point_2d_t
    (*RawToGeometricPoint2DFunction) ( fp_t rawX, fp_t rawY );

// Contains all data to interpret two arrays of int16_t as an array of geometric
// 2d points. 
typedef struct
{
    const int16_t *xPtr; // array of raw x coordinates 
    const int16_t *yPtr; // array of raw y coordinates
    const int32_t fracBits; // number of fractional bits to interpret the x and
                            // y coordinates array as fixed point numbers
    
    // Transforms the raw x and y values to a geometric 2d point
    RawToGeometricPoint2DFunction rawToGeometricPoint;
} geometric_point_2d_postprocessing_config_t ;

// Computes raw deltas to geometric point using a reference anchor
typedef geometric_point_2d_t (*RawToAnchorBasedGeometricPoint2DFunction) (
        fp_t rawX, fp_t rawY, const anchor_t *anchor );

// Contains all data to interpret two arrays of int16_t as a array of geometric
// 2d points relative to anchors.
typedef struct
{
    const int16_t *xPtr; // array of raw x deltas
    const int16_t *yPtr; // array of raw y deltas 
    const int32_t fracBits; // number of fractional bits to interpret the x and
                            // y coordinates array as fixed point numbers
    const int32_dim_t gridDim; // anchor based model output grid dimensions
    
    // Computes anchor from grid coordinates
    GridCoordinatesToAnchorFunction gridCoordinatesToAnchor;
    
    // Computes raw deltas to geometric point
    RawToAnchorBasedGeometricPoint2DFunction rawToGeometricPoint;
} anchor_geometric_point_2d_postprocessing_config_t ;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Create geometric points for the elements in indices using config.
// It is assumed array point is at least size large.
void RawToGeometricPoint2D(
    const size_t *indices,
    size_t size, 
    const geometric_point_2d_postprocessing_config_t *config,
    geometric_point_2d_t *point );

// Create bounding boxes for the elements in indices using config.
// It is assumed array point is at least size large.
void RawToAnchorBasedGeometricPoint2D(
    const size_t *indices,
    size_t size, 
    const anchor_geometric_point_2d_postprocessing_config_t *config,
    geometric_point_2d_t *point );

#endif