//=============================================================================
//
// Copyright(c) 2025 Mirametrix Inc. All rights reserved.
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

#include "debug.h"

#ifdef STD_IO
#include <stdio.h>
#endif

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

#ifdef STD_IO
//-----------------------------------------------------------------------------
//
int DebugOutput_( const char *format, ... )
{
    // char msg[256];
    // va_list arglist;
    // va_start( arglist, format );
    // vsprintf( msg, format, arglist );
    // va_end( arglist );
    // return out_stream_str( uart_write, msg );
    return 0;
}

//-----------------------------------------------------------------------------
//
int VDebugOutput_( const char *format, va_list arglist )
{
    // char msg[256];
    // vsprintf( msg, format, arglist );
    // return  out_stream_str( uart_write, msg );
    return 0;

}
#else
//-----------------------------------------------------------------------------
//
void DebugOutputAlt_( const char *format, ... )
{
    // va_list vl;

    // va_start( vl, format );
    // VFPrintF( stdout, format, vl );
}

//-----------------------------------------------------------------------------
//
void VDebugOutputAlt_( const char *format, va_list arglist )
{
    // VFPrintF( stdout, format, arglist );
}
#endif

