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

#include "postprocessing_rotation_matrix.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void RawToRotationMatrix(
    const size_t *indices,
    size_t size,
    const rotation_matrix_postprocessing_config_t *config,
    bool *validMatrix,
    fp_mat_t *rotationMatrix )
{
    for( size_t i = 0; i < size; ++i )
    {
        // Each rotation matrix is constructed from 6 fixed point numbers.
        fp_t raw[6];
        for ( size_t j = 0; j < 6; ++j )
        {
            raw[j] = InterpretIntAsFP( 
                config->dataPtr[indices[i]*6 + j], config->fracBits );
        }
        validMatrix[i] = 
            config->rawToRotationMatrix( raw, rotationMatrix[indices[i]] );
    }
}