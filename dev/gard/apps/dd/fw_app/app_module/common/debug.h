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
#ifndef DEBUG_STREAM_H
#define DEBUG_STREAM_H
#include <stdarg.h>

//======================================================
// M A C R O S

#ifdef DEBUG
#ifndef NHYPERRAM
#define STD_IO
#define DebugOutput(format, ...) DebugOutput_(format, ## __VA_ARGS__)
#define VDebugOutput(format, argList) VDebugOutput_(format, argList)
#else
#define DebugOutput(format, ...) DebugOutputAlt_(format, ## __VA_ARGS__)
#define VDebugOutput(format, argList) VDebugOutputAlt_(format, argList)
#endif
#else
#define DebugOutput(format, ...)
#define VDebugOutput(format, argList)
#endif

//=============================================================================
// M A C R O S

#ifdef SIMPLE_DEBUG
#define SIMPLE_DEB(str) DebugOutput(str)
#define SIMPLE_HEX(n) DebugOutput("0x%x", n)
#define SIMPLE_INT32(n) DebugOutput("%d", n)
#else
#define SIMPLE_DEB(str)
#define SIMPLE_HEX(n)
#define SIMPLE_INT32(n)
#endif

//-----------------------------------------------------------------------------
//
int DebugOutput_( const char *format, ... );
int VDebugOutput_( const char *format, va_list arglist );
void DebugOutputAlt_( const char *format, ... );
void VDebugOutputAlt_( const char *format, va_list arglist );

#endif // DEBUG_STREAM_H