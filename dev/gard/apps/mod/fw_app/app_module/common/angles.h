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

#ifndef ANGLES_H
#define ANGLES_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S
#include "matrix.h"

//=============================================================================
// S T R U C T S
typedef struct 
{
    fp_t pitch;
    fp_t roll;
    fp_t yaw;
} euler_angles_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Convert rotation to Euler angles.
// Pitch is rotation around the x axis.
// Yaw is rotation around the y axis.
// Roll is rotation around the z axis.
// Order of rotation is roll, then pitch, then yaw (YXZ).
euler_angles_t RotationMatrixToEulerAngles( const fp_mat_t *rotationMatrix );

// Convert Euler angles to rotation matrix.
// Pitch is rotation around the x axis.
// Yaw is rotation around the y axis.
// Roll is rotation around the z axis.
// Order of rotation is roll, then pitch, then yaw (YXZ).
void EulerAnglesToRotationMatrix( 
    const euler_angles_t *angles, fp_mat_t *rotationMatrix );

#endif