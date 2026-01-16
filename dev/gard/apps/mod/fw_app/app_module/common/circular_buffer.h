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

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include <stdint.h>
#include "types.h" // instead of std bool


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

typedef struct
{
	int32_t *const buffer;
	uint8_t head;
	uint8_t tail;
    uint8_t count;
	const uint8_t maxlen;
} circ_bbuf_t;


//=============================================================================
// M A C R O S

#define CreateCircularBuffer( name, size )\
    int32_t name##_data_space[size];      \
    circ_bbuf_t name = {                  \
        .buffer = name##_data_space,      \
        .head = 0,                        \
        .tail = 0,                        \
        .count = 0,                       \
        .maxlen = size                    \
    }

#define CreateStaticCircularBuffer( name, size )    \
    static int32_t name##_data_space[size];         \
    static circ_bbuf_t name = {                     \
        .buffer = name##_data_space,                \
        .head = 0,                                  \
        .tail = 0,                                  \
        .count = 0,                                 \
        .maxlen = size                              \
    }


//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

int PushToBuffer( circ_bbuf_t *buffer, int32_t data );

bool PopFromBuffer( circ_bbuf_t *buffer, int32_t *data );

void ResetBuffer( circ_bbuf_t *c );

uint8_t GetBufferSize( const circ_bbuf_t *c );

int32_t GetBufferValueAt( const circ_bbuf_t *buffer, uint8_t index );

#endif
