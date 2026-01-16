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

#ifndef QUICK_SELECT_H
#define QUICK_SELECT_H

#include <stddef.h>
#include <stdint.h>

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

//----------------------------------------------------------------------------
// Performs the QuickSelect algorithm on the indices array based on the
// values in the confidence array. This functions selects the indices
// associated with the highest values of the confidence array.
// See https://en.wikipedia.org/wiki/Quickselect for more details.
// IMPORTANT: This function changes the array indices points to.
// It is assumed that indices and confidence point to arrays of size arraySize.
void QuickSelect(
    size_t *indices,      // Indices of the confidence array
    size_t arraySize,     // Size of the indices and confidence arrays
    volatile const int16_t *score, // Contains the confidence of boxes
    size_t k);            // Number of elements to select


//----------------------------------------------------------------------------
// Performs the Quicksort algorithm on the indices array based on the
// values in the score array. This functions sorts the indices
// in order of the lowest values of the score array.
// IMPORTANT: This function changes the array indices points to.
// It is assumed that indices and confidence point to arrays of size arraySize.
void QuickSort(
    size_t *indices,
    const int16_t *score,
    size_t arraySize);

#endif