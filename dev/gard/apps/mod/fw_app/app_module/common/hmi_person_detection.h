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

#ifndef HMI_PERSON_DETECTION_H
#define HMI_PERSON_DETECTION_H

//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "range.h"
// #include "scaler.h"

// #include "config/camera_config/camera_config.h"

#include "postprocessing/postprocessing_anchor_based_detection.h"

//=============================================================================
// C O N S T A N T S

#define PERSON_DETECTION_CAP 40 // limit is ~70

//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

int32_t PersonDetection(
        uint32_t PERSON_DETECTION_NETWORK_OUTPUT_ADDR,
        fp_t *confidence,
        geometric_box_t *boxes,
        bool *isFrontal,
        fp_t *isFrontalConfidence,
        fp_t *isNotFrontalConfidence);

#endif