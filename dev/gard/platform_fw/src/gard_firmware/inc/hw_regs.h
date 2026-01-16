/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef HW_REGS_H
#define HW_REGS_H

#include "gard_types.h"
#include "utils.h"

/**
 * This file which defines the hardware registers used by the GARD firmware.
 * The contents of this file are in addition to the contents in file
 * bsp/sys_platform.h.
 */

/**
 * ML Engine registers
 */
/* ML Engine Control register */
#define GARD__ML_ENG_CONTROL                       (*(volatile uint32_t *)0x4000387C)
/* ML Control register bits */
#define GARD__ML_ENG_CONTROL__START_ML_ENG         (1U << 0)

/* ML Engine Status register */
#define GARD__ML_ENG_RDY_STATUS                    (*(volatile uint32_t *)0x40003880)
/* ML Engine Status bits */
#define GARD__ML_ENG_RDY_STATUS__NOT_BUSY          (1U << 0)

/* ML Engine IRQ Clear register */
#define GARD__ML_ENG_IRQ_CLEAR                     (*(volatile uint32_t *)0x40003884)
/* ML Engine IRQ Clear bits */
#define GARD__ML_ENG_IRQ_CLEAR__CLR_IRQ            (1U << 0)

/* ML Engine Code Base register */
#define GARD__ML_ENG_CODE_BASE                     (*(volatile uint32_t *)0x40003854)

/* ML Engine read scaler data register */
#define GARD__ML_ENG_SCALER_COMM                   (*(volatile uint32_t *)0x4000323C)
/* ML Engine read scaler data register bits */
#define GARD__ML_ENG_SCALER_COMM__START_READ       (1U << 0)

/* ML Engine read scaler data completion register*/
#define GARD__ML_ENG_SCALER_XFER_STATUS            (*(volatile uint32_t *)0x40003244)
/* ML Engine read scaler data completion bits */
#define GARD__ML_ENG_SCALER_XFER_STATUS__READ_DONE (1U << 0)

/**
 * Image Capture registers
 */
/* Image Capture Control register */
#define GARD__IMG_CAPTURE_CTRL                     (*(volatile uint32_t *)0x40003230)
/* Image Capture Control bits */
#define GARD__IMG_CAPTURE_CTRL__START_CAPTURE      (1U << 0)

/* Rescale engine Status register */
#define GARD__RESCALE_ENG_STATUS                   (*(volatile uint32_t *)0x40003240)
/* Rescale engine Status bits */
#define GARD__RESCALE_ENG_STATUS__RESCALE_DONE                                 \
	(*(volatile uint32_t *)0x40003240)

#define GARD__RESCALE_ENG_RD_BUFF_ADDR     (*(volatile uint32_t *)0x4000321C)
#define GARD__RESCALE_ENG_WR_BUFF_ADDR     (*(volatile uint32_t *)0x40003218)

/* Image capture dimensions registers*/
#define GARD__IMG_CAPTURE_WIDTH            (*(volatile uint32_t *)0x40003210)
#define GARD__IMG_CAPTURE_HEIGHT           (*(volatile uint32_t *)0x40003214)
#define GARD__IMG_CAPTURE_CROP_LEFT        (*(volatile uint32_t *)0x40003204)
#define GARD__IMG_CAPTURE_CROP_RIGHT       (*(volatile uint32_t *)0x40003200)
#define GARD__IMG_CAPTURE_CROP_UPPER       (*(volatile uint32_t *)0x4000320C)
#define GARD__IMG_CAPTURE_CROP_BOTTOM      (*(volatile uint32_t *)0x40003208)
#define GARD__IMG_CAPTURE_OUT_IMG_WIDTH    (*(volatile uint32_t *)0x40003224)
#define GARD__IMG_CAPTURE_OUT_IMG_HEIGHT   (*(volatile uint32_t *)0x40003228)
#define GARD__IMG_CAPTURE_OUT_IMG_SIZE     (*(volatile uint32_t *)0x40003220)

/**
 * 2 stage scaler engine mini isp related registers
 */
