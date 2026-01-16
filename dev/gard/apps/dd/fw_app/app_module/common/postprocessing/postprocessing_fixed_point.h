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

#ifndef POSTPROCESSING_FIXED_POINT_H
#define POSTPROCESSING_FIXED_POINT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>

#include "fixed_point.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Function transforming raw fixed point into its interpreted value.
// For example, it could be sigmoid applied to the raw value to get a
// confidence score.
typedef fp_t (*RawToFPFunction) ( fp_t ); 

// Contains all data required to interpret an array of int16 values as an array
// of fp_t.
typedef struct
{
    const int16_t *dataPtr;        // pointer to an array containing raw data
    int32_t fracBits;        // since the raw data is signed fixed point,
                                   // we need the number of fraction bits
    RawToFPFunction rawToFP; // function to transform the raw data to fixed 
                                   // point numbers
} fp_postprocessing_config_t;

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

fp_postprocessing_config_t CreateFPPostprocessingConfig(
    const int16_t *dataPtr, // pointer to an array containing raw data
    const int32_t fracBits,       // number of fractional bits to use when interpreting
                            // dataPtr values as fixed point numbers
    const RawToFPFunction rawToFP ); // function to transform raw data to fixed point
                               // numbers

// Interpret some of the raw data to fixed point number.
// The processed fixed point numbers are those located at the values contained
// in array indices.
// As an example, if indices = {4, 6, 1}, the function will process the 4th,
// 6th and first position in the config.dataPtr array to create fixed point
// numbers.
void RawToFP(
    const size_t *indices,                    // position to process in raw data
    size_t size,                              // number of position to process
    const fp_postprocessing_config_t *config, // postprocessing data
    fp_t *values );                           // output array of fixed point
                                              // numbers of size

#endif