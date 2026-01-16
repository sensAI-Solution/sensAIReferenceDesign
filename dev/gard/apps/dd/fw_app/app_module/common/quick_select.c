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

#include "quick_select.h"

#include "app_assert.h"
#include "errors.h"
#include "types.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//----------------------------------------------------------------------------
// Swaps the elements at positions idx1 and idx2 in array.
// It is assumed that idx1 and idx2 values are less than the array size and
// greater than or equal to zero.
static inline void Swap(
    size_t *array, // Array to swap elements from
    size_t idx1,   // index of the first element
    size_t idx2)   // index of the second element
{
    size_t tmp;
    tmp = array[idx1];
    array[idx1] = array[idx2];
    array[idx2] = tmp;
}

//----------------------------------------------------------------------------
// Groups the indices in indices array into two groups such that :
// - those whose associated value is greater than the value associated with
//   pivot are placed at the left of pivot position ;
// - those whose associated value is lesser than the value associated with
//   pivot are places at the right of pivot position.
// It is assumed that indices is an array containing one instance of each number
// from 0 to size(indices)-1.
// It is assumed that indices and confidence point to same-size arrays.
// It is assumed that left <= right.
// It is assumed that pivot, left and right values are lesser than the size of
// the indices and confidences arrays and greater then 0.
static size_t PartitionArray(
    size_t *indices,           // Indices to partition
    volatile const int16_t *score,      // Score values associated with indices
    size_t left, size_t right, // Limits of the subarray we are partitioning
    size_t pivot,              // Pivot index to partition around
    bool descending)           // Partition values such that if true,
                               // left values are greater than right and vice versa if false
{
    assert(
        left <= right, EC_OUT_OF_BOUNDS,
        "Left is greater than right: %d %d\r\n", left, right);

    int16_t pivotValue = score[indices[pivot]];
    Swap(indices, pivot, right);
    size_t storeIndex = left;

    for (size_t i = left; i <= right; ++i)
    {
        if (descending ? score[indices[i]] > pivotValue : score[indices[i]] < pivotValue)
        {
            Swap(indices, storeIndex, i);
            storeIndex += 1;
        }
    }
    Swap(indices, storeIndex, right);
    return storeIndex;
}

//----------------------------------------------------------------------------
//
void QuickSelect(
    size_t *indices,      // Indices of the confidence array
    size_t arraySize,     // Size of the indices and confidence arrays
    volatile const int16_t *score, // Contains the confidence of boxes
    size_t k)             // Number of elements to select
{
    size_t left = 0;
    size_t right = arraySize - 1;

    if (right - left + 1 <= k)
    {
        return;
    }

    while (true)
    {
        if (left == right)
        {
            return;
        }

        size_t pivot = left + (right - left) / 2;
        pivot = PartitionArray(indices, score, left, right, pivot, true);

        if (k == pivot)
        {
            return;
        }
        else if (k < pivot)
        {
            right = pivot - 1;
        }
        else
        {
            left = pivot + 1;
        }
    }
}

void QuickSortImpl(
    size_t *indices,
    const int16_t *score,
    size_t left,
    size_t right)
{
    if (left < right)
    {
        size_t pivot = left + (right - left) / 2;
        pivot = PartitionArray(indices, score, left, right, pivot, false);

        if (pivot > 0) // To prevent size_t underflow
        {
            QuickSortImpl(indices, score, left, pivot - 1);
        }
        QuickSortImpl(indices, score, pivot + 1, right);
    }
}

void QuickSort(
    size_t *indices,
    const int16_t *score,
    size_t arraySize)
{
    if (arraySize > 0)
    {
        QuickSortImpl(indices, score, 0, arraySize - 1);
    }
}