/* Image Capture Control bits */
#define GARD__CAPTURE_GRAYSCALE            (1U << 5)
/* Auto white balance Control bits */
#define GARD__CAPTURE_ENABLE_AWB           (1U << 7)
/* Image Capture Control bits */
#define GARD__CAPTURE_START                (1U << 0)
/* Rescale Control bits */
#define GARD__RESCALE_START                (1U << 1)
/* Capture stage done status bits */
#define GARD__CAPTURE_STAGE_DONE_STATUS    (1U << 0)
/* Rescale stage done status bits */
#define GARD__RESCALE_STAGE_DONE_STATUS    (1U << 1)

/* Box scaler config registers */
#define GARD__BOX_SCALER_PRECROP_RL        (*(volatile uint32_t *)0x40012100)
#define GARD__BOX_SCALER_PRECROP_BU        (*(volatile uint32_t *)0x40012104)
#define GARD__BOX_SCALER_BOX_SC_FACTOR     (*(volatile uint32_t *)0x40012108)

/* Bilinear scaler config registers */
#define GARD__BL_SCALER_CROP_RL            (*(volatile uint32_t *)0x40012110)
#define GARD__BL_SCALER_CROP_BU            (*(volatile uint32_t *)0x40012114)

#define GARD__BL_SCALER_IN_DIM             (*(volatile uint32_t *)0x40012118)
#define GARD__BL_SCALER_IN_SIZE            (*(volatile uint32_t *)0x4001211C)

#define GARD__BL_SCALER_OUT_DIM            (*(volatile uint32_t *)0x40012120)
#define GARD__BL_SCALER_OUT_SIZE           (*(volatile uint32_t *)0x40012124)

/* capture config registers */
#define GARD__CAPTURE_FB_AXI_CONFIG        (*(volatile uint32_t *)0x40012130)
#define GARD__CAPTURE_FB_WR_BASE_ADDR      (*(volatile uint32_t *)0x40012134)
#define GARD__CAPTURE_FB_RD_BASE_ADDR      (*(volatile uint32_t *)0x40012138)
#define GARD__CAPTURE_FEATURE_CTRL         (*(volatile uint32_t *)0x40010010)
#define GARD__CAPTURE_START_TRIGGER        (*(volatile uint32_t *)0x40010004)

/* video apb config registers */
#define GARD__VIDEO_APB_CONFIG_0           (*(volatile uint32_t *)0x4000A000)
#define GARD__VIDEO_APB_CONFIG_8           (*(volatile uint32_t *)0x4000A008)
#define GARD__VIDEO_APB_CONFIG_C           (*(volatile uint32_t *)0x4000A00C)
#define GARD__VIDEO_APB_CONFIG_10          (*(volatile uint32_t *)0x4000A010)

/* ISP Image capture statistics */
#define GARD__CAPTURE_IMAGE_STATS          (*(volatile uint32_t *)0x40010014)

#if defined(ML_APP_HMI)
#define GARD__CAPTURE_IMAGE_STATS__IMG_AVG (*(volatile uint8_t *)0x40003400)
#elif defined(ML_APP_MOD)
#define GARD__CAPTURE_IMAGE_STATS__IMG_AVG (*(volatile uint8_t *)0x40010014)
#endif

#define GARD__CAPTURE_IMAGE_STATS__IMG_MAX (*(volatile uint8_t *)0x40010015)
#define GARD__CAPTURE_IMAGE_STATS__IMG_MIN (*(volatile uint8_t *)0x40010016)

#define MAX_GRAY_AVERAGE_SUPPORTED_BY_ISP  255
#define MIN_GRAY_AVERAGE_SUPPORTED_BY_ISP  0

/* 2 Stage mini isp capture stage status register */
#define GARD__2_STAGE_MISP_STATUS          (*(volatile uint32_t *)0x40010008)

/**
 * ML Engine related helper macros
 */
/**
 * Starting ML engine is performed by setting and immediately resetting the
 * START_ML_ENGINE bit in the ML_ENGINE_CONTROL register.
 */
#define GARD__START_ML_ENGINE()                                                \
	({                                                                         \
		GARD__SET_BITS(GARD__ML_ENG_CONTROL,                                   \
					   GARD__ML_ENG_CONTROL__START_ML_ENG);                    \
		GARD__RESET_BITS(GARD__ML_ENG_CONTROL,                                 \
						 GARD__ML_ENG_CONTROL__START_ML_ENG);                  \
	})

