/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "assert.h"
#include "ml_ops.h"
#include "hw_regs.h"
#include "fw_core.h"
#include "ospi_support.h"
#include "app_module.h"
#include "utils.h"
#include "memmap.h"
#include "camera_config.h"
#include "fixed_point.h"
#include "object_detection.h"
#include "iface_support.h"
#include "range.h"

// Macro magic to simplify buffer allocation
#define SEND_DATA(data) memcpy(&ctxt->outputI2C[index], (uint8_t*)&data, sizeof(data)); index += sizeof(data);

// TODO: This needs to NOT be hardcoded
const int32_dim_t NETWORK_INPUT_DIM =
    CreateLiteralInt32Dim( 384, 288 );

const pixel_box_t SOURCE_IMAGE_ROI =
    CreateLiteralPixelBox( 0, 0, 3200, 2400 );



#define ML_ENGINE_OUTPUT_FRAC_BITS 10

#define OBJECT_DETECTION_CAP 500
const fp_t OBJECT_DETECTION_CONFIDENCE_THRESHOLD = FloatToFP( 0.6, 10 );
const fp_t OBJECT_DETECTION_IOU_THRESHOLD = FloatToFP( 0.4, 10 );


/**
 * enum ml_done_states captures the possible states for the ML done state
 * machine code to track. Based on the states different actions will be executed
 * by the App Module.
 */
enum ml_done_states {
	ML_DONE_FIRST_NETWORK_COMPLETE
} state;

struct network_info networks_list[] = {{
	.network = ML_APP_1_NETWORK_ID, /* First network handle Object detection */
	.inout_offset = ML_APP_1_INPUT_START_ADDRESS,
	.inout_size   = ML_APP_1_INOUT_SIZE,
}};

struct networks     app_networks    = {
		   .count_of_networks = GET_ARRAY_COUNT(networks_list),
		   .networks          = networks_list,
};

/**
 * struct  app_module_context is a sample structure that shows what could be
 * used to maintain the App Module's context. The App Module can define its own
 * context structure and use it to store any data that is needed across the
 * App Module's functions.
 */
struct app_module_context {
	enum ml_done_states state_var;
	struct networks    *p_networks;

    struct ObjectDetectionPostprocessingConfig config;
    fp_t confidences[OBJECT_DETECTION_CAP];
    geometric_box_t boxes[OBJECT_DETECTION_CAP];
    ObjectClass classes[OBJECT_DETECTION_CAP];
    struct ObjectDetectionOutput results;
	unsigned char outputI2C[0x22 + 0x10 * OBJECT_DETECTION_CAP + 0x01];
	uint8_t streamComplete;
	uint32_t skipOutputNb;
};

/* app_ctxt is the variable holding App Module Context contents. */
struct app_module_context app_ctxt = {
	.state_var  = ML_DONE_FIRST_NETWORK_COMPLETE,
	.p_networks = &app_networks,
};

/**
 * app_preinit() is called by the FW Core before it has initialized all of its
 * data structures and hardware blocks.
 * This is an optional function for App Module to implement.
 *
 * @return pointer to the App Module's context.
 */
app_handle_t app_preinit(void)
{
	/**
	 * The App Module can perform any pre-initialization tasks here.
	 * For now, we just return pointer to our internal app_ctxt as we will be
	 * needing it later.
	 */

	return (app_handle_t)&app_ctxt;
}

/**
 * app_init() is called by the FW Core to initialize the App Module. Before this
 * function is called the FW Core has already initialized the hardware blocks
 * and its internal structures. All the remaining initialization needed by the
 * App Module should be done here.
 *
 * @param app_context is the handle returned by app_preinit() or NULL.
 *
 * @return pointer to the App Module's context.
 */
