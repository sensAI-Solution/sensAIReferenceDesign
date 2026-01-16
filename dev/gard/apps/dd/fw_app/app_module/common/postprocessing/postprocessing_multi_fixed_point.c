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

#include "postprocessing_multi_fixed_point.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
multi_fp_postprocessing_config_t CreateMultiFPPostprocessingConfig(
    int16_t **dataPtr,
    size_t argsNb,
    int32_t fracBits,
    MultiRawToFPFunction multiRawToFP )
{
    multi_fp_postprocessing_config_t config = {
        .dataPtr = dataPtr,
        .argsNb = argsNb,
        .fracBits = fracBits,
        .multiRawToFP = multiRawToFP
    };
    return config;
}

//-----------------------------------------------------------------------------
//
void MultiRawToFP(
    const size_t *indices,
    size_t size,
    const multi_fp_postprocessing_config_t *config,
    fp_t *values )
{
    fp_t raw[MULTI_RAW_TO_FP_MAX_ARGS];
    for( size_t i = 0; i < size; ++i )
    {
        for( size_t j = 0; j < config->argsNb; ++j )
        {
            raw[j] = InterpretIntAsFP( 
                config->dataPtr[j][indices[i]],
                config->fracBits );
        }
        values[i] =
            config->multiRawToFP( raw, config->argsNb );
    }
}