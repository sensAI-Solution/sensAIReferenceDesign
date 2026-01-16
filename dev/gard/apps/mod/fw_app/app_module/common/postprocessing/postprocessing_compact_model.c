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

#include "postprocessing_compact_model.h"

#include "app_assert.h"
#include "errors.h"
#include "postprocessing_filters.h"
#include "quick_select.h"

// Defines the maximum size 
#define ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE 288


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
int32_t PostprocessCompactModel(
    int32_t nbOutputs,
    const fp_postprocessing_config_t *confidenceConfig,
    const bounding_boxes_postprocessing_config_t *boundingBoxConfig,
    const fp_postprocessing_config_t *faceProbConfig,
    const fp_postprocessing_config_t *noFaceProbConfig,
    int32_t maxBoxes,
    fp_t confidenceThreshold,
    fp_t nmsIoUThreshold,
    size_t *indices,
    fp_t *confidence,
    geometric_box_t *boxes )
{
    assert( nbOutputs <= ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE ,
            EC_NOT_ENOUGH_SPACE,
            "PostprocessCompactModel: expected a %d sized indice list, got size %d\r\n",
            nbOutputs, ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE);
    
    // Prevent issues if nbOutputs > ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE
    nbOutputs = ( nbOutputs <= ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE ) ?
        nbOutputs :
        ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE;
    
    // Create a list of indices, ordered from 0 to nbOutputs.
    size_t currentIndices[ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE];
    for( int32_t i = 0; i < nbOutputs; ++i )
    {
        currentIndices[i] = i;
    }

    int16_t rawConf[ANCHOR_BASED_DETECTION_MAX_INDICES_LIST_SIZE];
    for( int32_t i = 0; i < nbOutputs; ++i )
    {
        bool face = faceProbConfig->dataPtr[i] > noFaceProbConfig->dataPtr[i];
        rawConf[i] = face ? confidenceConfig->dataPtr[i] : 0;
    }
    
    // The maxBoxes first indices of currentIndices will be the ones associated
    // with the maxBoxes greatest confidence scores.
    QuickSelect( currentIndices, nbOutputs, rawConf, maxBoxes );
    
    // Get the confidence scores for the maxBoxes first indices of currentIndices
    RawToFP( currentIndices, maxBoxes, confidenceConfig, confidence );
 
    // Remove from the indices and confidence arrays the indices and confidence
    // scores lesser than or equal to the confidence threshold
    size_t nbBoxes = FilterOutBelowThreshold(
        confidenceThreshold, maxBoxes, currentIndices, confidence );
    
    // Get the bounding boxes associated with the indices in currentIndices
    RawToBoundingBoxes( currentIndices, nbBoxes, boundingBoxConfig, boxes );
    
    // Remove from currentIndices, confidence and boxes the indices, confidence
    // scores and bounding boxes for which the bounding box is a duplicate of 
    // another bounding box with greater confidence.
    // Duplicate identification is based on the IoU score between two bounding
    // boxes.
    nbBoxes = FilterOutBelowIoUThreshold(
        nmsIoUThreshold, 
        nbBoxes,
        currentIndices,
        confidence,
        boxes );
    
    for ( size_t i = 0; i < nbBoxes; ++i )
    {
        indices[i] = currentIndices[i];
    }
    return nbBoxes;
}