app_handle_t app_init(app_handle_t app_context)
{
	struct app_module_context *ctxt = (struct app_module_context *)app_context;

	/**
	 * The App Module can perform any initialization tasks here.
	 * For now, we just return the app_context as is.
	 */

	GARD__DBG_ASSERT((NULL == app_context) || (ctxt == &app_ctxt),
					 "Invalid app_context");

	/**
	 * This is the right time to register the ML networks with FW Core.
	 */
	register_networks(ctxt->p_networks);

	ctxt->config.maxBoxes = OBJECT_DETECTION_CAP;
	ctxt->config.confidenceThreshold = OBJECT_DETECTION_CONFIDENCE_THRESHOLD;
	ctxt->config.IoUThreshold = OBJECT_DETECTION_IOU_THRESHOLD;

	ctxt->results.boxes = ctxt->boxes;
	ctxt->results.classes = ctxt->classes;
	ctxt->results.confidences = ctxt->confidences;

	ctxt->streamComplete = true;
	ctxt->skipOutputNb = 0;

	/**
	 * Set the first network to be run by ML engine when image capture is done.
	 * Alternatively, this call can also be done in app_preprocess().
	 */
	schedule_network_to_run(ctxt->p_networks->networks[0].network);

#ifdef START_CAMERA_STREAM_ON_BOOT
	(void)start_camera_streaming();
#endif

	/**
	 * Start capture of the first image from the camera. Subsequent image
	 * captures should happen ideally in the routines app_ml_done() or
	 * app_image_processing_done() depending on the stage of the pipeline when
	 * it is determined that the next image is needed to continue.
	 */
	capture_image_async();

	return app_context;
}

/**
 * app_preprocess() is called by the FW Core after the image capture is done,
 * App Module can provide this handler if he wishes to view the image and decide
 * the next course of action which possibly could be either the run the
 * ML engine on the image or drop the image and capture the next one.
 *
 *
 * @param app_context is the handle returned by app_init() or NULL.
 * @param image_data is a pointer to the captured image data. If the image is
 *                   placed in the ML engine buffers then this varaiable will be
 *                   NULL.
 *
 * @return APP_CODE__SUCCESS if preprocessing was successful, or an error code
 *         otherwise.
 */
enum app_ret_code app_preprocess(app_handle_t app_context, void *image_data)
{
	GARD__DBG_ASSERT((app_context == (app_handle_t)&app_ctxt),
					 "Invalid app_context");

	/**
	 * The App Module can perform any preprocessing on the captured image here.
	 * He can also schedule the ML engine to be run on the image data or go back
	 * to capturing the next image and skip this one.
	 *
	 * In this example, we will start the next image capture as the scaler
	 * buffers are free, we will also start the ML engine.
	 */
	start_ml_engine();
	return APP_CODE__SUCCESS;
}

/**
 * app_ml_done() is called by the FW Core when the ML engine has finished
 * processing the image data. This call will be called to process the results of
 * each network that was run on the ML engine.
 *
 * @param app_context is the handle returned by app_init() or NULL.
 * @param ml_results is a pointer to the results from the ML engine.
 *
 * @return APP_CODE__SUCCESS if processing was successful, or an error code
 *         otherwise.
 */
enum app_ret_code app_ml_done(app_handle_t app_context, void *ml_results)
{
	struct app_module_context *ctxt = (struct app_module_context *)app_context;

	GARD__DBG_ASSERT((app_context == (app_handle_t)&app_ctxt) &&
						 (NULL != ml_results),
					 "Invalid app_context or image_data");

   	fp_range_t endUserCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_ROI.dimensions.width, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t endUserCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(SOURCE_IMAGE_ROI.dimensions.height, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateXRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(NETWORK_INPUT_DIM.width, ML_ENGINE_OUTPUT_FRAC_BITS));

    fp_range_t sourceCoordinateYRange = CreateFPRange(
        CreateFPInt(0, ML_ENGINE_OUTPUT_FRAC_BITS),
        CreateFPInt(NETWORK_INPUT_DIM.height, ML_ENGINE_OUTPUT_FRAC_BITS));

	/**
	 * The App Module can process the results from the ML engine here. This
	 * processing happens in a state machine to act properly on the results
	 * from the ML engine.
	 */

