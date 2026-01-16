/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "camera_capture.h"
#include "camera_config.h"
#include "fw_core.h"
#include "fw_globals.h"
#include "hw_regs.h"
#include "gpio.h"
#include "gpio_support.h"
#include "app_module.h"
#include "ml_ops.h"
#include "pipeline_ops.h"

/**
 * TBD-SRP: Remove these values when they come from the camera configuration
 * blob.
 *
 */
#define CAMERA_CAPTURE_WIDTH  480
#define CAMERA_CAPTURE_HEIGHT 270
#define CAMERA_CROP_LEFT      0x0
#define CAMERA_CROP_RIGHT     479
#define CAMERA_CROP_UPPER     0
#define CAMERA_CROP_BOTTOM    269
#define CAMERA_OUTPUT_WIDTH   256
#define CAMERA_OUTPUT_HEIGHT  144

#ifdef ML_APP_MOD
/**
 * Box scaler configuration parameters for ML_APP_MOD
 * Input 3280 x 2464 at 30fps with 4 lanes.
 * Output 384 x 288.
 */
#define CAMERA_CAPTURE_PRECROP_LEFT   0
#define CAMERA_CAPTURE_PRECROP_RIGHT  820
#define CAMERA_CAPTURE_PRECROP_UPPER  0
#define CAMERA_CAPTURE_PRECROP_BOTTOM 1232
#define BOX_SCALER_FACTOR_Y           8
#define BOX_SCALER_FACTOR_X           2

/**
 * Bilinear scaler capture configuration parameters for ML_APP_MOD
 *
 */
#define BL_SCALER_CROP_LEFT           0
#define BL_SCALER_CROP_RIGHT          410
#define BL_SCALER_CROP_UPPER          0
#define BL_SCALER_CROP_BOTTOM         308
#define BL_SCALER_CROP_IN_HEIGHT      308
#define BL_SCALER_CROP_IN_WIDTH       410
#define BL_SCALER_CROP_IN_SIZE                                                 \
	(BL_SCALER_CROP_IN_HEIGHT * BL_SCALER_CROP_IN_WIDTH)
#define BL_SCALER_CROP_OUT_HEIGHT 288
#define BL_SCALER_CROP_OUT_WIDTH  384
#define BL_SCALER_CROP_OUT_SIZE                                                \
	(BL_SCALER_CROP_OUT_HEIGHT * BL_SCALER_CROP_OUT_WIDTH)

/**
 * Bilinear scaler rescale configuration parameters for ML_APP_MOD
 *
 */
#define BL_SCALER_RESCALE_CROP_LEFT      0
#define BL_SCALER_RESCALE_CROP_RIGHT     384
#define BL_SCALER_RESCALE_CROP_UPPER     0
#define BL_SCALER_RESCALE_CROP_BOTTOM    288
#define BL_SCALER_RESCALE_CROP_IN_HEIGHT 288
#define BL_SCALER_RESCALE_CROP_IN_WIDTH  384
#define BL_SCALER_RESCALE_CROP_IN_SIZE                                         \
	(BL_SCALER_RESCALE_CROP_IN_HEIGHT * BL_SCALER_RESCALE_CROP_IN_WIDTH)
#define BL_SCALER_RESCALE_CROP_OUT_HEIGHT 288
#define BL_SCALER_RESCALE_CROP_OUT_WIDTH  384
#define BL_SCALER_RESCALE_CROP_OUT_SIZE                                        \
	(BL_SCALER_RESCALE_CROP_OUT_HEIGHT * BL_SCALER_RESCALE_CROP_OUT_WIDTH)

#endif

/**
 * capture_image_async() is used by the App Module to start capturing the image
 * from the connected camera. The function assumes that the camera is already
 * initialized and ready to capture images
 * Note: Capturing an image involves:
 * 1) Capturing and scaling the feed.
 * 2) If ML engine is the destination for holding the image then we move the
 * image to the buffer within ML engine.
 *
 * Note: The routine only starts the camera capture process. The completely
 * captured image will be available after some time, hence the name of the
 * routine has '_async' postfixed to indicate this.
 *
 * @return Nothing
 */
