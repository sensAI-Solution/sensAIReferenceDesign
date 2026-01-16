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
#include "defect_detection_module.h"
#include "assert.h"


const int32_dim_t DEFECT_DETECTION_NETWORK_INPUT_DIM =
    CreateLiteralInt32Dim(288, 384);
const uint8_t DEFECT_DETECTION_NETWORK_INPUT_CHANNELS = 3;

const fp_t DEFECT_DETECTION_SCORE_THRESHOLD = FloatToFP(0.14, FRAC_BITS); // Range [0, 2]: 0 >> no defect, 2 >> defect


#define SIZE_OF_SCORE_BUFFER 10


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

//-----------------------------------------------------------------------------
//
defect_detection_t InitDefectDetection( void )
{
    defect_detection_preprocessor_t preprocessor = {
        .scalerConfig = {
            .roi = CreatePixelBox(0, 0, DEFECT_DETECTION_NETWORK_INPUT_DIM.width, DEFECT_DETECTION_NETWORK_INPUT_DIM.height),
            .nbChannels = DEFECT_DETECTION_NETWORK_INPUT_CHANNELS
        },
    };

    CreateStaticCircularBuffer( SCORE_BUFFER, SIZE_OF_SCORE_BUFFER );
    ResetBuffer( &SCORE_BUFFER );

    defect_detection_postprocessor_t postprocessor = {
        .nbRegisteredVectors = 0,
        .scoreThreshold = DEFECT_DETECTION_SCORE_THRESHOLD,
        .scoreBuffer = &SCORE_BUFFER,
        .scoreAccum = 0
    };

    defect_detection_t defectDetection = {
        .input = preprocessor,
        .output = postprocessor
    };

    return defectDetection;
}

//-----------------------------------------------------------------------------
//
static uint32_t ComputeNormInt(const int16_t *vector, uint8_t vectorSize)
{
    uint64_t sumSq = 0;

    for (uint8_t i = 0; i < vectorSize; i++)
    {      
        sumSq += (uint32_t)vector[i] * (uint32_t)vector[i];
    }

    return ISqrt64(sumSq);
}

//-----------------------------------------------------------------------------
//
bool AddReferenceVector(defect_detection_t *defectDetection, const int16_t *refVector)
{
    GARD__ASSERT(defectDetection->output.nbRegisteredVectors < DEFECT_DETECTION_NB_REF_IMAGE,
                 "Maximum number of reference vectors reached");
    
    // Store not normalized vector
    for (uint8_t i = 0; i < DEFECT_DETECTION_VECTOR_SIZE_16B; ++i)
    {
        defectDetection->output.refVectors[defectDetection->output.nbRegisteredVectors][i] = refVector[i];
        defectDetection->output.refVectorNorms[defectDetection->output.nbRegisteredVectors] = 
            ComputeNormInt(refVector, DEFECT_DETECTION_VECTOR_SIZE_16B);    
    }
    ++defectDetection->output.nbRegisteredVectors;
    return true;
}

//-----------------------------------------------------------------------------
//
bool UpdateThreshold(defect_detection_t *defectDetection, int32_t rawThreshold )
{
    defectDetection->output.scoreThreshold = InterpretIntAsFP( rawThreshold, FRAC_BITS );
    return true;
}

//-----------------------------------------------------------------------------
//
static int32_t CosineSimilarityInt(uint32_t normA, uint32_t normB, int64_t dotProduct)
{
    // Avoid division by zero
    normA = normA == 0 ? 1 : normA;
    normB = normB == 0 ? 1 : normB;

    int64_t result = ( dotProduct << FRAC_BITS ) / ((int64_t)normA * (int64_t)normB);

    GARD__ASSERT(result < INT32_MAX, 
        "Cosine similarity result overflow");
    GARD__ASSERT(result > INT32_MIN, 
        "Cosine similarity result underflow");

    const int32_t maxSimilarity = (1 << FRAC_BITS);
    const int32_t minSimilarity = -(1 << FRAC_BITS);
    result = result > maxSimilarity ? maxSimilarity : result;
    result = result < minSimilarity ? minSimilarity : result;

    return (int32_t)result;
}

//-----------------------------------------------------------------------------
//
defect_detection_result_t FinishDefectDetection(
    defect_detection_t *defectDetection,
    const int16_t *outputVector 
    )
{
    int64_t dotProduct = 0;
    for (uint8_t i = 0; i < DEFECT_DETECTION_VECTOR_SIZE_16B; i++)
    {      
        dotProduct += (int32_t)outputVector[i] * (int32_t)defectDetection->output.refVectors[0][i];
    }

    uint32_t normA = ComputeNormInt(outputVector, DEFECT_DETECTION_VECTOR_SIZE_16B); 
    
    int32_t cosSimilarityInt = CosineSimilarityInt(
        normA,
        defectDetection->output.refVectorNorms[0],
        dotProduct );
    int32_t distance = (1 << FRAC_BITS) - cosSimilarityInt; // Move from simialrity [-1, 1] to distance [0, 2] range

    int32_t avgDistance = 0;
    uint8_t nbResults = GetBufferSize( defectDetection->output.scoreBuffer );
    if( nbResults == SIZE_OF_SCORE_BUFFER )
    {
        int32_t lastDistance;
        PopFromBuffer( defectDetection->output.scoreBuffer, &lastDistance );
        PushToBuffer( defectDetection->output.scoreBuffer, distance );

        defectDetection->output.scoreAccum = 
            defectDetection->output.scoreAccum - lastDistance + distance;

        avgDistance = defectDetection->output.scoreAccum / nbResults;
    }
    else if( nbResults > SIZE_OF_SCORE_BUFFER )
    {
        GARD__ASSERT( false, "Defect detection score buffer overflow" );
    }
    else
    {
        PushToBuffer( defectDetection->output.scoreBuffer, distance );
        defectDetection->output.scoreAccum += distance;
    }

    fp_t avgDistanceFP = InterpretIntAsFP( avgDistance, FRAC_BITS );

    return (defect_detection_result_t){.score = avgDistanceFP, .isDefective = FPGt( avgDistanceFP, defectDetection->output.scoreThreshold ) };
}
