#include "box.h"
#include "range.h"
#include "anchor.h"
#include "frame_data.h"
#include "postprocessing_fixed_point.h"
#include "postprocessing_bounding_box.h"
#include "postprocessing_geometric_point.h"
#include "hmi_face_detection.h"
#include "postprocessing_anchor_based_detection.h"
#include "source_image.h"
#include "ml_engine_config.h"
#include "types.h"


#define LiteralArray( ... ) __VA_ARGS__

const int32_dim_t FACE_DETECTION_NETWORK_INPUT_DIM =
    CreateLiteralInt32Dim(256, 144);
const int32_dim_t FACE_DETECTION_NETWORK_GRID_DIM =
    CreateLiteralInt32Dim(16, 9);
const fp_range_t FACE_DETECTION_NETWORK_CONFIDENCE_RAW_RANGE =
    CreateLiteralFPRange(
        CreateLiteralFPInt(-32, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateLiteralFPInt(32, ML_ENGINE_OUTPUT_FRAC_BITS));

const fp_range_t FACE_DETECTION_NETWORK_DELTA_RAW_RANGE =
    CreateLiteralFPRange(
        CreateLiteralFPInt(-32, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateLiteralFPInt(32, ML_ENGINE_OUTPUT_FRAC_BITS));

const fp_range_t FACE_DETECTION_NETWORK_DELTA_VALUE_RANGE =
    CreateLiteralFPRange(
        CreateLiteralFPInt(-32, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateLiteralFPInt(32, ML_ENGINE_OUTPUT_FRAC_BITS));

fp_dim_t FACE_DETECTION_NETWORK_ANCHORS_DIM[] = LiteralArray({CreateLiteralFPDim(
                                                                  CreateLiteralFPInt(80, ML_ENGINE_OUTPUT_FRAC_BITS),
                                                                  CreateLiteralFPInt(80, ML_ENGINE_OUTPUT_FRAC_BITS)),
                                                              CreateLiteralFPDim(
                                                                  CreateLiteralFPInt(30, ML_ENGINE_OUTPUT_FRAC_BITS),
                                                                  CreateLiteralFPInt(30, ML_ENGINE_OUTPUT_FRAC_BITS))});

const fp_t FACE_DETECTION_CONFIDENCE_THRESHOLD =
    CreateLiteralFP(45, 100, ML_ENGINE_OUTPUT_FRAC_BITS);

const fp_t FACE_DETECTION_NMS_THRESHOLD =
    CreateLiteralFP(35, 100, ML_ENGINE_OUTPUT_FRAC_BITS);
	anchor_t FaceDetectionGridCoordinatesToAnchor(const anchor_grid_coords_t *coords)
	{
		fp_t anchorX = CreateFP(
			(coords->col + 1) * FACE_DETECTION_NETWORK_INPUT_DIM.width,
			FACE_DETECTION_NETWORK_GRID_DIM.width + 1,
			ML_ENGINE_OUTPUT_FRAC_BITS);
		fp_t anchorY = CreateFP(
			(coords->row + 1) * FACE_DETECTION_NETWORK_INPUT_DIM.height,
			FACE_DETECTION_NETWORK_GRID_DIM.height + 1,
			ML_ENGINE_OUTPUT_FRAC_BITS);

		anchor_t anchor = {
			.dimensions = FACE_DETECTION_NETWORK_ANCHORS_DIM[coords->anchor],
			.center = CreateGeometricPoint(anchorX, anchorY)};

		return anchor;
	}

	geometric_box_t FaceDetectionRawToBoundingBox(
		fp_t rawDeltaX, fp_t rawDeltaY, fp_t rawDeltaW, fp_t rawDeltaH,
		const anchor_t *anchor)
	{
		fp_t centerX = FPAdd(anchor->center.x,
							 FPMul(anchor->dimensions.width, rawDeltaX));
		fp_t centerY = FPAdd(anchor->center.y,
							 FPMul(anchor->dimensions.height, rawDeltaY));
		fp_t width = FPMul(anchor->dimensions.width, rawDeltaW);
		fp_t height = FPMul(anchor->dimensions.height, rawDeltaH);

		return CreateGeometricBox(
			FPSub(centerX, FPRShift(width, 1)),
			FPSub(centerY, FPRShift(height, 1)),
			FPAdd(centerX, FPRShift(width, 1)),
			FPAdd(centerY, FPRShift(height, 1)));
	}

	geometric_point_2d_t FaceDetectionRawToLandmarks(
		fp_t rawX, fp_t rawY, const anchor_t *anchor )
	{
		fp_t centerX =
			FPAdd( anchor->center.x, FPMul( anchor->dimensions.width, rawX ) );
		fp_t centerY =
			FPAdd( anchor->center.y, FPMul( anchor->dimensions.height, rawY ) );

		return CreateGeometricPoint( centerX, centerY );
	}


int32_t FaceDetection(
	uint32_t FACE_DETECTION_NETWORK_OUTPUT_ADDR,
    fp_t *confidence,
    geometric_box_t *boxes,
	frame_data_t *frameData)
{
	const int32_dim_t FACE_DETECTION_NETWORK_INPUT_DIM =
		CreateLiteralInt32Dim(256, 144);
	const int32_dim_t FACE_DETECTION_NETWORK_GRID_DIM =
		CreateLiteralInt32Dim(16, 9);
	const int32_t FACE_DETECTION_NETWORK_ANCHORS_PER_CELL = 2;
	const int32_t FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET =
		FACE_DETECTION_NETWORK_GRID_DIM.width *
		FACE_DETECTION_NETWORK_GRID_DIM.height *
		FACE_DETECTION_NETWORK_ANCHORS_PER_CELL;
	const int16_t *const FACE_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 0;
    const int16_t *const FACE_DETECTION_NETWORK_DELTA_X_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 1;
    const int16_t *const FACE_DETECTION_NETWORK_DELTA_Y_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 2;
    const int16_t *const FACE_DETECTION_NETWORK_DELTA_W_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 3;
    const int16_t *const FACE_DETECTION_NETWORK_DELTA_H_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 4;
    const int16_t *const FACE_DETECTION_NETWORK_LDK_X_OUTPUT_ADDR[5] = {
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 5,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 7,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 9,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 11,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 13 };
    const int16_t *const FACE_DETECTION_NETWORK_LDK_Y_OUTPUT_ADDR[5] = {
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 6,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 8,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 10,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 12,
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 14 };
    const int16_t *const FACE_DETECTION_NETWORK_PITCH_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 15;
    const int16_t *const FACE_DETECTION_NETWORK_YAW_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 16;
    const int16_t *const FACE_DETECTION_NETWORK_ROLL_OUTPUT_ADDR =
        (int16_t *)FACE_DETECTION_NETWORK_OUTPUT_ADDR +
        FACE_DETECTION_NETWORK_OUTPUT_CHANNEL_OFFSET * 17;


// #ifndef N2STEP_SCALER
//     scaler_source_image_t faceDetectionSource = CreateScalerSourceImage(
//         0, REGULAR_FRAME_BUFFER_SOURCE_IMAGE_DIM,
//         0, REGULAR_FRAME_BUFFER_SOURCE_IMAGE_ROI, false);
// #else
//     scaler_source_image_t faceDetectionSource = CreateScalerSourceImage(
//         0, SOURCE_IMAGE_DIM,
//         0, SOURCE_IMAGE_ROI, false);
// #endif

    // pixel_box_t faceDetectionRoI = CreatePixelBox(
    //     0, 0,
    //     SOURCE_IMAGE_ROI.dimensions.width,
    //     SOURCE_IMAGE_ROI.dimensions.height);

    // scaler_output_image_t faceDetectionOutput = CreateScalerOutputImage(
    //     (int32_t)FACE_DETECTION_NETWORK_INPUT_ADDR,
    //     FACE_DETECTION_NETWORK_INPUT_DIM);

    // scaler_config_t faceDetectionScalerConfig = CreateScalerConfig(
    //     faceDetectionSource, faceDetectionRoI, false, faceDetectionOutput);

    fp_postprocessing_config_t faceDetectionConfidenceConfig =
        CreateFPPostprocessingConfig(
            FACE_DETECTION_NETWORK_CONFIDENCE_OUTPUT_ADDR,
            ML_ENGINE_OUTPUT_FRAC_BITS,
            FPSigmoid);

    bounding_boxes_postprocessing_config_t faceDetectionBoundingBoxesConfig = {
        .deltaXPtr = FACE_DETECTION_NETWORK_DELTA_X_OUTPUT_ADDR,
        .deltaYPtr = FACE_DETECTION_NETWORK_DELTA_Y_OUTPUT_ADDR,
        .deltaWPtr = FACE_DETECTION_NETWORK_DELTA_W_OUTPUT_ADDR,
        .deltaHPtr = FACE_DETECTION_NETWORK_DELTA_H_OUTPUT_ADDR,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .gridDim = FACE_DETECTION_NETWORK_GRID_DIM,
        .gridCoordinatesToAnchor = FaceDetectionGridCoordinatesToAnchor,
        .rawToBoundingBox = FaceDetectionRawToBoundingBox};

    fp_range_t faceDetectionCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(FACE_DETECTION_NETWORK_INPUT_DIM.width, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t faceDetectionCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(FACE_DETECTION_NETWORK_INPUT_DIM.height, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_WIDTH, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_HEIGHT, ML_ENGINE_OUTPUT_FRAC_BITS));

    size_t nbFaces = 0;
    size_t faceIndices[FACE_DETECTION_CAP];
    geometric_point_2d_t faceLandmarks[5][FACE_DETECTION_CAP];
    fp_t facePitch[FACE_DETECTION_CAP];
    fp_t faceYaw[FACE_DETECTION_CAP];
    fp_t faceRoll[FACE_DETECTION_CAP];

    // CropAndResizeImage(&faceDetectionScalerConfig, true);
    // if ( !WaitForInterrupt(INT_SCALER, 500) )
    // {
    //     return 0;
    // }

    // RunMLEngine(
    //     FACE_DETECTION_NETWORK_FIRMWARE_ADDR,
    //     FACE_DETECTION_NETWORK_INPUT_ADDR,
    //     FACE_DETECTION_NETWORK_INPUT_DIM.width *
    //     FACE_DETECTION_NETWORK_INPUT_DIM.height,
    //     true );


    // // Wait for ML engine

    // if( !WaitForInterrupt(INT_ML_ENGINE, 500) )
    // {
    //     return 0;
    // }
// int i = 80*80/2;

     // Postprocess FD
    nbFaces = PostprocessAnchorBasedDetection(
        FACE_DETECTION_NETWORK_ANCHORS_PER_CELL *
            FACE_DETECTION_NETWORK_GRID_DIM.width *
            FACE_DETECTION_NETWORK_GRID_DIM.height,
        &faceDetectionConfidenceConfig,
        &faceDetectionBoundingBoxesConfig,
        FACE_DETECTION_CAP,
        FACE_DETECTION_CONFIDENCE_THRESHOLD,
        FACE_DETECTION_NMS_THRESHOLD,
        faceIndices,
        confidence,
        boxes );

    for( size_t i = 0; i < 5; ++i )
    {
        anchor_geometric_point_2d_postprocessing_config_t
        faceDetectionLandmarksConfig = {
            .xPtr = FACE_DETECTION_NETWORK_LDK_X_OUTPUT_ADDR[i],
            .yPtr = FACE_DETECTION_NETWORK_LDK_Y_OUTPUT_ADDR[i],
            .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
            .gridDim = FACE_DETECTION_NETWORK_GRID_DIM,
            .gridCoordinatesToAnchor = FaceDetectionGridCoordinatesToAnchor,
            .rawToGeometricPoint = FaceDetectionRawToLandmarks
        };

        RawToAnchorBasedGeometricPoint2D(
            faceIndices,
            nbFaces,
            &faceDetectionLandmarksConfig,
            faceLandmarks[i] );
    }

    fp_postprocessing_config_t faceDetectionPitchConfig = {
        .dataPtr = FACE_DETECTION_NETWORK_PITCH_OUTPUT_ADDR,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = RadiansToDegrees
    };

    RawToFP( faceIndices, nbFaces, &faceDetectionPitchConfig, facePitch );

    fp_postprocessing_config_t faceDetectionYawConfig = {
        .dataPtr = FACE_DETECTION_NETWORK_YAW_OUTPUT_ADDR,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = RadiansToDegrees
    };

    RawToFP( faceIndices, nbFaces, &faceDetectionYawConfig, faceYaw );

    fp_postprocessing_config_t faceDetectionRollConfig = {
        .dataPtr = FACE_DETECTION_NETWORK_ROLL_OUTPUT_ADDR,
        .fracBits = ML_ENGINE_OUTPUT_FRAC_BITS,
        .rawToFP = RadiansToDegrees
    };

    RawToFP( faceIndices, nbFaces, &faceDetectionRollConfig, faceRoll );

    // Change boxes coordinates to source image coordinate system
    for (int32_t i = 0; i < (int32_t) nbFaces; ++i)
    {
        fp_t left = FPMap(
            boxes[i].left,
            &faceDetectionCoordinateXRange, &sourceCoordinateXRange );
        fp_t right = FPMap(
            boxes[i].right,
            &faceDetectionCoordinateXRange, &sourceCoordinateXRange );
        fp_t top = FPMap(
            boxes[i].top,
            &faceDetectionCoordinateYRange, &sourceCoordinateYRange );
         fp_t bottom = FPMap(
            boxes[i].bottom,
            &faceDetectionCoordinateYRange, &sourceCoordinateYRange );
        boxes[i] = CreateGeometricBox( left, top, right, bottom );

        for( int32_t j = 0; j < 5; ++j )
        {
            fp_t x = FPMap(
                faceLandmarks[j][i].x,
                &faceDetectionCoordinateXRange, &sourceCoordinateXRange );
            fp_t y = FPMap(
                faceLandmarks[j][i].y,
                &faceDetectionCoordinateYRange, &sourceCoordinateYRange );
            faceLandmarks[j][i] = CreateGeometricPoint( x, y );
            SetLandmark2dFaceDet(
                &frameData->detectedUsers[i].landmarks, j, faceLandmarks[j][i] );
        }

        frameData->detectedUsers[i].roiPtr = &boxes[i];
        frameData->detectedUsers[i].eulerAnglesICS.pitch = facePitch[i];
        frameData->detectedUsers[i].eulerAnglesICS.yaw = faceYaw[i];
        frameData->detectedUsers[i].eulerAnglesICS.roll = faceRoll[i];
    }
    return nbFaces;
}
