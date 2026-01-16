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

#ifndef ISQRT_H
#define ISQRT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Computes the integer square root of an integer.
// n is assumed to be greater than or equal to 0.
int32_t ISqrt( int32_t n );

uint32_t ISqrt64( uint64_t n );

#endif
