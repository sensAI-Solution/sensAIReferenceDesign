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

#include "postprocessing_fixed_point.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
fp_postprocessing_config_t CreateFPPostprocessingConfig(
    const int16_t *dataPtr,
    int32_t fracBits,
    RawToFPFunction rawToFP )
{
    fp_postprocessing_config_t config = {
        .dataPtr = dataPtr,
        .fracBits = fracBits,
        .rawToFP = rawToFP
    };
    return config;
}

//-----------------------------------------------------------------------------
//
void RawToFP(
    const size_t *indices,
    size_t size,
    const fp_postprocessing_config_t *config,
    fp_t *values )
{
    for( size_t i = 0; i < size; ++i )
    {        
        // Raw FP
        values[i] = InterpretIntAsFP( config->dataPtr[indices[i]], config->fracBits );
        
        // Mapping
        if(config->rawToFP != NULL)
        {
            values[i] =
                config->rawToFP( values[i] );
        }
    }
}