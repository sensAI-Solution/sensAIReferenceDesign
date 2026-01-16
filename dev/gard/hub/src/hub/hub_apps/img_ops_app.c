/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

/* Sample test application of HUB img ops usage */

/* For time() */
#include <time.h>
#include <unistd.h>

#include "hub.h"

#define GARD_CAMERA_ID (0)

/* Save a given image buffer as a BMP file */
int save_image_buffer_as_bmp(struct hub_img_ops_ctx *p_img_ops_ctx,
							 const char             *bmp_file_name)
{
	FILE *fp_bmp = NULL;

	/* BMP file header (14 bytes) */
	struct bmp_file_header {
		uint16_t file_type;
		uint32_t file_size;
		uint16_t reserved1;
		uint16_t reserved2;
		uint32_t offset_data;
	} __attribute__((packed)) bmp_file_header;

	/* BMP info header (BITMAPINFOHEADER - 40 bytes) */
	struct bmp_info_header {
		uint32_t size;
		int32_t  width;  /* Signed - positive for bottom-up */
		int32_t  height; /* Signed - positive for bottom-up */
		uint16_t planes;
		uint16_t bit_count;
		uint32_t compression;
		uint32_t image_size;
		int32_t  x_pels_per_meter;
		int32_t  y_pels_per_meter;
		uint32_t clr_used;
		uint32_t clr_important;
	} __attribute__((packed)) bmp_info_header;

	if (NULL == p_img_ops_ctx || NULL == bmp_file_name) {
		printf("Error in save_image_buffer_as_bmp!\n");
		return -1;
	}

	fp_bmp = fopen(bmp_file_name, "wb");
	if (NULL == fp_bmp) {
		printf("Error in fopen!\n");
		return -1;
	}

	switch (p_img_ops_ctx->image_format) {
	case HUB_IMAGE_FORMAT__RGB_NON_PLANAR:
		bmp_info_header.bit_count = 24;
		break;
	case HUB_IMAGE_FORMAT__RGB_PLANAR:
		bmp_info_header.bit_count = 24;
		break;
	case HUB_IMAGE_FORMAT__GRAYSCALE:
		bmp_info_header.bit_count = 8;
		break;
	default:
		printf("Image format is not supported!\n");
		fclose(fp_bmp);
		return -1;
	}

	/* Initialize the BMP file header */
	// Magic number 'BM' in little endian for BMP
	bmp_file_header.file_type = 0x4D42; /* 'BM' */
	bmp_file_header.reserved1 = 0;
	bmp_file_header.reserved2 = 0;
	/* offset_data will be set after calculating file_size */
	bmp_file_header.offset_data =
		sizeof(bmp_file_header) + sizeof(bmp_info_header);

	/* Initialize the BMP info header */
	bmp_info_header.size =
		sizeof(bmp_info_header); /* Must be 40 for BITMAPINFOHEADER */
	bmp_info_header.width = (int32_t)p_img_ops_ctx->h_size;
	bmp_info_header.height =
		(int32_t)p_img_ops_ctx->v_size; /* Positive = bottom-up */
	bmp_info_header.planes           = 1;
	/* bit_count is set above in the switch statement */
	bmp_info_header.compression      = 0; /* BI_RGB - uncompressed */
	bmp_info_header.image_size       = 0; /* Can be 0 for uncompressed */
	bmp_info_header.x_pels_per_meter = 0; /* Unspecified */
	bmp_info_header.y_pels_per_meter = 0; /* Unspecified */
	bmp_info_header.clr_used      = 0; /* 0 = use default (2^n for bit_count) */
	bmp_info_header.clr_important = 0; /* 0 = all colors important */

	/* Get the image width and height */
	uint32_t width                = p_img_ops_ctx->h_size;
	uint32_t height               = p_img_ops_ctx->v_size;

	/* Calculate bytes per pixel based on format */
	uint32_t bytes_per_pixel = (bmp_info_header.bit_count == 8) ? 1 : 3;

	/* Calculate padded row size (multiple of 4 bytes) */
	uint32_t row_stride           = (width * bytes_per_pixel + 3) & ~3u;

	/* Allocate pixel buffer for BMP data */
	uint8_t *bmp_pixel_data       = (uint8_t *)calloc(row_stride * height, 1);
	if (!bmp_pixel_data) {
		printf("Error: Unable to allocate BMP pixel buffer!\n");
		return -1;
	}

	if (p_img_ops_ctx->image_format == HUB_IMAGE_FORMAT__RGB_NON_PLANAR) {
		// RGB Non-Planar: contiguous RGB, i.e., RGBRGB... in memory
		for (uint32_t y = 0; y < height; ++y) {
			uint8_t *row_ptr = bmp_pixel_data + (height - 1 - y) * row_stride;
			const uint8_t *src_ptr =
				p_img_ops_ctx->p_image_buffer + y * width * 3;
			for (uint32_t x = 0; x < width; ++x) {
				// BMP expects BGR order
				*row_ptr++ = src_ptr[x * 3 + 2];  // B
				*row_ptr++ = src_ptr[x * 3 + 1];  // G
				*row_ptr++ = src_ptr[x * 3 + 0];  // R
			}
			// Padding is already zeroed by calloc
		}
	} else if (p_img_ops_ctx->image_format == HUB_IMAGE_FORMAT__RGB_PLANAR) {
		const uint8_t *R = p_img_ops_ctx->p_image_buffer;
		const uint8_t *G = R + width * height;
		const uint8_t *B = G + width * height;

		for (uint32_t y = 0; y < height; ++y) {
			uint8_t *row_ptr = bmp_pixel_data + (height - 1 - y) * row_stride;
			for (uint32_t x = 0; x < width; ++x) {
				*row_ptr++ = B[y * width + x];
				*row_ptr++ = G[y * width + x];
				*row_ptr++ = R[y * width + x];
			}
			// Padding is already zeroed via calloc
		}
	} else if (p_img_ops_ctx->image_format == HUB_IMAGE_FORMAT__GRAYSCALE) {
		// Grayscale: each pixel single byte
		for (uint32_t y = 0; y < height; ++y) {
			uint8_t *row_ptr = bmp_pixel_data + (height - 1 - y) * row_stride;
			const uint8_t *src_ptr = p_img_ops_ctx->p_image_buffer + y * width;
			for (uint32_t x = 0; x < width; ++x) {
				row_ptr[x] = src_ptr[x];
			}
			// Padding zeroed by calloc
		}
	}

	/* Calculate and set the total file size */
	bmp_file_header.file_size =
		sizeof(bmp_file_header) + sizeof(bmp_info_header) + row_stride * height;

	/* Write BMP file header */
	fwrite(&bmp_file_header, sizeof(bmp_file_header), 1, fp_bmp);
	/* Write BMP info header */
	fwrite(&bmp_info_header, sizeof(bmp_info_header), 1, fp_bmp);
	/* Write BMP pixel buffer */
	fwrite(bmp_pixel_data, 1, row_stride * height, fp_bmp);

	/* Close the BMP file */
	fclose(fp_bmp);

	/* Free the BMP pixel buffer */
	free(bmp_pixel_data);

	return 0;
}