#define GARD__IS_ML_ENGINE_DONE()                                              \
	({                                                                         \
		bool is_done = false;                                                  \
		is_done      = GARD__GET_BITS(GARD__ML_ENG_RDY_STATUS,                 \
									  GARD__ML_ENG_RDY_STATUS__NOT_BUSY) &     \
				  GARD__ML_ENG_RDY_STATUS__NOT_BUSY;                           \
		is_done;                                                               \
	})
/**
 * Clearing the IRQ is done by setting and resetting the CLR_IRQ bit in the
 * ML_ENGINE_IRQ_CLEAR register.
 */
#define GARD__CLEAR_ISR()                                                      \
	({                                                                         \
		GARD__SET_BITS(GARD__ML_ENG_IRQ_CLEAR,                                 \
					   GARD__ML_ENG_IRQ_CLEAR__CLR_IRQ);                       \
		GARD__RESET_BITS(GARD__ML_ENG_IRQ_CLEAR,                               \
						 GARD__ML_ENG_IRQ_CLEAR__CLR_IRQ);                     \
	})

/**
 * Scaler engine related helper macros
 */
#define GARD__SET_INPUT_IMG_DIMENSIONS(width, height)                          \
	({                                                                         \
		GARD__IMG_CAPTURE_WIDTH  = (width);                                    \
		GARD__IMG_CAPTURE_HEIGHT = (height);                                   \
	})

#define GARD__SET_CROP_DIMENSIONS(left, right, upper, bottom)                  \
	({                                                                         \
		GARD__IMG_CAPTURE_CROP_LEFT   = (left);                                \
		GARD__IMG_CAPTURE_CROP_RIGHT  = (right);                               \
		GARD__IMG_CAPTURE_CROP_UPPER  = (upper);                               \
		GARD__IMG_CAPTURE_CROP_BOTTOM = (bottom);                              \
	})

#define GARD__SET_OUTPUT_IMG_DIMENSIONS(width, height)                         \
	({                                                                         \
		GARD__IMG_CAPTURE_OUT_IMG_WIDTH  = (width);                            \
		GARD__IMG_CAPTURE_OUT_IMG_HEIGHT = (height);                           \
		GARD__IMG_CAPTURE_OUT_IMG_SIZE   = (width) * (height);                 \
	})

#define GARD__SETUP_SCALER_ENGINE_BUFFERS()                                    \
	({                                                                         \
		GARD__RESCALE_ENG_WR_BUFF_ADDR = 0x0;                                  \
		GARD__RESCALE_ENG_RD_BUFF_ADDR = 0x0;                                  \
	})

#define GARD__START_SCALER_ENGINE()                                            \
	({                                                                         \
		GARD__SET_BITS(GARD__IMG_CAPTURE_CTRL,                                 \
					   GARD__IMG_CAPTURE_CTRL__START_CAPTURE);                 \
	})

#define GARD__IS_SCALER_ENGINE_DONE()                                           \
	({                                                                          \
		bool is_done = false;                                                   \
		is_done      = GARD__GET_BITS(GARD__RESCALE_ENG_STATUS,                 \
									  GARD__RESCALE_ENG_STATUS__RESCALE_DONE) & \
				  GARD__RESCALE_ENG_STATUS__RESCALE_DONE;                       \
		is_done;                                                                \
	})

#define GARD__STOP_SCALER_ENGINE()                                             \
	({                                                                         \
		GARD__RESET_BITS(GARD__IMG_CAPTURE_CTRL,                               \
						 GARD__IMG_CAPTURE_CTRL__START_CAPTURE);               \
	})

#define GARD__MOVE_DATA_FROM_SCALER_TO_ML_ENGINE()                             \
	({                                                                         \
		GARD__SET_BITS(GARD__ML_ENG_SCALER_COMM,                               \
					   GARD__ML_ENG_SCALER_COMM__START_READ);                  \
	})

#define GARD__IS_ML_ENGINE_READ_SCALER_DATA_DONE()                             \
	({                                                                         \
		bool is_data_read_done = false;                                        \
		is_data_read_done =                                                    \
			GARD__GET_BITS(GARD__ML_ENG_SCALER_XFER_STATUS,                    \
						   GARD__ML_ENG_SCALER_XFER_STATUS__READ_DONE) &       \
			GARD__ML_ENG_SCALER_XFER_STATUS__READ_DONE;                        \
		is_data_read_done;                                                     \
	})

