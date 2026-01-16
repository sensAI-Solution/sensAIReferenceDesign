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

// #include "interrupt.h"
#include "quick_select.h"
// #include "wait.h"

#include "preprocessing/bounding_box.h"

#include "postprocessing/postprocessing_filters.h"
// #include <convert3d.h>

// #include "ml_engine_config.h"
#include "hmi_person_detection.h"
#include "ml_engine_config.h"
#include "source_image.h"
// #include "config/addresses.h"

//=============================================================================
// C O N S T A N T S


const int32_dim_t PERSON_DETECTION_NETWORK_INPUT_DIM =
    CreateLiteralInt32Dim( 256, 144 );

// Current Person Detection NN now has 2 grids/level of detection:
const int32_dim_t PERSON_DETECTION_NETWORK_GRID_DIM_32x18 =
    CreateLiteralInt32Dim( 32, 18 );
const int32_dim_t PERSON_DETECTION_NETWORK_GRID_DIM_16x9 =
    CreateLiteralInt32Dim( 16, 9 );

const int32_t PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 =
    PERSON_DETECTION_NETWORK_GRID_DIM_32x18.width *
    PERSON_DETECTION_NETWORK_GRID_DIM_32x18.height;
const int32_t PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 =
    PERSON_DETECTION_NETWORK_GRID_DIM_16x9.width *
    PERSON_DETECTION_NETWORK_GRID_DIM_16x9.height;

const fp_t PERSON_DETECTION_CONFIDENCE_THRESHOLD = // TODO test 0.50, 0.55, 0.60
    CreateLiteralFP( 55, 100, ML_ENGINE_OUTPUT_FRAC_BITS );

const fp_t PERSON_DETECTION_NMS_THRESHOLD = 
    CreateLiteralFP( 40, 100, ML_ENGINE_OUTPUT_FRAC_BITS );


//=============================================================================
// F U N C T I O N S   C O D E   S E C T I O N

anchor_t PersonDetectionGridCoordinateToAnchor(
    const anchor_grid_coords_t *coords,
    const int32_dim_t *gridDim )
{    
    // Transform relative results from the NN to absolute values coordinates in
    // the input image
    fp_t strideX =
        CreateFPInt( PERSON_DETECTION_NETWORK_INPUT_DIM.width / 
            gridDim->width, ML_ENGINE_OUTPUT_FRAC_BITS );
    fp_t strideY =
        CreateFPInt( PERSON_DETECTION_NETWORK_INPUT_DIM.height / 
            gridDim->height, ML_ENGINE_OUTPUT_FRAC_BITS );
    
    fp_t centerX = CreateFPInt( coords->col, ML_ENGINE_OUTPUT_FRAC_BITS );
    fp_t centerY = CreateFPInt( coords->row, ML_ENGINE_OUTPUT_FRAC_BITS );
    
    fp_t half = CreateFP( 1, 2, ML_ENGINE_OUTPUT_FRAC_BITS );
    centerX = FPAdd( FPMul( strideX, half), FPMul( strideX, centerX));
    centerY = FPAdd( FPMul( strideY, half), FPMul( strideY, centerY));
    
    anchor_t anchor = {
        .center = CreateGeometricPoint( centerX, centerY ),
        .dimensions = { // set to 0x0 (unused with anchor free detection)
            .width = CreateLiteralFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS ),
            .height = CreateLiteralFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS )
        }
    };
    return anchor;
}

anchor_t PersonDetectionGridCoordinateToAnchor_32x18( const anchor_grid_coords_t *coords )
{
    return PersonDetectionGridCoordinateToAnchor( coords,
        &PERSON_DETECTION_NETWORK_GRID_DIM_32x18);
}

anchor_t PersonDetectionGridCoordinateToAnchor_16x9( const anchor_grid_coords_t *coords )
{
    return PersonDetectionGridCoordinateToAnchor( coords,
        &PERSON_DETECTION_NETWORK_GRID_DIM_16x9);
}