void capture_image_async(void)
{
	if (!camera_started) {
		/* Nothing to do if camera is not connected. */
		return;
	}

	/**
	 * First we setup the buffers to capture the image.
	 * Next we start the camera capturing and rescaling process.
	 */
#ifdef ML_APP_HMI
	GARD__SETUP_SCALER_ENGINE_BUFFERS();
#elif defined(ML_APP_MOD)
	/* Capture Config : capture buffer address for ML_APP_MOD, rgbs capture
	 */
	setup_camera_capture_parameters();

	GARD__SET_CAPTURE_CONFIGS(ML_APP_1_PREINPUT_START_ADDRESS);
#endif

	/* GPIO Index 1 tracks Camera Capture events
	 * GPIO Index 2 tracks all events : Capture, Rescale, data movement to ML
	 * engine and ML engine start/stop
	 */
	gpio_pin_write(&gpio_0, GPIO_PIN_1, GPIO_OUTPUT_HIGH);

	gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_HIGH);

	/**
	 * Start Capture and rescaling process.
	 */
#ifdef ML_APP_HMI
	GARD__START_SCALER_ENGINE();

#ifdef NO_RESCALE_DONE_ISR
	rescaling_started = true;
#endif

#elif defined(ML_APP_MOD)
	GARD__START_CAPTURE_STAGE();

	capture_started = true;
#endif
}

/**
 * setup_camera_capture_parameters() sets up the camera capture parameters such
 * as input image dimensions, crop dimensions, and output image dimensions.
 * This function is called once during the initialization of the firmware.
 *
 * @return Nothing
 */
void setup_camera_capture_parameters(void)
{
	/**
	 * This function sets up the camera capture parameters such as input
	 * image dimensions, crop dimensions, and output image dimensions.
	 */
#ifdef ML_APP_HMI
	GARD__SET_INPUT_IMG_DIMENSIONS(CAMERA_CAPTURE_WIDTH, CAMERA_CAPTURE_HEIGHT);
	GARD__SET_CROP_DIMENSIONS(CAMERA_CROP_LEFT, CAMERA_CROP_RIGHT,
							  CAMERA_CROP_UPPER, CAMERA_CROP_BOTTOM);
	GARD__SET_OUTPUT_IMG_DIMENSIONS(CAMERA_OUTPUT_WIDTH, CAMERA_OUTPUT_HEIGHT);

#elif defined(ML_APP_MOD)
	/* Clear capture and rescale stage done status bits
	 */
	GARD__STOP_CAPTURE_STAGE();
	GARD__STOP_RESCALE_STAGE();

	/* Box Scaler Config : input image 960x1080, crop 0,960,0,1080
	 * output 480*270 grayscale
	 */
	GARD__SET_BOX_SCALER_CONFIGS(
		CAMERA_CAPTURE_PRECROP_LEFT, CAMERA_CAPTURE_PRECROP_RIGHT,
		CAMERA_CAPTURE_PRECROP_UPPER, CAMERA_CAPTURE_PRECROP_BOTTOM,
		BOX_SCALER_FACTOR_Y, BOX_SCALER_FACTOR_X);

	/* Bilinear Scaler Config : input image 480x270, scaled output 256*144
	 */
	GARD__SET_BILINEAR_SCALER_CONFIGS(
		BL_SCALER_CROP_LEFT, BL_SCALER_CROP_RIGHT, BL_SCALER_CROP_UPPER,
		BL_SCALER_CROP_BOTTOM, BL_SCALER_CROP_IN_HEIGHT,
		BL_SCALER_CROP_IN_WIDTH, BL_SCALER_CROP_IN_SIZE,
		BL_SCALER_CROP_OUT_HEIGHT, BL_SCALER_CROP_OUT_WIDTH,
		BL_SCALER_CROP_OUT_SIZE);

#endif
}

