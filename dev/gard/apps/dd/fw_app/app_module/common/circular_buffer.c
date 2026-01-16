//=============================================================================fC
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

#include "circular_buffer.h"
#include "app_assert.h"


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
int PushToBuffer( circ_bbuf_t *buffer, int32_t data )
{
    // Full
    if( buffer->count == buffer->maxlen ) 
    {
        return -1;
    }

    buffer->buffer[buffer->head] = data;
    buffer->head = (buffer->head + 1) % buffer->maxlen;
    ++buffer->count;
    return 0;
}

//-----------------------------------------------------------------------------
//
bool PopFromBuffer( circ_bbuf_t *buffer, int32_t *data )
{
    // Empty
    if( buffer->count == 0 )
    {
        return false;
    }

    *data = buffer->buffer[buffer->tail];
    buffer->tail = ( buffer->tail + 1 ) % buffer->maxlen;
    --buffer->count;
    return true;
}

//-----------------------------------------------------------------------------
//
void ResetBuffer( circ_bbuf_t *c )
{
	c->head = 0;
	c->tail = 0;
    c->count = 0;
}

//-----------------------------------------------------------------------------
//
uint8_t GetBufferSize( const circ_bbuf_t *c ) 
{
    return c->count;
}

//-----------------------------------------------------------------------------
//
int32_t GetBufferValueAt( const circ_bbuf_t *buffer, uint8_t index ) 
{
    assert( index < buffer->count, EC_OUT_OF_BOUNDS, "Circular buffer index is out of bounds. Index: %u. Actual size: %u", index, buffer->count );
    uint8_t actualIndex = ( buffer->tail + index ) % buffer->maxlen;
    return buffer->buffer[actualIndex];
}