/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "gard_types.h"
#include "assert.h"
#include "sys_platform.h"
#include "uart.h"
#include "ospi_support.h"
#include "host_cmds.h"
#include "iface_support.h"
#include "app_module.h"
#include "fw_core.h"
#include "camera_config.h"
#include "fw_globals.h"
#include "ml_ops.h"
#include "hw_regs.h"
#include "camera_capture.h"
#include "gpio.h"
#include "gpio_support.h"
#include "pipeline_ops.h"

/**
 * fw_core_init() initializes the Gard Firmware (GARD FW) environment. This is
 * the only init function for the function, so if any additional initialization
 * needs to be done then it should be done here.
 *
 * @param None
 *
 * @return true if the initialization was successful, false otherwise.
 */
bool fw_core_init()
{
	/**
	 * Initialize the resources used by GARD FW here. Primary resources to be
	 * initialized are:
	 * - OSPI interface for accessing SPI flash
	 * - GPIO interface.
	 * - I2C interface with Host
	 * - UART interface with Host
	 * - Host Command service module
	 */

	sd = ospi_init();
	GARD__DBG_ASSERT(sd != NULL, "OSPI initialization failed");

	/* Initialize all the GPIOs used by the firmware. */
	gpio_init();

	/* Reset ml_engine_work_done as ML engine has not even started. */
	ml_engine_work_done = false;

	/* Initialize the slow serial interfaces. */
	ifaces_init();

	/* Initialize the Host Command service module. */
	host_request_init();

#ifdef ML_APP_HMI
	/* One-time setup of the camera parameters. */
	setup_camera_capture_parameters();
#elif defined(ML_APP_MOD)
	/* set up video apb registers
	 * <TBD-KDB> needs overhaul with bit field level defines
	 */
	GARD__SET_VIDEO_APB_CONFIGS(0x3D6U, 0x099E0667U, 0x0U, 0x09A00CD0U);
	setup_misp_config();
#else
#error "One of ML_APP_HMI or ML_APP_MOD should be defined"
#endif

	set_target_gray_average(DEFAULT_TARGET_GRAY_AVERAGE);

#ifdef TEST_AUTO_EXPOSURE
	set_target_gray_average(250);

	set_timer_for_time(10000);
#endif

	return true;
}

#if defined(TEST_APP_MOD_DATA_XFER_TRIGGER)
uint8_t  temp_buf[100]                        = "Sending data to Host\n";
uint32_t count_of_gpio_interrupts_to_dispatch = 10;
#endif

/**
 * main() is the main entry point for the firmware. At this point the C
 * subsystem has been initialized by the reset vector and bootstrap code. If
 * multi-phase loading is enabled then only partial firmware has been loaded and
 * the rest of the Part-2 of the firmware still needs to be loaded before
 * starting the main loop.
 *
 * @param None
 *
 * @return main never returns, still the return type is int for compatibility
 *         with the C standard.
 */
int main(void)
{
	bool         no_work         = false;
	app_handle_t app_ctxt_handle = NULL;
#ifdef TEST_AUTO_EXPOSURE
	/**
	 * When testing auto exposure, we can turn OFF the auto exposure toggling
	 * by setting this flag to false.
	 */
	bool     toggle_auto_exposure = auto_exposure_enabled;
	uint32_t gray_target          = 250;
#endif

	/**
	 * Implementation of app_preinit() is optional, if the App Module does
	 * implement it then it will also return a handle to App Context.
	 */
	if (NULL != app_module_callbacks.app_preinit_cb) {
		app_ctxt_handle = (app_module_callbacks.app_preinit_cb)();
	}

	/* GARD specific initialization happens here */
	GARD__DBG_ASSERT(fw_core_init(), "GARD Initialization Failed.");

	/**
	 * This is the final chance for App Module to initialize its data structures
	 */
	GARD__DBG_ASSERT(NULL != app_module_callbacks.app_init_cb,
					 "App Module does not implement app_init()");
	app_ctxt_handle = (app_module_callbacks.app_init_cb)(app_ctxt_handle);

	GARD__DBG_ASSERT(NULL != app_ctxt_handle,
					 "App Module Initialization Failed.");

	while (true) {
		/* Service Host Commands arriving over various interfaces */
		if (service_host_requests()) {
			no_work = false;
		}

		/* Service handlers for interfaces. */
		if (execute_rx_handlers()) {
			no_work = false;
		}

		if (execute_tx_handlers()) {
			no_work = false;
		}

		if (ml_engine_work_done) {
			ml_engine_work_done = false;

			/* ML engine has completed processing the image, call the
			 * app_ml_done() function to allow App Module to process the
			 * results.
			 */
			if (NULL != app_module_callbacks.app_ml_done_cb) {
				(app_module_callbacks.app_ml_done_cb)(
					app_ctxt_handle,
					(void *)get_info_of_last_executed_network()->inout_offset);
			}
			/* Process pipeline pause request at post processing boundary */
			pipeline_stage_completed(PIPELINE_STAGE_ML_POST_PROCESSING_DONE);
			no_work = false;
		}

		if (call_app_image_processing_done) {
			if (NULL != app_module_callbacks.app_image_processing_done_cb) {
				(app_module_callbacks.app_image_processing_done_cb)(
					app_ctxt_handle);
			}
			call_app_image_processing_done = false;
			no_work                        = false;
		}

#ifdef NO_ML_DONE_ISR
		/**
		 * This is a temporary workaround to continue the normal ML Done
		 * processing in the absence of ML_DONE ISR.
		 *
		 * TBD-SRP: Should be removed once the ML_DONE ISR is implemented.
		 */
		if (ml_engine_started && GARD__IS_ML_ENGINE_DONE()) {
			ml_engine_started = false;
			gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_LOW);
			ml_engine_done_isr(NULL);
		}
#endif

#ifdef NO_CAPTURE_DONE_ISR
		/**
		 * This is a temporary workaround to continue the normal capture done
		 * processing in the absence of CAPTURE_DONE_ISR.
		 *
		 * TBD-SRP: Should be removed once the CAPTURE_DONE_ISR ISR is
		 * implemented.
		 */
		if (capture_started && (false == ml_engine_started)
#if defined(ML_APP_MOD)
			&& GARD__IS_CAPTURE_STAGE_DONE()
#endif
		) {
			capture_started = false;
			gpio_pin_write(&gpio_0, GPIO_PIN_1, GPIO_OUTPUT_LOW);
			gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_LOW);
			capture_done_isr(NULL);
		}
