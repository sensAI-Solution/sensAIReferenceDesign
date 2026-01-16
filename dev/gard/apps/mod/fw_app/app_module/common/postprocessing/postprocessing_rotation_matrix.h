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

#ifndef POSTPROCESSING_ROTATION_MATRIX_H
#define POSTPROCESSING_ROTATION_MATRIX_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "range.h"
#include "matrix.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Interpet raw fixed point number as a 3x3 rotation matrix
typedef bool (*RawToRotationMatrixFunction) (
    const fp_t *raw, fp_mat_t rotationMatrix );

// Contains all data required to interpret an array of fixed point numbers as
// an array of rotations matrices.
typedef struct
{
    const int16_t *dataPtr;
    const int32_t fracBits;
    RawToRotationMatrixFunction rawToRotationMatrix;
} rotation_matrix_postprocessing_config_t;

//=============================================================================
// M A C R O S   D E C L A R A T I O N S

// Defines a function to interpret an array of fixed point numbers as a 3x3
// rotation matrix. Arguments argOriginRange and argImageRange are fixed point
// ranges which are use to linearly map the raw fixed point values from their 
// origin range (that is their raw value) to their interpretation range. So for
// example if the raw values are within [-24, 24] and have to be interpreted as
// values between [-1, 1], origin range should be [-24, 24] and image range 
// [-1, 1]. 
#define CreateInterpretAsRotationMatrix( \
    name, \
    argOriginRange, \
    argImageRange ) \
    bool name( \
        const fp_t *raw, \
        fp_mat_t rotationMatrix ) \
    { \
        fp_t zero = CreateFPInt( 0, raw[0].fracBits ); \
        fp_range_t originRange = argOriginRange; \
        fp_range_t imageRange = argImageRange; \
        \
        fp_t R1 = FPMap( raw[0], &originRange, &imageRange ); \
        fp_t R2 = FPMap( raw[1], &originRange, &imageRange ); \
        fp_t R3 = FPMap( raw[2], &originRange, &imageRange ); \
        fp_t R4 = FPMap( raw[3], &originRange, &imageRange ); \
        fp_t R5 = FPMap( raw[4], &originRange, &imageRange ); \
        fp_t R6 = FPMap( raw[5], &originRange, &imageRange ); \
         \
        CreateFPMat(V1, 3, 1, R1.fracBits ); \
        MatSet(V1, 0, 0, R1 ); \
        MatSet(V1, 1, 0, R2 ); \
        MatSet(V1, 2, 0, R3 ); \
        \
        fp_t normV1; \
        MatrixNorm( V1, &normV1 ); \
         \
        if( FPEq( normV1, zero ) ) \
        { \
            return false; \
        } \
         \
        InvScaleMat( V1, normV1, V1 ); \
         \
        CreateFPMat( V4, 3, 1, R1.fracBits ); \
        MatSet(V4, 0, 0, R4 ); \
        MatSet(V4, 1, 0, R5 ); \
        MatSet(V4, 2, 0, R6 ); \
         \
        CreateFPMat( V3, 3, 1, R1.fracBits ); \
        MatCrossProduct( V1, V4, V3 ); \
         \
        fp_t normV3; \
        MatrixNorm( V3, &normV3 ); \
         \
        if( FPEq( normV3, zero ) ) \
        { \
            return false; \
        } \
         \
        InvScaleMat( V3, normV3, V3 ); \
         \
        CreateFPMat( V2, 3, 1, R1.fracBits ); \
        MatCrossProduct( V3, V1, V2 ); \
         \
        MatSet( rotationMatrix, 0, 0, MatGet( V1, 0, 0 ) ); \
        MatSet( rotationMatrix, 1, 0, MatGet( V1, 1, 0 ) ); \
        MatSet( rotationMatrix, 2, 0, MatGet( V1, 2, 0 ) ); \
         \
        MatSet( rotationMatrix, 0, 1, MatGet( V2, 0, 0 ) ); \
        MatSet( rotationMatrix, 1, 1, MatGet( V2, 1, 0 ) ); \
        MatSet( rotationMatrix, 2, 1, MatGet( V2, 2, 0 ) ); \
         \
        MatSet( rotationMatrix, 0, 2, MatGet( V3, 0, 0 ) ); \
        MatSet( rotationMatrix, 1, 2, MatGet( V3, 1, 0 ) ); \
        MatSet( rotationMatrix, 2, 2, MatGet( V3, 2, 0 ) ); \
         \
        return true; \
    }

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Create rotation matrices for the elements in indices using config.
// It is assumed array validMatrix and rotationMatrix are at least size large.
void RawToRotationMatrix(
    const size_t *indices, // indices of the elements to process
    size_t size,           // size of array indices
    const rotation_matrix_postprocessing_config_t *config, // Configuration for
                                                           // rotation matrix
                                                           // processing
    bool *validMatrix,          // output rotation matrices validity
    fp_mat_t *rotationMatrix ); // output rotation matrices

#endif