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

#ifndef ASSERT_H
#define ASSERT_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "types.h" // instead of std bool
#include "errors.h"

//=============================================================================
// M A C R O S

#ifdef DEBUG
#define assert(condition, err, format, ...) assert_dbg(condition, err, format, ## __VA_ARGS__ )
#else
#define assert(...)
#endif

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Check condition is true and exits the program otherwise. 
// Before exiting, the function will display a message the same way as printf,
// meaning format and subsequent arguments can be used the same way printf would
// be. On exit, the provided error code will be sent out.
void assert_dbg( bool condition, int err, const char *format, ... );

// Check condition is true and exists the program otherwise. On exit, the
// provided error code will be sent out.
void assert_rel( bool condition, int err, const char *format, ... );

#endif