/**
 * setup_image_rescale_parameters() sets up the image rescale parameters such
 * as input image dimensions, crop dimensions, and output image dimensions.
 * This function is called once during the initialization of the firmware.
 *
 * @return Nothing
 */
void setup_image_rescale_parameters(void)
{
	/**
	 * This function sets up the image rescale parameters such as input
	 * image dimensions, crop dimensions, and output image dimensions.
	 */
#ifdef ML_APP_MOD
	/* Bilinear Scaler Config : input image 256x144, scaled output 256*144
	 */
	GARD__SET_BILINEAR_SCALER_CONFIGS(
		BL_SCALER_RESCALE_CROP_LEFT, BL_SCALER_RESCALE_CROP_RIGHT,
		BL_SCALER_RESCALE_CROP_UPPER, BL_SCALER_RESCALE_CROP_BOTTOM,
		BL_SCALER_RESCALE_CROP_IN_HEIGHT, BL_SCALER_RESCALE_CROP_IN_WIDTH,
		BL_SCALER_RESCALE_CROP_IN_SIZE, BL_SCALER_RESCALE_CROP_OUT_HEIGHT,
		BL_SCALER_RESCALE_CROP_OUT_WIDTH, BL_SCALER_RESCALE_CROP_OUT_SIZE);

#endif
}

/**
 * capture_done_isr() is the ISR that is called when the 2 stage scaler engine
 * has completed the image capture process. This routine will start the rescale
 * stage in case of 2 stage scaler engine.
 *
 * @param ctxt is a pointer to the context passed to the ISR. It is not used in
 * this implementation.
 *
 * @return None
 */
void capture_done_isr(void *ctxt)
{
#if defined(ML_APP_MOD)
	/* Clear capture and rescale stage done status bits
	 */
	GARD__STOP_CAPTURE_STAGE();
	GARD__STOP_RESCALE_STAGE();

	/* Allow pause controller to halt the pipeline after capture if requested.
	 */
	pipeline_stage_completed(PIPELINE_STAGE_CAPTURE_DONE);
	if (PIPELINE_PAUSED == ml_pipeline_state) {
		return;
	}

	/* start rescale stage */
	setup_image_rescale_parameters();

	GARD__SET_RESCALE_CONFIGS(ML_APP_1_PREINPUT_START_ADDRESS,
							  ML_APP_1_INPUT_START_ADDRESS);

	GARD__START_RESCALE_STAGE();

	rescaling_started = true;
#endif
}

/**
 * rescaling_done_isr() is the ISR that is called when the rescaling engine has
 * completed the image capture and scaling process. This routine will move the
 * image to the ML engine's read buffer so that it may be processed by the ML
 * engine.
 *
 * @param ctxt is a pointer to the context passed to the ISR. It is not used in
 * this implementation.
 *
 * @return None
 */
void rescaling_done_isr(void *ctxt)
{
#ifdef ML_APP_HMI
	/* Clear capture start since process completed */
	GARD__STOP_SCALER_ENGINE();

	/**
	 * If rescaling is done, but the ML engine is still running then we cannot
	 * move the captured image in the ML engine's buffers yet as they may still
	 * be in use by the ML engine. In this case we wait till the ML engine work
	 * is done, at which point we start the image movement to the ML engine.
	 * Also we cannot start a new image capture till the buffers in the scaler
	 * engine have been flushed. As soon as the scaler buffer contents are
	 * flushed out to the ML engine, we start a new image capture, which
	 * proceeds in parallel with the ML engine processing the previous image.
	 */
	/* Optionally pause before the scaler output is consumed by ML/buffer move.
	 */
	pipeline_stage_completed(PIPELINE_STAGE_RESCALE_DONE);
	if (PIPELINE_PAUSED == ml_pipeline_state) {
		return;
	}

	if (ml_engine_started) {
		waiting_to_copy_image = true;
	} else {
		gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_HIGH);

		/**
		 * Start the image movement from the rescaling engine to the ML engine.
		 * Wait for the operation to complete by polling for completion in the
		 * main loop.
		 */
		GARD__MOVE_DATA_FROM_SCALER_TO_ML_ENGINE();

		buffer_move_to_ml_started = true;
	}