#endif

#ifdef NO_RESCALE_DONE_ISR
		/**
		 * This is a temporary workaround to continue the normal Rescaling done
		 * processing in the absence of RESCALE_DONE_ISR.
		 *
		 * TBD-SRP: Should be removed once the RESCALE_DONE_ISR ISR is
		 * implemented.
		 */
		if (rescaling_started &&
#ifdef ML_APP_HMI
			GARD__IS_SCALER_ENGINE_DONE()) {
#elif defined(ML_APP_MOD)
			GARD__IS_RESCALE_STAGE_DONE()) {
#endif
			rescaling_started = false;
			gpio_pin_write(&gpio_0, GPIO_PIN_1, GPIO_OUTPUT_LOW);
			gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_LOW);
			rescaling_done_isr(NULL);

#ifdef ML_APP_MOD
			if ((NULL != app_module_callbacks.app_preprocess_cb) &&
				(PIPELINE_PAUSED != ml_pipeline_state)) {
				(app_module_callbacks.app_preprocess_cb)(app_ctxt_handle, NULL);
			}
#endif
		}
#endif

#ifdef NO_BUFF_MOVE_DONE_ISR
		/**
		 * This is a temporary workaround to continue the normal Rescaling done
		 * processing in the absence of RESCALE_DONE_ISR.
		 *
		 * TBD-SRP: Should be removed once the RESCALE_DONE_ISR ISR is
		 * implemented.
		 */
		if (buffer_move_to_ml_started &&
			GARD__IS_ML_ENGINE_READ_SCALER_DATA_DONE()) {
			buffer_move_to_ml_started = false;
			gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_LOW);

			if ((NULL != app_module_callbacks.app_preprocess_cb) &&
				(PIPELINE_PAUSED != ml_pipeline_state)) {
				(app_module_callbacks.app_preprocess_cb)(app_ctxt_handle, NULL);
			}
		}
#endif

#if defined(TEST_AUTO_EXPOSURE)
		if (has_timer_expired() && toggle_auto_exposure) {
			set_target_gray_average(gray_target);
			if (gray_target == 1) {
				gray_target = 250;
			} else {
				gray_target = 1;
			}

			set_timer_for_time(10000);
		}
#endif

#if defined(TEST_APP_MOD_DATA_XFER_TRIGGER)
		if (count_of_gpio_interrupts_to_dispatch && has_timer_expired()) {
			stream_data_to_host_async(temp_buf, sizeof(temp_buf), 100, NULL);
			if (--count_of_gpio_interrupts_to_dispatch != 0) {
				set_timer_for_time(5000);  // 5 seconds
			}
		}
#endif
		/**
		 * TBD-SRP: The current implementation of GARD does not support moving
		 * the captured image to HRAM buffer and hence we do not call
		 * app_rescale_done().
		 */

		/**
		 * Finally, if we did not do any work then we enter low-power mode
		 * until the next interrupt arrives.
		 */
		if (no_work) {
			/* Execute WFI instruction for CPU to enter low-power mode. */
		}
	}

	return 0;
}
