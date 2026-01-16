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

#include "postprocessing_filters.h"


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
size_t FilterOutBelowThreshold (
    fp_t threshold,  // Threshold value
    size_t size,     // Size of the input lists
    size_t *indices, // Indices list
    fp_t *score )    // Score list
{
    size_t i = 0;
    while( i < size )
    {
        if ( FPLe( score[i], threshold ) )
        {
            // Moving the last element of the lists to replace the removed one
            score[i] = score[size - 1];
            indices[i] = indices[size - 1];
            // Lists are now 1 element shorter
            size -= 1;
            // Since we have to test the element we just moved at index i
            // we don't increment i to test the new element during next loop
            // iteration
        }
        else
        {
            i += 1;
        }
    }
    return size;
}

//-----------------------------------------------------------------------------
//
size_t FilterOutBelowIoUThresholdWithClassSwitch (
    fp_t iouThreshold, // IoU value below which two bounding boxes are different
    size_t size,       // Size of the input lists
    size_t *indices,   // Indices list
    fp_t *score,       // Scores list
    geometric_box_t *boxes, // Bounding boxes list
    int32_t *classes, // Classes list
    bool compareDifferentClasses ) // Compare boxes of different classe (true) or not (false)
{
    // Index of the first bounding box
    size_t index1 = 0;

    while ( index1 < size )
    {
        geometric_box_t box1 = boxes[index1];

        // Index of the second bounding box
        size_t index2 = index1 + 1;
        while ( index2 < size )
        {
            if( compareDifferentClasses || ( classes[index1] == classes[index2] ) )
            {
                geometric_box_t box2 = boxes[index2];
                fp_t iou = ComputeGeometricIoU( box1, box2 );

                if( FPLt( iou, iouThreshold ) )
                {
                    index2 += 1;
                }
                else
                {
                    // We keep the bounding box whose score is the highest
                    size_t winnerIndex =
                        FPGt( score[index1], score[index2] ) ?
                        index1 :
                        index2;

                    // The first bounding box is replaced by the winning one
                    indices[index1] = indices[winnerIndex];
                    score[index1] = score[winnerIndex];
                    boxes[index1] = boxes[winnerIndex];

                    // The second bounding box is replaced by the last bounding box
                    // of the list
                    indices[index2] = indices[size - 1];
                    score[index2] = score[size - 1];
                    boxes[index2] = boxes[size - 1];

                    if( !compareDifferentClasses )
                    {
                        classes[index1] = classes[winnerIndex];
                        classes[index2] = classes[size - 1];
                    }

                    // The lists are now 1 element shorter
                    size -= 1;

                    // Since we have to test the element we just moved at index
                    // index2, we don't increment index2 to test the new element
                    // during next loop iteration
                }
            }
            else
            {
                index2 += 1;
            }
        }
        index1 += 1;
    }
	return size;
}

size_t FilterOutBelowIoUThreshold (
    fp_t iouThreshold, // IoU value below which two bounding boxes are different
    size_t size,       // Size of the input lists
    size_t *indices,   // Indices list
    fp_t *score,       // Scores list
    geometric_box_t *boxes ) // Bounding boxes list
{
    return FilterOutBelowIoUThresholdWithClassSwitch(
        iouThreshold,
        size,
        indices,
        score,
        boxes,
        NULL,
        true );
}

size_t FilterOutSameClassBelowIoUThreshold (
    fp_t iouThreshold, // IoU value below which two bounding boxes are different
    size_t size,       // Size of the input lists
    size_t *indices,   // Indices list
    fp_t *score,       // Scores list
    geometric_box_t *boxes, // Bounding boxes list
    int32_t *classes ) // Classes list
{
    return FilterOutBelowIoUThresholdWithClassSwitch(
        iouThreshold,
        size,
        indices,
        score,
        boxes,
        classes,
        false );
}
