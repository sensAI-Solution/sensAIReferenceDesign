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

#ifndef POSTPROCESSING_FILTERS_H
#define POSTPROCESSING_FILTERS_H


//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "fixed_point.h"
#include "box.h"


//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// Remove from the list of indices and the list scores elements whose score
// is lesser than or equal to the threshold.
// Those are in place changes, the input lists are modified.
//
// THIS FUNCTION DOESN'T PRESERVE ORDER OF THE LISTS' ELEMENTS.
//
// The function returns the new size of the lists.
size_t FilterOutBelowThreshold (
    fp_t threshold,  // Threshold value
    size_t size,     // Size of the input lists
    size_t *indices, // Indices list
    fp_t *score );   // Score list 

// Remove from the list of bounding boxes elements that are too similar to 
// another more trustable one. Indices and scores associated to the removed 
// elements are also removed from their respective lists.
// Those are in place changes, the input lists are modified. 
// The iouThreshold value is used to compare similarity between bounding boxes.
//
// THIS FUNCTION DOESN'T PRESERVE ORDER OF THE LISTS' ELEMENTS.
//
// The function returns the new size of the lists. 
size_t FilterOutBelowIoUThreshold (
    fp_t iouThreshold, // IoU value below which two bounding boxes are different
    size_t size,       // Size of the input lists
    size_t *indices,   // Indices list
    fp_t *score,       // Scores list
    geometric_box_t *boxes ); // Bounding boxes list

// Remove from the list of bounding boxes elements that are too similar to 
// another more trustable one. Indices, classes and scores associated to the
// removed  elements are also removed from their respective lists.
// Those are in place changes, the input lists are modified. 
// The iouThreshold value is used to compare similarity between bounding boxes.
//
// TWO BOXES BELONGING TO TWO DIFFERENT CLASSES WON'T BE COMPARED TO EACH OTHER
//
// THIS FUNCTION DOESN'T PRESERVE ORDER OF THE LISTS' ELEMENTS.
//
// The function returns the new size of the lists. 
size_t FilterOutSameClassBelowIoUThreshold (
    fp_t iouThreshold, // IoU value below which two bounding boxes are different
    size_t size,       // Size of the input lists
    size_t *indices,   // Indices list
    fp_t *score,       // Scores list
    geometric_box_t *boxes, // Bounding boxes list
    int32_t *classes ); // Classes list

#endif
