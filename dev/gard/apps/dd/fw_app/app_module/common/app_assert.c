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

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "app_assert.h"

#include <stdarg.h>

#include "assert.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
void assert_rel( bool condition, int err, const char *format, ... )
{
	va_list args;
	va_start(args, format);
	GARD__ASSERT( condition, err, format, args );
	va_end(args);	
}

//-----------------------------------------------------------------------------
//
void assert_dbg( bool condition, int err, const char *format, ... )
{
	va_list args;
	va_start(args, format);
	GARD__DBG_ASSERT( condition, err, format, args );
	va_end(args);
}