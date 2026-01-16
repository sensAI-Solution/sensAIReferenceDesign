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

#ifndef POSTPROCESSING_ANCHOR_BASED_DETECTION_H
#define POSTPROCESSING_ANCHOR_BASED_DETECTION_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "postprocessing_fixed_point.h"
#include "postprocessing_bounding_box.h"

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

// This function postprocesses raw data coming out of a anchor based detection
// model. It will select at most maxBoxes bounding boxes whose associated
// confidence scores are above confidenceThreshold and will apply the NMS 
// algorithm on the selected bounding boxes.
// It is guaranteed to select the bounding boxes with the highest confidence
// scores.
int32_t PostprocessAnchorBasedDetection(
    int32_t nbOutputs, // Number of bounding boxes from the model
    const fp_postprocessing_config_t *confidenceConfig, // postprocessing configuration for the confidence scores
    const bounding_boxes_postprocessing_config_t *boundingBoxConfig, // postprocessing configuration for the bounding boxes
    int32_t maxBoxes, // Maximum number of bounding boxes to return
    fp_t confidenceThreshold, // Threshold below which a bounding box won't be 
                              // selected
    fp_t nmsIoUThreshold, // IoU threshold above which two bounding boxes are
                          // identified
    size_t *indices, // Indices of the bounding boxes returned
    fp_t *confidence, // Confidence scores of the bounding boxes returned
    geometric_box_t *boxes ); // Bounding boxes returned 

#endif
