/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "object_detection.h"

#include "preprocessing/bounding_box.h"
#include "postprocessing/postprocessing_filters.h"
#include "postprocessing/postprocessing_anchor_based_detection.h"
#include "utils.h"

const int32_t ML_ENGINE_OUTPUT_FRAC_BITS = 10;

const int32_dim_t OBJECT_DETECTION_NETWORK_INPUT_DIM =
    CreateLiteralInt32Dim( 384, 288 );

const int32_dim_t OBJECT_DETECTION_NETWORK_GRID_DIM[] = {
    CreateLiteralInt32Dim( 12, 9 ),
    CreateLiteralInt32Dim( 24, 18 ),
    CreateLiteralInt32Dim( 48, 36 ) };

anchor_t ObjectDetectionGridCoordinateToAnchor(
    const anchor_grid_coords_t *coords,
    const int32_dim_t *gridDim )
{
    // Transform relative results from the NN to absolute values coordinates in
    // the input image
    fp_t strideX =
        CreateFPInt( OBJECT_DETECTION_NETWORK_INPUT_DIM.width /
            gridDim->width, ML_ENGINE_OUTPUT_FRAC_BITS );
    fp_t strideY =
        CreateFPInt( OBJECT_DETECTION_NETWORK_INPUT_DIM.height /
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

anchor_t ObjectDetectionGridCoordinateToAnchor_64x36( const anchor_grid_coords_t *coords )
{
    return ObjectDetectionGridCoordinateToAnchor( coords,
        &OBJECT_DETECTION_NETWORK_GRID_DIM[2]);
}

anchor_t ObjectDetectionGridCoordinateToAnchor_32x18( const anchor_grid_coords_t *coords )
{
    return ObjectDetectionGridCoordinateToAnchor( coords,
        &OBJECT_DETECTION_NETWORK_GRID_DIM[1]);
}

anchor_t ObjectDetectionGridCoordinateToAnchor_16x9( const anchor_grid_coords_t *coords )
{
    return ObjectDetectionGridCoordinateToAnchor( coords,
        &OBJECT_DETECTION_NETWORK_GRID_DIM[0]);
}

geometric_box_t ObjectDetectionRawToBoundingBox(
    fp_t rawDeltaLeft, fp_t rawDeltaTop, fp_t rawDeltaRight, fp_t rawDeltaBottom,
    const anchor_t *anchor )
{
    int32_t fracBits = rawDeltaLeft.fracBits;
    fp_t deltaScale =
        CreateFPInt( OBJECT_DETECTION_NETWORK_INPUT_DIM.width / 32, fracBits );

    // Compute the box top left and bottom right coordinates from center
    fp_t left = FPSub( anchor->center.x, FPMul(rawDeltaLeft, deltaScale) );
    fp_t right = FPAdd( anchor->center.x, FPMul(rawDeltaRight, deltaScale) );

    if( FPGt( left, right  ) )
    {
        fp_t tmp = right;
        right = left;
        left = tmp;
    }

    fp_t top = FPSub( anchor->center.y, FPMul(rawDeltaTop, deltaScale) );
    fp_t bottom = FPAdd( anchor->center.y, FPMul(rawDeltaBottom, deltaScale) );

    if( FPGt( top, bottom  ) )
    {
        fp_t tmp = bottom;
        bottom = top;
        top = tmp;
    }

    return CreateGeometricBox( left, top, right, bottom );
}

int32_t PostprocessObjectDetectionOutput(
    uint32_t networkOutputCoordsAddr,
    uint32_t networkOutputConfidenceAddr,
    int32_dim_t gridDim,
    anchor_t (*gridCoordinatesToAnchor)( const anchor_grid_coords_t * ),
    int32_t maxBoxes,
    fp_t confidenceThreshold,
    size_t *indices,
    fp_t *confidences,
    geometric_box_t *boxes)
{
    size_t offset = gridDim.width * gridDim.height;
    const int16_t *confidencePtr = &(((int16_t *) networkOutputConfidenceAddr)[offset * 0]);

    const int16_t *deltaXPtr = &(((int16_t *) networkOutputCoordsAddr)[offset * 0]);
    const int16_t *deltaYPtr = &(((int16_t *) networkOutputCoordsAddr)[offset * 1]);
    const int16_t *deltaWPtr = &(((int16_t *) networkOutputCoordsAddr)[offset * 2]);
    const int16_t *deltaHPtr = &(((int16_t *) networkOutputCoordsAddr)[offset * 3]);

    fp_postprocessing_config_t objectDetectionConfidenceConfig =
        CreateFPPostprocessingConfig(
            confidencePtr,
            ML_ENGINE_OUTPUT_FRAC_BITS,
            FPSigmoid );

    bounding_boxes_postprocessing_config_t objectDetectionBoundingBoxesConfig = {
        .deltaXPtr = deltaXPtr,
        .deltaYPtr = deltaYPtr,
        .deltaWPtr = deltaWPtr,
        .deltaHPtr = deltaHPtr,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .gridDim = gridDim,
        .gridCoordinatesToAnchor = gridCoordinatesToAnchor,
        .rawToBoundingBox = ObjectDetectionRawToBoundingBox
    };

    int32_t nbPersons = PostprocessAnchorBasedDetection(
            offset,
            &objectDetectionConfidenceConfig,
            &objectDetectionBoundingBoxesConfig,
            maxBoxes,
            confidenceThreshold,
            CreateFPInt( 1, ML_ENGINE_OUTPUT_FRAC_BITS ), // skip NMS for now
            indices,
            confidences,
            boxes );

    return nbPersons;
}

ObjectClass GetClass(
    uint32_t networkOutputConfidenceAddr,
    int32_dim_t gridDim,
    int32_t classesNb,
    size_t index,
    fp_t *confidence
)
{
    size_t offset = gridDim.width * gridDim.height;
    int16_t *classScoresData = &(((int16_t *) networkOutputConfidenceAddr)[offset]);

    fp_t sum = CreateFPInt( 0, ML_ENGINE_OUTPUT_FRAC_BITS );
    fp_t maxScore = CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS );
    int32_t maxIndex = 0;

    for( int32_t i = 0; i < classesNb; ++i)
    {
        fp_t logit = InterpretIntAsFP( classScoresData[offset * i + index], ML_ENGINE_OUTPUT_FRAC_BITS );
        fp_t score = FPSigmoid( logit );
        sum = FPAdd( sum, score );

        if( FPGt( score, maxScore ) )
        {
            maxScore = score;
            maxIndex = i;
        }
    }

    *confidence = FPDiv(maxScore, sum);
    return maxIndex;
}

int32_t PostprocessObjectDetection(
    struct ObjectDetectionPostprocessingConfig *config,
    struct ObjectDetectionOutput *results )
{
    anchor_t (*ObjectDetectionGridCoordinateToAnchor[])( const anchor_grid_coords_t *) = {
        ObjectDetectionGridCoordinateToAnchor_16x9,
        ObjectDetectionGridCoordinateToAnchor_32x18,
        ObjectDetectionGridCoordinateToAnchor_64x36
    };

    size_t indices[config->maxBoxes];

    int32_t totalBoxes = 0;
    int32_t remainingBoxes = config->maxBoxes;

    for( size_t j = 0; j < RESOLUTION_LAYERS_NB; ++j )
    {
        int32_t nbOutputs =
            OBJECT_DETECTION_NETWORK_GRID_DIM[j].height *
            OBJECT_DETECTION_NETWORK_GRID_DIM[j].width;

        int32_t maxBoxes =
            nbOutputs < remainingBoxes / (RESOLUTION_LAYERS_NB - j)
            ? nbOutputs
            : remainingBoxes / (RESOLUTION_LAYERS_NB - j);

        int32_t nbBoxes = PostprocessObjectDetectionOutput(
            RAW_DATA_OUTPUT_COORDS_ADDRESSES[j],
            RAW_DATA_OUTPUT_CONFIDENCE_ADDRESSES[j],
            OBJECT_DETECTION_NETWORK_GRID_DIM[j],
            ObjectDetectionGridCoordinateToAnchor[j],
            maxBoxes,
            config->confidenceThreshold,
            &indices[totalBoxes], &results->confidences[totalBoxes],
            &results->boxes[totalBoxes]);

        remainingBoxes -= nbBoxes;

        for( size_t i = 0; i < nbBoxes; ++i)
        {
            fp_t confidence;
            (&results->classes[totalBoxes])[i] = (int32_t) GetClass(
                RAW_DATA_OUTPUT_CONFIDENCE_ADDRESSES[j],
                OBJECT_DETECTION_NETWORK_GRID_DIM[j],
                OBJECT_DETECTION_CLASSES_NB,
                (&indices[totalBoxes])[i],
                &confidence);
        }

        totalBoxes += nbBoxes;
    }

    int32_t nbBoxes = FilterOutSameClassBelowIoUThreshold (
        config->IoUThreshold,  // Threshold value
        totalBoxes,     // Size of the input lists
        indices,
        results->confidences,
        results->boxes,
        (int32_t *) results->classes );

    return nbBoxes;
}