geometric_box_t PersonDetectionRawToBoundingBox(
    fp_t rawDeltaX, fp_t rawDeltaY, fp_t rawDeltaW, fp_t rawDeltaH,
    const anchor_t *anchor )
{    
    int32_t fracBits = rawDeltaX.fracBits;    
    fp_t deltaScale =
        CreateFPInt( PERSON_DETECTION_NETWORK_INPUT_DIM.width / 32, fracBits );

    // Compute the box top left and bottom right coordinates from center
    fp_t left = FPSub( anchor->center.x, FPMul(rawDeltaX, deltaScale) );
    fp_t right = FPAdd( anchor->center.x, FPMul(rawDeltaW, deltaScale) );
    fp_t top = FPSub( anchor->center.y, FPMul(rawDeltaY, deltaScale) );
    fp_t bottom = FPAdd( anchor->center.y, FPMul(rawDeltaH, deltaScale) );
    
    return CreateGeometricBox( left, top, right, bottom );
}

int32_t PersonDetection(
    uint32_t PERSON_DETECTION_NETWORK_OUTPUT_ADDR,
    fp_t *confidence,
    geometric_box_t *boxes,
    bool *isFrontal,
    fp_t *isFrontalConfidence,
    fp_t *isNotFrontalConfidence)
{
    // Output addresses for level 1 (16x9)
    const int16_t * const PERSON_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 4;
    const int16_t * const PERSON_DETECTION_NETWORK_FRONTAL_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 5;
    const int16_t * const PERSON_DETECTION_NETWORK_NON_FRONTAL_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 6;

    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_X1_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 0;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_Y1_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 1;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_X2_OUTPUT_ADDR_16x9 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 2;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_Y2_OUTPUT_ADDR_16x9 =
       (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 3;
        
    // Output addresses for level 2 (32x18)
    const int16_t * const PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_16x9 * 7;

    const int16_t * const PERSON_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 4;
    const int16_t * const PERSON_DETECTION_NETWORK_FRONTAL_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 5;
    const int16_t * const PERSON_DETECTION_NETWORK_NON_FRONTAL_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 6;
        
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_X1_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 0;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_Y1_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 1;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_X2_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 2;
    const int16_t * const PERSON_DETECTION_NETWORK_DELTA_Y2_OUTPUT_ADDR_32x18 =
        (int16_t *)PERSON_DETECTION_NETWORK_OUTPUT_ADDR_32x18 +
        PERSON_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET_32x18 * 3;
        
// #ifndef N2STEP_SCALER
//     scaler_source_image_t personDetectionSource = CreateScalerSourceImage(
//         0, REGULAR_FRAME_BUFFER_SOURCE_IMAGE_DIM,
//         0, REGULAR_FRAME_BUFFER_SOURCE_IMAGE_ROI, false);
// #else
//     scaler_source_image_t personDetectionSource = CreateScalerSourceImage(
//         0, SOURCE_IMAGE_DIM,
//         0, SOURCE_IMAGE_ROI, false);
// #endif

    // pixel_box_t personDetectionRoI = CreatePixelBox(
    //     0, 0,
    //     SOURCE_IMAGE_ROI.dimensions.width,
    //     SOURCE_IMAGE_ROI.dimensions.height);

    // scaler_output_image_t personDetectionOutput = CreateScalerOutputImage(
    //     (int32_t)PERSON_DETECTION_NETWORK_INPUT_ADDR,
    //     PERSON_DETECTION_NETWORK_INPUT_DIM);

    // scaler_config_t personDetectionScalerConfig = CreateScalerConfig(
    //     personDetectionSource, personDetectionRoI, false, personDetectionOutput);

    fp_postprocessing_config_t personDetectionConfidenceConfig1 =
        CreateFPPostprocessingConfig(
            PERSON_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR_32x18,
            ML_ENGINE_OUTPUT_FRAC_BITS,
            FPSigmoid );
            
    fp_postprocessing_config_t personDetectionConfidenceConfig2 =
        CreateFPPostprocessingConfig(
            PERSON_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR_16x9,
            ML_ENGINE_OUTPUT_FRAC_BITS,
            FPSigmoid );
    
    bounding_boxes_postprocessing_config_t personDetectionBoundingBoxesConfig1 = {
        .deltaXPtr = PERSON_DETECTION_NETWORK_DELTA_X1_OUTPUT_ADDR_32x18,
        .deltaYPtr = PERSON_DETECTION_NETWORK_DELTA_Y1_OUTPUT_ADDR_32x18,
        .deltaWPtr = PERSON_DETECTION_NETWORK_DELTA_X2_OUTPUT_ADDR_32x18,
        .deltaHPtr = PERSON_DETECTION_NETWORK_DELTA_Y2_OUTPUT_ADDR_32x18,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .gridDim = PERSON_DETECTION_NETWORK_GRID_DIM_32x18,
        .gridCoordinatesToAnchor = PersonDetectionGridCoordinateToAnchor_32x18,
        .rawToBoundingBox = PersonDetectionRawToBoundingBox
    };
    
    bounding_boxes_postprocessing_config_t personDetectionBoundingBoxesConfig2 = {
        .deltaXPtr = PERSON_DETECTION_NETWORK_DELTA_X1_OUTPUT_ADDR_16x9,
        .deltaYPtr = PERSON_DETECTION_NETWORK_DELTA_Y1_OUTPUT_ADDR_16x9,
        .deltaWPtr = PERSON_DETECTION_NETWORK_DELTA_X2_OUTPUT_ADDR_16x9,
        .deltaHPtr = PERSON_DETECTION_NETWORK_DELTA_Y2_OUTPUT_ADDR_16x9,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .gridDim = PERSON_DETECTION_NETWORK_GRID_DIM_16x9,
        .gridCoordinatesToAnchor = PersonDetectionGridCoordinateToAnchor_16x9,
        .rawToBoundingBox = PersonDetectionRawToBoundingBox
    };
    
    fp_postprocessing_config_t personDetectionFrontalConfig1 = {
        .dataPtr = PERSON_DETECTION_NETWORK_FRONTAL_OUTPUT_ADDR_32x18,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = FPId
    };
    
    fp_postprocessing_config_t personDetectionFrontalConfig2 = {
        .dataPtr = PERSON_DETECTION_NETWORK_FRONTAL_OUTPUT_ADDR_16x9,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = FPId
    };
    
    fp_postprocessing_config_t personDetectionNonFrontalConfig1 = {
        .dataPtr = PERSON_DETECTION_NETWORK_NON_FRONTAL_OUTPUT_ADDR_32x18,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = FPId
    };
    
    fp_postprocessing_config_t personDetectionNonFrontalConfig2 = {
        .dataPtr = PERSON_DETECTION_NETWORK_NON_FRONTAL_OUTPUT_ADDR_16x9,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = FPId
    };

    fp_range_t personDetectionCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(PERSON_DETECTION_NETWORK_INPUT_DIM.width, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t personDetectionCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(PERSON_DETECTION_NETWORK_INPUT_DIM.height, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_WIDTH, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_HEIGHT, ML_ENGINE_OUTPUT_FRAC_BITS));

    size_t nbPersons1, nbPersons2, nbPersonsTotal;
    // Arrays containing results from both of the 2 outputs/scale levels
    size_t allPersonIndices[PERSON_DETECTION_CAP * 2];
    fp_t allConfidences[PERSON_DETECTION_CAP * 2];
    int16_t allConfidencesRaw[PERSON_DETECTION_CAP * 2]; // to be used with QuickSelect() later
    geometric_box_t allBoxes[PERSON_DETECTION_CAP * 2];
    bool allIsFrontal[PERSON_DETECTION_CAP * 2];
    
    // CropAndResizeImage(&personDetectionScalerConfig, true);
    // if( !WaitForInterrupt(INT_SCALER, 500) )
    // {
    //     return 0;
    // }
    
    // // Start PD
    // RunMLEngine(
    //     (int32_t)PERSON_DETECTION_NETWORK_FIRMWARE_ADDR,
    //     (int32_t)PERSON_DETECTION_NETWORK_INPUT_ADDR,
    //     PERSON_DETECTION_NETWORK_INPUT_DIM.width *
    //         PERSON_DETECTION_NETWORK_INPUT_DIM.height,
    //     true);
    
    // // Wait for ML engine    
    // if( !WaitForInterrupt(INT_ML_ENGINE, 500) )
    // {
    //     return 0;
    // }
    
    // Postprocess PD, 32x18
    nbPersons1 = PostprocessAnchorBasedDetection(
        PERSON_DETECTION_NETWORK_GRID_DIM_32x18.width *
            PERSON_DETECTION_NETWORK_GRID_DIM_32x18.height,
        &personDetectionConfidenceConfig1,
        &personDetectionBoundingBoxesConfig1,
        PERSON_DETECTION_CAP,
        PERSON_DETECTION_CONFIDENCE_THRESHOLD,
        CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS ), // skip NMS for now
        allPersonIndices,
        allConfidences,
        allBoxes );
    
    fp_t personFrontal[PERSON_DETECTION_CAP * 2];
    fp_t personNonFrontal[PERSON_DETECTION_CAP * 2];
    
    RawToFP(
        allPersonIndices,
        nbPersons1,
        &personDetectionFrontalConfig1,
        personFrontal );
    
    RawToFP(
        allPersonIndices,
        nbPersons1,
        &personDetectionNonFrontalConfig1,
        personNonFrontal );
    
    for( size_t i = 0; i < nbPersons1; ++i )
    {
        allConfidencesRaw[i] = 
            personDetectionConfidenceConfig1.dataPtr[allPersonIndices[i]];
        allIsFrontal[i] = FPGt( personFrontal[i], personNonFrontal[i] );
    }
    
    // Postprocess PD, 16x9
    nbPersons2 = PostprocessAnchorBasedDetection(
        PERSON_DETECTION_NETWORK_GRID_DIM_16x9.width *
            PERSON_DETECTION_NETWORK_GRID_DIM_16x9.height,
        &personDetectionConfidenceConfig2,
        &personDetectionBoundingBoxesConfig2,
        PERSON_DETECTION_CAP,
        PERSON_DETECTION_CONFIDENCE_THRESHOLD,
        CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS ), // skip NMS for now
        allPersonIndices,
        &allConfidences[nbPersons1], // continue writing from nbPersons1
        &allBoxes[nbPersons1] );
    
    RawToFP(
        allPersonIndices,
        nbPersons2,
        &personDetectionFrontalConfig2,
        personFrontal + nbPersons1 );
    
    RawToFP(
        allPersonIndices,
        nbPersons2,
        &personDetectionNonFrontalConfig2,
        personNonFrontal + nbPersons1 );
    
    for( size_t i = 0; i < nbPersons2; ++i )
    {
        // continue writing from nbPersons1
        allConfidencesRaw[nbPersons1 + i] = 
            personDetectionConfidenceConfig2.dataPtr[allPersonIndices[i]];
        allIsFrontal[nbPersons1 + i] = FPGt(personFrontal[i + nbPersons1], personNonFrontal[i + nbPersons1]);
    }
    
    // sum results from both outputs
    nbPersonsTotal = nbPersons1 + nbPersons2;
    
    // reset all indices for 2nd pass of filtering on both outputs
    for( size_t i = 0; i < nbPersonsTotal; ++i )
    {
        allPersonIndices[i] = i;
    }
    
    // perform QuickSelect on both outputs
    QuickSelect( allPersonIndices, nbPersonsTotal, allConfidencesRaw, PERSON_DETECTION_CAP );
    nbPersonsTotal = nbPersonsTotal > PERSON_DETECTION_CAP ? PERSON_DETECTION_CAP : nbPersonsTotal;

    // copy detections results to output params
    for( size_t i = 0; i < nbPersonsTotal; ++i )
    {
        confidence[i] = allConfidences[allPersonIndices[i]];
        boxes[i] = allBoxes[allPersonIndices[i]];
    }
    
    // perform NMS on both outputs
    nbPersonsTotal = FilterOutBelowIoUThreshold(
        PERSON_DETECTION_NMS_THRESHOLD, 
        nbPersonsTotal,
        allPersonIndices,
        confidence,
        boxes );        
    
    // copy frontal/non-frontal results
    for( size_t i = 0; i < nbPersonsTotal; ++i )
    {
        isFrontal[i] = allIsFrontal[allPersonIndices[i]];
        isFrontalConfidence[i] = personFrontal[allPersonIndices[i]];
        isNotFrontalConfidence[i] = personNonFrontal[allPersonIndices[i]];
    }
    
    // Change boxes coordinates to source image coordinate system
    for (size_t i = 0; i < nbPersonsTotal; ++i)
    {
        fp_t left = FPMap(
            boxes[i].left,
            &personDetectionCoordinateXRange, &sourceCoordinateXRange);
        fp_t right = FPMap(
            boxes[i].right,
            &personDetectionCoordinateXRange, &sourceCoordinateXRange);
        fp_t top = FPMap(
            boxes[i].top,
            &personDetectionCoordinateYRange, &sourceCoordinateYRange);
        fp_t bottom = FPMap(
            boxes[i].bottom,
            &personDetectionCoordinateYRange, &sourceCoordinateYRange);
        boxes[i] = CreateGeometricBox( left, top, right, bottom );
    }
    
    return nbPersonsTotal;
}