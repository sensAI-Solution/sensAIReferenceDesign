//=============================================================================
//
// Copyright(c) 2022 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================

#ifndef POSTPROCESSING_MULTI_FIXED_POINT_H
#define POSTPROCESSING_MULTI_FIXED_POINT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>

#include "fixed_point.h"

//=============================================================================
// C O N S T A N T S   D E C L A R A T I O N S

#define MULTI_RAW_TO_FP_MAX_ARGS 3

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Function transforming multiple raw fixed point into one interpreted value.
// For example, it could be FPAdd applied to two raw values to get their sum.
typedef fp_t (*MultiRawToFPFunction) ( fp_t *, size_t );

// Contains all data required to interpret an multiple arrays of int16 values asctime
// an array of fp_t.
typedef struct
{
    int16_t **dataPtr; // arrays of arrays containing raw data
    size_t argsNb;     // number of raw data arrays
    int32_t fracBits;  // since the raw data is signed fixed point,
                       // we need the number of fraction bits
    MultiRawToFPFunction multiRawToFP; // function to transform the raw data to
                                       // fixed point numbers
} multi_fp_postprocessing_config_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

//-----------------------------------------------------------------------------
//
multi_fp_postprocessing_config_t CreateMultiFPPostprocessingConfig(
    int16_t **dataPtr, // arrays of arrays containing raw data
    size_t argsNb,     // number of raw data arrays
    int32_t fracBits,  // since the raw data is signed fixed point,
                       // we need the number of fraction bits
    MultiRawToFPFunction multiRawToFP ); // function to transform the raw data to
                                       // fixed point numbers

// Interpret multiple raw data values as to fixed point number.
// The processed fixed point numbers are those located at the values contained
// in array indices.
// As an example, if indices = {4, 6, 1}, the function will process the 4th,
// 6th and first position in the config.dataPtr arrays to create fixed point
// numbers.
void MultiRawToFP(
    const size_t *indices, // position to process in raw data
    size_t size,           // number of positions to process
    const multi_fp_postprocessing_config_t *config, // postprocessing data
    fp_t *values ); // output array of fixed point numbers

#endif
    