	uint32_t nbObjects = 0;
	nbObjects = nbObjects;
	switch (ctxt->state_var) {
	case ML_DONE_FIRST_NETWORK_COMPLETE:
		/**
		 * The first network has completed processing. Process the results and
		 * decide if we need to move to second network or drop the current frame
		 * and capture the next one.
		 *
		 * In this example, we will schedule the second network to run.
		 */
		nbObjects = PostprocessObjectDetection(
            &ctxt->config, &ctxt->results);

		/* Start image capture -> rescale -> ml sequence again */
		capture_image_async();

		struct ObjectDetectionData data;

		size_t index = 0;
		ctxt->outputI2C[index] = 0x7e; // start flag
		index++;

		int dataLengthIndex = index;
		index += sizeof(uint16_t);

		ctxt->outputI2C[index] = 1; // RT_DATA
		index++;
		ctxt->outputI2C[index] = 1; // RT_DATA version
		index++;

		SEND_DATA(SOURCE_IMAGE_ROI.dimensions.width);
		SEND_DATA(SOURCE_IMAGE_ROI.dimensions.height);
		SEND_DATA(SOURCE_IMAGE_ROI.left);
		SEND_DATA(SOURCE_IMAGE_ROI.top);
		SEND_DATA(SOURCE_IMAGE_ROI.right);
		SEND_DATA(SOURCE_IMAGE_ROI.bottom);

		uint32_t zero = 0; // reserved size
		SEND_DATA(zero);

		SEND_DATA(nbObjects);
		for( int32_t i = 0; i < nbObjects && i < 16; ++i )
		{
			data.objectClass = ctxt->results.classes[i];
			data.confidence = ctxt->results.confidences[i].n;
			data.left = FPRound( FPMap( ctxt->results.boxes[i].left, &sourceCoordinateXRange, &endUserCoordinateXRange ) );
			data.top = FPRound( FPMap( ctxt->results.boxes[i].top, &sourceCoordinateYRange, &endUserCoordinateYRange ) );
			data.right = FPRound( FPMap( ctxt->results.boxes[i].right, &sourceCoordinateXRange, &endUserCoordinateXRange ) );
			data.bottom = FPRound( FPMap( ctxt->results.boxes[i].bottom, &sourceCoordinateYRange, &endUserCoordinateYRange ) );

			SEND_DATA(data);
		}
		uint32_t dataLength = index-dataLengthIndex-2;
		memcpy( &ctxt->outputI2C[dataLengthIndex], &dataLength, sizeof(uint16_t));
		
		// Just testing streamComplete apparently block the output. I suspect this might
		// have to do with HUB not hooking to GARD before GARD send a "data available",
		// signal, leading to HUB waiting for GARD data and GARD waiting for HUB to read
		// data. By setting a maximum number of outputs to skip because streamComplete is
		// false, we make sure they don't get interblocked forever.
		// Another option would be to test elapsed time since last output, but we don't
		// have a time API available.
		if( ctxt->streamComplete || ctxt->skipOutputNb > 9)
		{
			ctxt->streamComplete = false;
			ctxt->skipOutputNb = 0;
			stream_data_to_host_async(ctxt->outputI2C, index, 10, &ctxt->streamComplete);
		}
		else
		{
			ctxt->skipOutputNb += 1;
		}

 		break;
	}

	return APP_CODE__SUCCESS;
}

/**
 * app_image_processing_done() is called by the FW Core when the image
 * processing is done and the App Module wants to do any sort of cleanup of the
 * resources or even send the results to HUB running on the Host.
 * This function is ideally scheduled by the App Module to be called after all
 * networks have been processed for a particular image.
 *
 * @param app_context is the handle returned by app_init() or NULL.
 *
 * @return APP_CODE__SUCCESS if processing was successful, or an error code
 *         otherwise.
 */
enum app_ret_code app_image_processing_done(app_handle_t app_context)
{
	GARD__DBG_ASSERT((app_context == (app_handle_t)&app_ctxt),
					 "Invalid app_context or image_data");

	/**
	 * At this point the most likely action for the App Module is to send the
	 * final results to the Host. If we are called here then that is what is the
	 * case.
	 *
	 * In this example we will send the results to the Host and also schedule
	 * the first network to run again on ML engine after image capture is done.
	 */
	schedule_network_to_run(app_ctxt.p_networks->networks[0].network);

	return APP_CODE__SUCCESS;
}

/**
 * app_rescale_done() is called by the FW Core when the image rescaling
 * operation has completed. This function is an opportunity for the App Module
 * to perform any final processing after the rescaling is done. This may not be
 * available in all the hardware platforms as this entails that the image is in
 * RAM and available to the firmware for further processing.
 *
 * @param app_context is the handle returned by app_init() or NULL.
 *
 * @return APP_CODE__SUCCESS if processing was successful, or an error code
 *         otherwise.
 */
enum app_ret_code app_rescale_done(app_handle_t app_context)
{
	GARD__DBG_ASSERT((app_context == (app_handle_t)&app_ctxt),
					 "Invalid app_context or image_data");

	/**
	 * The App Module can perform any final processing after the rescaling
	 * is done here.
	 * For now, we just return success as no processing is done.
	 */
	return APP_CODE__SUCCESS;
}

/**
 * This is where we let the FW Core know about the App Module's callbacks.
 */
DEFINE_APP_MODULE_CALLBACKS(app_preinit,
							app_init,
							app_preprocess,
							app_ml_done,
							app_image_processing_done,
							app_rescale_done);
