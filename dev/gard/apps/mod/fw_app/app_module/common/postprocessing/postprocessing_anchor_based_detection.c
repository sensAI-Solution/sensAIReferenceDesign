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

#include "postprocessing_anchor_based_detection.h"
#include "postprocessing_filters.h"
#include "quick_select.h"

//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
int32_t PostprocessAnchorBasedDetection(
    int32_t nbOutputs,
    const fp_postprocessing_config_t *confidenceConfig,
    const bounding_boxes_postprocessing_config_t *boundingBoxConfig,
    int32_t maxBoxes,
    fp_t confidenceThreshold,
    fp_t nmsIoUThreshold,
    size_t *indices,
    fp_t *confidence,
    geometric_box_t *boxes )
{
    // Create a list of indices, ordered from 0 to nbOutputs.
    size_t currentIndices[nbOutputs];
    for( int32_t i = 0; i < nbOutputs; ++i )
    {
        currentIndices[i] = i;
    }

    int32_t maxIndex = maxBoxes < nbOutputs ? maxBoxes : nbOutputs;

    // The maxBoxes first indices of currentIndices will be the ones associated
    // with the maxBoxes greatest confidence scores.
    QuickSelect( currentIndices, nbOutputs, confidenceConfig->dataPtr, maxIndex );

    // Get the confidence scores for the maxBoxes first indices of currentIndices
    RawToFP( currentIndices, maxIndex, confidenceConfig, confidence );

    // Remove from the indices and confidence arrays the indices and confidence
    // scores lesser than or equal to the confidence threshold
    size_t nbBoxes = FilterOutBelowThreshold(
        confidenceThreshold, maxIndex, currentIndices, confidence );

    // Get the bounding boxes associated with the indices in currentIndices
    RawToBoundingBoxes( currentIndices, nbBoxes, boundingBoxConfig, boxes );

    // Remove from currentIndices, confidence and boxes the indices, confidence
    // scores and bounding boxes for which the bounding box is a duplicate of
    // another bounding box with greater confidence.
    // Duplicate identification is based on the IoU score between two bounding
    // boxes.
    if( FPLt( nmsIoUThreshold, CreateFPInt( 1, nmsIoUThreshold.fracBits ) ) )
    {
        nbBoxes = FilterOutBelowIoUThreshold(
            nmsIoUThreshold,
            nbBoxes,
            currentIndices,
            confidence,
            boxes );
    }

    for ( size_t i = 0; i < nbBoxes; ++i )
    {
        indices[i] = currentIndices[i];
    }
    return nbBoxes;
}