#define GARD__CLEAR_MOVE_DATA_FROM_SCALER_TO_ML_ENGINE()                       \
	({                                                                         \
		GARD__RESET_BITS(GARD__ML_ENG_SCALER_COMM,                             \
						 GARD__ML_ENG_SCALER_COMM__START_READ);                \
	})

/**
 * 2 stage scaler engine mini isp related helper macros
 */

/**
 * Configures the box scaler engine with pre-crop dimensions and scaling
 * factors. Sets up the pre-crop left/right, upper/bottom boundaries and the X/Y
 * scaling factors. Usage: GARD__SET_BOX_SCALER_CONFIGS(precrop_left,
 * precrop_right, precrop_upper, precrop_bottom, scale_y, scale_x);
 *
 */
#define GARD__SET_BOX_SCALER_CONFIGS(precrop_left, precrop_right,              \
									 precrop_upper, precrop_bottom,            \
									 scaler_factor_y, scaler_factor_x)         \
	({                                                                         \
		GARD__BOX_SCALER_PRECROP_RL = (precrop_right << 16U) | precrop_left;   \
		GARD__BOX_SCALER_PRECROP_BU = (precrop_bottom << 16U) | precrop_upper; \
		GARD__BOX_SCALER_BOX_SC_FACTOR =                                       \
			(scaler_factor_x << 8U) | scaler_factor_y;                         \
	})

/**
 * Configures the bilinear scaler engine with crop dimensions, input and output
 * image parameters. Sets up crop boundaries (left/right, upper/bottom), input
 * image dimensions and size, and output image dimensions and size.
 * Usage: GARD__SET_BILINEAR_SCALER_CONFIGS(crop_left, crop_right, crop_upper,
 *        crop_bottom, in_height, in_width, in_size, out_height, out_width,
 * out_size);
 *
 */
#define GARD__SET_BILINEAR_SCALER_CONFIGS(                                     \
	crop_left, crop_right, crop_upper, crop_bottom, in_height, in_width,       \
	in_size, out_height, out_width, out_size)                                  \
	({                                                                         \
		GARD__BL_SCALER_CROP_RL  = (crop_right << 16U) | crop_left;            \
		GARD__BL_SCALER_CROP_BU  = (crop_bottom << 16U) | crop_upper;          \
		GARD__BL_SCALER_IN_DIM   = (in_width << 16U) | in_height;              \
		GARD__BL_SCALER_IN_SIZE  = in_size;                                    \
		GARD__BL_SCALER_OUT_DIM  = (out_width << 16U) | out_height;            \
		GARD__BL_SCALER_OUT_SIZE = out_size;                                   \
	})

/**
 * Configures video apb registers.
 * Usage: GARD__SET_CAPTURE_CONFIGS();
 *
 */
#define GARD__SET_VIDEO_APB_CONFIGS(config_0, config_8, config_c, config_10)   \
	({                                                                         \
		GARD__VIDEO_APB_CONFIG_0  = config_0;                                  \
		GARD__VIDEO_APB_CONFIG_8  = config_8;                                  \
		GARD__VIDEO_APB_CONFIG_C  = config_c;                                  \
		GARD__VIDEO_APB_CONFIG_10 = config_10;                                 \
	})

/**
 * Configures the image capture system with frame buffer settings and capture
 * features. Sets up the AXI configuration and enables auto white balance mode.
 * Usage: GARD__SET_CAPTURE_CONFIGS();
 *
 */
#define GARD__SET_MISP_CONFIGS()                                               \
	({                                                                         \
		GARD__CAPTURE_FB_AXI_CONFIG = 0x1F1F;                                  \
		GARD__RESET_BITS(GARD__CAPTURE_FEATURE_CTRL, 0x000000FF);              \
		GARD__SET_BITS(GARD__CAPTURE_FEATURE_CTRL, GARD__CAPTURE_ENABLE_AWB);  \
	})

/**
 * Configures the image capture system with frame buffer settings and capture
 * features. Sets up the AXI configuration, frame buffer write base address, and
 * enables auto white balance mode. Usage:
 * GARD__SET_CAPTURE_CONFIGS(capture_address);
 *
 */
