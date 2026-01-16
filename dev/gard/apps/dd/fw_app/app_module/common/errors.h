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

#ifndef ERRORS_H
#define ERRORS_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

//=============================================================================
// S T R U C T   D E C L A R A T I O N S
enum ErrorCode
{
    EC_SUCCESS = 0
    , EC_BAD_START_SIGNAL   // Start signal was 0
    , EC_ZERO_VALUE         // Value was 0
    , EC_NOT_EVEN           // Value not even
    , EC_NEGATIVE_VALUE     // Value was less than 0 (should be using unsigned)
    , EC_NEGATIVE_OR_ZERO   // Value was 0 or negative
    , EC_OUT_OF_BOUNDS      // Value was out of our expected bounds
    , EC_FP_NOT_EQUIVALENT  // Fixed point values not equivalent
    , EC_FP_CANNOT_REPRESENT_NUMBER // Some number cannot be represented with fixed point
    , EC_ROI_OUT_OF_BOUNDS  // Explicit, indicating the ROI for crop/scale.
    , EC_LIMITED_RESOURCE_SHORTAGE // Some limited resource is depleted
    , EC_RELEASING_NON_ALLOCATED_RESOURCE // Releasing a non allocated resource
    , EC_CLOCK_NOT_INITIALIZED // The clock hasn't been initialized properly
    , EC_TIMER_INTERRUPT_BUSY // Only one timer interrupt is allowed
    , EC_MATRIX_FRACBITS_DONT_MATCH // Matrix arguments of a fp operation don't
                                    // use the same number of fractional bits
    , EC_MATRICES_DIM_DONT_MATCH    // Matrix arguments dimensions of an 
                                    // operation are not compatible
    , EC_INVALID_MATRIX             // Matrix can't store the result of an 
                                    // operation
    , EC_VECTOR_NORM_IS_ZERO        // Vector norm is zero when it shouldn't be
    , EC_MATRIX_ALREADY_FILTERED    // Tried to filter an already filtered matrix
    , EC_MATRIX_BAD_FILTER_LENGTH   // Tried to use a filter greater than rows/cols
    , EC_MATRIX_BAD_RESHAPE_DIM     // Tried to reshape a matrix to wrong dimesnions
    , EC_SCALE_ROTATION_ESTIMATOR_NOT_READY // scale_rotation_estimator_t wasn't
                                            // properly initialized before use
    , EC_VALUE_NULL  // The pointer being considered is null
    , EC_UNEXPECTED_INT // An unexpected interrupts was triggered
    , EC_CAPTURE_IS_RUNNING // The camera image capture runs when it shouldn't
    , EC_SCALER_IS_RUNNING // The scaler runs when it shouldn't
    , EC_ML_ENGINE_IS_RUNNING // The ML engine runs when it shouldn't
    , EC_ADDRESS_NOT_ALIGNED // An address is not properly aligned
    , EC_INTERRUPT_PENDING // The interrupt queue is not empty when it should be
    , EC_QUEUE_UNINITIALIZED
    , EC_QUEUE_OVERFLOW
    , EC_QUEUE_UNDERFLOW
    , EC_FUNCTION_NOT_IMPLEMENTED
    , EC_WRONG_ORDER // Values were expected in a certain order and were not
    , EC_NOT_ENOUGH_SPACE
};

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S
#endif