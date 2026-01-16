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

#include "angles.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
// See https://en.wikipedia.org/wiki/Rotation_matrix for explanations (section
// General 3D rotations). Pitch, roll and yaw are respectively rotations around
// axis x, z and y. Order of rotation is roll, then pitch, then yaw.
euler_angles_t RotationMatrixToEulerAngles( const fp_mat_t *rotationMatrix )
{
    uint8_t fracBits = rotationMatrix->fracBits;
    fp_t zero = CreateFPInt( 0, fracBits );
    fp_t upperThreshold = CreateFP( 998, 1000, fracBits );
    fp_t lowerThreshold = CreateFP( -998, 1000, fracBits );
    euler_angles_t result;
    result.roll = zero;
    result.yaw = zero;
    result.pitch = zero;

    if( FPGt( MatGet( *rotationMatrix, 1, 2 ), upperThreshold ) )
    {
        result.yaw = FPMinus( 
            FPAtan2( 
                MatGet( *rotationMatrix, 0, 1 ),
                MatGet( *rotationMatrix, 0, 0 ) ) );
        result.pitch = FPMinus( HALF_PI );
    }
    else if( FPLt( MatGet( *rotationMatrix, 1, 2 ), lowerThreshold ) )
    {
        result.yaw = FPMinus(
            FPAtan2(
                MatGet( *rotationMatrix, 0, 1 ),
                MatGet( *rotationMatrix, 0, 0 ) ) );
        result.pitch = FPMinus( HALF_PI );
    }
    else
    {
        result.yaw = FPAtan2(
            MatGet( *rotationMatrix, 0, 2 ), 
            MatGet( *rotationMatrix, 2, 2 ) );
        result.pitch = FPAsin( FPMinus( MatGet( *rotationMatrix, 1, 2 ) ) );
        result.roll = FPAtan2(
            MatGet( *rotationMatrix, 1, 0 ),
            MatGet( *rotationMatrix, 1, 1 ) );
    }
    return result;
}

//-----------------------------------------------------------------------------
// See https://en.wikipedia.org/wiki/Rotation_matrix for explanations (section
// General 3D rotations). Pitch, roll and yaw are respectively rotations around
// axis x, z and y. Order of rotation is roll, then pitch, then yaw.
void EulerAnglesToRotationMatrix( const euler_angles_t *euler, fp_mat_t *rotMat )
{
    fp_t cos1 = FPCos( euler->yaw );
    fp_t cos2 = FPCos( euler->pitch );
    fp_t cos3 = FPCos( euler->roll );
    fp_t sin1 = FPSin( euler->yaw );
    fp_t sin2 = FPSin( euler->pitch );
    fp_t sin3 = FPSin( euler->roll );

    fp_t m11 = FPAdd( FPMul( cos1, cos3 ), FPMul( FPMul( sin1, sin2 ), sin3 ) );
    fp_t m12 = FPSub( FPMul( FPMul( cos3, sin1 ), sin2 ), FPMul( cos1, sin3 ) );
    fp_t m13 = FPMul( cos2, sin1 );
    fp_t m21 = FPMul( cos2, sin3 );
    fp_t m22 = FPMul( cos2, cos3 );
    fp_t m23 = FPMinus( sin2 );
    fp_t m31 = FPSub( FPMul( FPMul( cos1, sin2 ), sin3 ), FPMul( cos3, sin1 ) );
    fp_t m32 = FPAdd( FPMul( FPMul( cos1, cos3 ), sin2 ), FPMul( sin1, sin3 ) );
    fp_t m33 = FPMul( cos1, cos2 );

    MatSet( *rotMat, 0, 0, m11 );
    MatSet( *rotMat, 0, 1, m12 );
    MatSet( *rotMat, 0, 2, m13 );
    MatSet( *rotMat, 1, 0, m21 );
    MatSet( *rotMat, 1, 1, m22 );
    MatSet( *rotMat, 1, 2, m23 );
    MatSet( *rotMat, 2, 0, m31 );
    MatSet( *rotMat, 2, 1, m32 );
    MatSet( *rotMat, 2, 2, m33 );
}
