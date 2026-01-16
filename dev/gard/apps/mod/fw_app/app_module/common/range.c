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
#include "range.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
// FPMap applies an affine function to argument n, defined by: 
// - FPMap(originRange->min) = imageRange->min
// - FPMap(originRange->max) = imageRange->max
// If n is outside originRange, the affine function will still be applied and its
// result returned.
fp_t FPMap( 
    fp_t n, const fp_range_t *originRange, const fp_range_t *imageRange )
{
    fp_t originRangeSize = GetFPRangeSize( originRange );
    fp_t imageRangeSize = GetFPRangeSize( imageRange );
    fp_t result = FPSub( n, originRange->min );
    result = FPMul( result, imageRangeSize );
    result = FPDiv( result, originRangeSize );
    result = FPAdd( result, imageRange->min );
    return result;
}