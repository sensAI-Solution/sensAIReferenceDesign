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

#ifndef RANGE_H
#define RANGE_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "app_assert.h"
#include "errors.h"
#include "fixed_point.h"

//=============================================================================
// S T R U C T   D E C L A R A T I O N S

// Represents a range [min, max]
typedef struct
{
    const fp_t min;
    const fp_t max; // max is included in the range.
} fp_range_t;

//=============================================================================
// M A C R O S   D E C L A R A T I O N S

// Creates a literal fp_range_t struct. This is useful to initialize static or
// global variables. literalFPMin and literalFPMax have to be literal fp_t
// struct.
#define CreateLiteralFPRange( literalFPMin, literalFPMax ) \
{ \
    .min = literalFPMin, \
    .max = literalFPMax \
}

// Creates a function which linearly maps a number from an origin range to
// a target range. In this function, the origin and image ranges are fixed.
#define CreateFPMapping( name, argOriginRange, argImageRange ) \
fp_t name ( fp_t n ) \
{ \
    fp_range_t originRange = argOriginRange; \
    fp_range_t imageRange = argImageRange; \
    return FPMap( n, &originRange, &imageRange ); \
}    

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Create a fp_range_t struct.
static inline fp_range_t CreateFPRange( fp_t min, fp_t max )
{
    assert( FPLe( min, max ), EC_WRONG_ORDER, 
        "CreateFPRange: %d > %d\r\n", min.n, max.n );
    fp_range_t range = CreateLiteralFPRange( min, max );
    return range;
}

// Returns the size of range.
static inline fp_t GetFPRangeSize( const fp_range_t *range )
{
    return FPSub( range->max, range->min );
}

// Linearly maps n from the origin coordinate system to the image coordinate
// system.
fp_t FPMap( 
    fp_t n, const fp_range_t *originRange, const fp_range_t *imageRange );
#endif