#elif defined(ML_APP_MOD)
	/* Clear rescale start since process completed */
	GARD__STOP_RESCALE_STAGE();

	/* Optionally pause before the scaler output is consumed by ML/buffer move
	 */
	pipeline_stage_completed(PIPELINE_STAGE_RESCALE_DONE);
	if (PIPELINE_PAUSED == ml_pipeline_state) {
		return;
	}
#endif

	if (auto_exposure_enabled) {
		run_auto_exposure();
	}
}

/**
 * capture_rescaled_image_async() starts an asynchronous capture and rescale
 * operation to produce an image intended for transmission to HUB. Completion is
 * indicated when the pipeline transitions to the paused state at the rescale
 * stage.
 *
 * Note: This routine only initiates the process. Image availability is
 * asynchronous, so HUB should query readiness and image info before issuing the
 * read-at-offset request.
 *
 * @param None
 *
 * @return None
 */
void capture_rescaled_image_async(void)
{
	bool pipeline_active;

	/**
	 * Pause ML pipeline execution after the rescale stage so HUB can access the
	 * refreshed buffer once it becomes available.
	 */
	pause_pipeline_execution_async(PIPELINE_PAUSE_ON_RESCALE_DONE);

	/**
	 * Start capture of the image from the camera. Depending on
	 * the stage of the pipeline when it is determined that the next image is
	 * needed to continue.
	 */
	pipeline_active = GARD__IS_PIPELINE_ACTIVE();

	if (!pipeline_active) {
		capture_image_async();
	}
}

/**
 * is_rescaled_image_captured() reports whether the rescaled image requested
 * for HUB transmission is ready in the designated buffer. If true is returned,
 * HUB may proceed to call get_rescaled_image_info() to get the image details.
 * followed by transmission of image from the buffer. Readiness corresponds to
 * the pipeline being paused after the rescale stage completes.
 *
 * @return true if the rescale operation has completed and the image is ready
 *         to be read by HUB; false otherwise.
 */
bool is_rescaled_image_captured(void)
{
	return ((PIPELINE_PAUSED == ml_pipeline_state) &&
			(PIPELINE_STAGE_RESCALE_DONE == ml_pipeline_paused_at));
}

/**
 * get_rescaled_image_info() fills the provided `image_info` structure with the
 * buffer location, geometry, format, and size of the rescaled image produced by
 * the scaler pipeline. Callers should invoke this only after
 * is_rescaled_image_captured() returns true.
 *
 * @param rescaled_image Pointer to the structure that will receive the
 *                       rescaled image metadata.
 */
void get_rescaled_image_info(struct image_info *rescaled_image)
{
	if (!camera_started) {
		rescaled_image->image_data = (void *)0U;
		rescaled_image->width      = 0U;
		rescaled_image->height     = 0U;
		rescaled_image->format     = IMAGE_FORMAT__INVALID;
		rescaled_image->size       = 0U;
	} else {
		rescaled_image->image_data = (void *)ML_APP_1_INPUT_START_ADDRESS;
		rescaled_image->width      = BL_SCALER_RESCALE_CROP_OUT_WIDTH;
		rescaled_image->height     = BL_SCALER_RESCALE_CROP_OUT_HEIGHT;
		rescaled_image->format     = IMAGE_FORMAT__RGB_PLANAR;

		if (IMAGE_FORMAT__GRAYSCALE != rescaled_image->format) {
			rescaled_image->size = 3U * BL_SCALER_RESCALE_CROP_OUT_SIZE;
		}
	}
}