int main(int argc, char *argv[])
{
	enum hub_ret_code ret;

	/* HUB handle - used for all transactions using HUB */
	hub_handle_t hub = NULL;
	/* GARD handle - used for all transactions using a particualr GARD */
	gard_handle_t grd;
	(void)grd;
	/* Context for image operations */
	struct hub_img_ops_ctx img_ops_ctx             = {0};

	char                   bmp_file_name[PATH_MAX] = {0};

	uint32_t               gard_num                = 0;

	printf("Welcome to H.U.B. v%s\n", hub_get_version_string());
	if (argc < 4) {
		printf("Usage: %s <host_cfg_json_file> <directory of GARD jsons> "
			   "<directory to save BMP files>\n",
			   argv[0]);
		return -1;
	}

	/**
	 * Pre-init HUB - hub handle gets filled with host props
	 * Either via system-wise discovery or host_config file passed in
	 */
	printf("\nRunning hub_preinit()...\n");
	ret = hub_preinit(argv[1], argv[2], &hub);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub preinit!\n");
		return -1;
	}

	/* Discover GARDs */
	printf("Running hub_discover_gards()...\n");
	ret = hub_discover_gards(hub);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_discover_gards!\n");
		goto err_app_1;
	}

	/* How many GARDs were discovered? */
	printf("%d GARD(s) discovered\n", hub_get_num_gards(hub));

	/* Initialize HUB */
	printf("\nRunning hub_init()...\n");
	ret = hub_init(hub);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_init!\n");
		goto err_app_1;
	}

	/* Get a handle to a specific GARD */
	printf("\nGetting GARD handle to GARD #%d\n", gard_num);
	grd = hub_get_gard_handle(hub, gard_num);
	if (NULL == grd) {
		printf("Could not get GARD handle for GARD index %d\n", gard_num);
		goto err_app_1;
	}
	printf("Performing operations on GARD %d...\n\n", gard_num);

	img_ops_ctx.camera_id = GARD_CAMERA_ID;

	/**
	 * Initiate a rescaled image capture and get the properties of the rescaled
	 * image from GARD
	 */
	ret = hub_capture_rescaled_image_from_gard(grd, &img_ops_ctx);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_capture_rescaled_image_from_gard!\n");
		goto err_app_1;
	}

	printf("Rescaled image properties:\n");
	printf("\tCamera ID: %d\n", img_ops_ctx.camera_id);
	printf("\tImage buffer size: %d\n", img_ops_ctx.image_buffer_size);
	printf("\tImage width: %d\n", img_ops_ctx.h_size);
	printf("\tImage height: %d\n", img_ops_ctx.v_size);
	printf("\tImage format: %d\n", img_ops_ctx.image_format);

	/* Allocate memory for the rescaled image */
	img_ops_ctx.p_image_buffer =
		calloc(img_ops_ctx.image_buffer_size, sizeof(uint8_t));
	if (NULL == img_ops_ctx.p_image_buffer) {
		printf("Error in calloc!\n");
		goto err_app_1;
	}

	/* Receive the rescaled image from GARD */
	ret = hub_recv_data_from_gard(grd, (void *)img_ops_ctx.p_image_buffer,
								  img_ops_ctx.image_buffer_address,
								  img_ops_ctx.image_buffer_size);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_recv_data_from_gard!\n");
		goto err_app_2;
	}

	/* Resume the pipelines in GARD */
	ret = hub_send_resume_pipeline(grd, img_ops_ctx.camera_id);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_send_resume_pipeline!\n");
		goto err_app_2;
	}

	/* Construct the BMP file name */
	snprintf(bmp_file_name, PATH_MAX, "%s/rescaled_image_GARD%d_CAM%d_%ld.bmp",
			 argv[3], gard_num, img_ops_ctx.camera_id, time(NULL));

	/* Save the rescaled image as a BMP file */
	ret = save_image_buffer_as_bmp(&img_ops_ctx, bmp_file_name);
	if (ret != 0) {
		printf("Error in save_image_buffer_as_bmp!\n");
		goto err_app_2;
	}

	printf("Rescaled image saved as %s\n", bmp_file_name);

	/* Clean up */
	/* Only call hub_fini if hub was initialized */
	if (hub != NULL) {
		ret = hub_fini(hub);
		if (HUB_SUCCESS != ret) {
			printf("HUB fini failed!!\n");
			return -1;
		}
	}

	return 0;

err_app_2:
	free(img_ops_ctx.p_image_buffer);
	img_ops_ctx.p_image_buffer = NULL;
err_app_1:
	if (hub != NULL) {
		printf("App error - running hub_fini()!\n");
		ret = hub_fini(hub);
		if (HUB_SUCCESS != ret) {
			printf("HUB fini failed!!\n");
			return -1;
		}
	}
	return -1;
}
