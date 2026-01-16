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


//=============================================================================
// I N C L U D E   F I L E S   A N D   F O R W A R D   D E C L A R A T I O N S

#include "box.h"
#include "fixed_point.h"
#include "circular_buffer.h"

#define DEFECT_DETECTION_VECTOR_SIZE_16B        32
#define DEFECT_DETECTION_NB_REF_IMAGE           1   // Normal image
#define FRAC_BITS                               10


//=============================================================================
// S T R U C T   D E C L A R A T I O N S

typedef struct
{
    pixel_box_t roi;
    uint8_t nbChannels;
} scaler_config_t;


// Contains information used to preprocess the network input
typedef struct
{
	scaler_config_t scalerConfig;
} defect_detection_preprocessor_t;


// Contains information used to retrieve and interpret the result of the network.
typedef struct
{
    int16_t refVectors[DEFECT_DETECTION_NB_REF_IMAGE][DEFECT_DETECTION_VECTOR_SIZE_16B];
    uint32_t refVectorNorms[DEFECT_DETECTION_NB_REF_IMAGE];
    uint8_t nbRegisteredVectors;
    fp_t scoreThreshold;
    circ_bbuf_t *scoreBuffer;
    int32_t scoreAccum;
} defect_detection_postprocessor_t;


// Contains information used to run the network.
typedef struct
{
	defect_detection_preprocessor_t input;
	defect_detection_postprocessor_t output;
} defect_detection_t;


typedef struct {
    fp_t score;
    bool isDefective;
} defect_detection_result_t;


//=============================================================================
// F U N C T I O N S   D E C L A R A T I O N S

defect_detection_t InitDefectDetection( void );

bool AddReferenceVector(defect_detection_t *defectDetection, const int16_t *refVector);

bool UpdateThreshold(defect_detection_t *defectDetection, int32_t rawThreshold );

defect_detection_result_t FinishDefectDetection(
    defect_detection_t *defectDetection,
    const int16_t *outputVector 
);