#define GARD__SET_CAPTURE_CONFIGS(capture_address)                             \
	({ 																		   \
		GARD__CAPTURE_FB_WR_BASE_ADDR = capture_address;					   \
	})

/**
 * Configures the rescale engine with input and output frame buffer addresses.
 * Sets up the AXI configuration, frame buffer read base address (input),
 * frame buffer write base address (output), and enables auto white balance
 * mode. Usage: GARD__SET_RESCALE_CONFIGS(input_address, output_address);
 *
 */
#define GARD__SET_RESCALE_CONFIGS(input_address, output_address)               \
	({                                                                         \
		GARD__CAPTURE_FB_WR_BASE_ADDR = output_address;                        \
		GARD__CAPTURE_FB_RD_BASE_ADDR = input_address;                         \
	})

/**
 * Starts the image capture process by triggering the capture start signal.
 * Sets the GARD__CAPTURE_START bit in the GARD__CAPTURE_START_TRIGGER register
 * to initiate image capture operation.
 * Usage: GARD__START_CAPTURE_STAGE();
 *
 */
#define GARD__START_CAPTURE_STAGE()                                            \
	({                                                                         \
		GARD__RESET_BITS(GARD__CAPTURE_START_TRIGGER, 0x000000FF);             \
		GARD__SET_BITS(GARD__CAPTURE_START_TRIGGER, GARD__CAPTURE_START);      \
	})

/**
 * Starts the rescale stage process by triggering the rescale start signal.
 * Sets the GARD__RESCALE_START bit in the GARD__CAPTURE_START_TRIGGER register
 * to initiate rescale operation.
 * Usage: GARD__START_RESCALE_STAGE();
 *
 */
#define GARD__START_RESCALE_STAGE()                                            \
	({                                                                         \
		GARD__RESET_BITS(GARD__CAPTURE_START_TRIGGER, 0x000000FF);             \
		GARD__SET_BITS(GARD__CAPTURE_START_TRIGGER, GARD__RESCALE_START);      \
	})

/**
 * Checks if the 2 stage scaler engine mini ISP has completed its operation.
 * Usage: bool is_done = GARD__IS_CAPTURE_STAGE_DONE()
 */
#define GARD__IS_CAPTURE_STAGE_DONE()                                          \
	({                                                                         \
		bool is_done = false;                                                  \
		is_done      = GARD__GET_BITS(GARD__2_STAGE_MISP_STATUS,               \
									  GARD__CAPTURE_STAGE_DONE_STATUS) &       \
				  GARD__CAPTURE_STAGE_DONE_STATUS;                             \
		is_done;                                                               \
	})

/**
 * Checks if the 2 stage scaler engine mini ISP has completed its operation.
 * Usage: bool is_done = GARD__IS_RESCALE_STAGE_DONE()
 */
#define GARD__IS_RESCALE_STAGE_DONE()                                          \
	({                                                                         \
		bool is_done = false;                                                  \
		is_done      = GARD__GET_BITS(GARD__2_STAGE_MISP_STATUS,               \
									  GARD__RESCALE_STAGE_DONE_STATUS) &       \
				  GARD__RESCALE_STAGE_DONE_STATUS;                             \
		is_done;                                                               \
	})

/**
 * Stops the image capture process by resetting the capture start signal.
 * Sets the GARD__CAPTURE_STAGE_DONE_STATUS bit in the GARD__2_STAGE_MISP_STATUS
 * Usage: GARD__STOP_CAPTURE_STAGE()
 */
#define GARD__STOP_CAPTURE_STAGE()                                             \
	({                                                                         \
		GARD__SET_BITS(GARD__2_STAGE_MISP_STATUS,                              \
					   GARD__CAPTURE_STAGE_DONE_STATUS);                       \
	})

/**
 * Stops the rescale stage process by setting the rescale done status.
 * Sets the GARD__RESCALE_STAGE_DONE_STATUS bit in the GARD__2_STAGE_MISP_STATUS
 * register to indicate rescale operation completion.
 * Usage: GARD__STOP_RESCALE_STAGE()
 */
#define GARD__STOP_RESCALE_STAGE()                                             \
	({                                                                         \
		GARD__SET_BITS(GARD__2_STAGE_MISP_STATUS,                              \
					   GARD__RESCALE_STAGE_DONE_STATUS);                       \
	})

#endif /* HW_REGS_H */
