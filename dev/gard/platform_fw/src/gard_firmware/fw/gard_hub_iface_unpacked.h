/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 ******************************************************************************/

#ifndef GARD_HUB_IFACE_UNPACKED_H
#define GARD_HUB_IFACE_UNPACKED_H

/**
 * This file contains the same structures as gard_hub_iface.h without the
 * packing so that the RISC-V firmware can use these structures directly.
 * Please ensure that both the versions of the structures are kept in sync.
 */

#include "types.h"

/**
 * The padded version of the structures defined in gard_hub_iface.h.
 */

struct _host_requests_unpked {
	uint8_t command_id;  // Command identifier having a value from enum
						 // HostRequestCommandIdsOverUart

	union {
		uint8_t command_body[1];  // Point where the command body starts.

		// struct send_data_to_gard_for_offset_request is to be used when
		// command_id is SEND_DATA_TO_GARD_FOR_OFFSET.
		struct _send_data_to_gard_for_offset_request_unpked {
			struct {
				uint32_t offset_address;  // Offset address in GARD memory map.
				uint32_t data_size;       // Size of data to be sent to GARD
				uint16_t control_code;    // Control code
				uint16_t mtu_size;        // Maximum Transmission Unit size
			} cmd;

			uint8_t data[0];  // Data to be sent to GARD

			struct {
				uint32_t end_of_data_marker;  // END OF DATA marker
				uint32_t opt_crc;             // CRC for data integrity
			} eod;
		} send_data_to_gard_for_offset_request;

		// struct recv_data_from_gard_at_offset_request is to be used when
		// command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		struct _recv_data_from_gard_at_offset_request_unpked {
			uint32_t offset_address;  // Offset address in GARD memory map.
			uint32_t data_size;       // Size of data to be sent to GARD;
			uint16_t control_code;    // Control code
			uint16_t mtu_size;        // Maximum Transmission Unit size
		} recv_data_from_gard_at_offset_request;

		// struct read_reg_value_from_gard_at_offset_request is to be used when
		// command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		struct _read_reg_value_from_gard_at_offset_request_unpked {
			uint32_t offset_address;      // Offset address in GARD memory map.
			uint32_t end_of_data_marker;  // END OF DATA marker
		} read_reg_value_from_gard_at_offset_request;

		// struct write_reg_value_to_gard_at_offset_request is to be used when
		// command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		struct _write_reg_value_to_gard_at_offset_request_unpked {
			uint32_t offset_address;      // Offset address in GARD memory map.
			uint32_t data;                // 32-bit data to be written to GARD
			uint32_t end_of_data_marker;  // END OF DATA marker
		} write_reg_value_to_gard_at_offset_request;

		// struct get_ml_engine_status_request is to be used when command_id is
		// GET_ML_ENGINE_STATUS.
		struct _get_ml_engine_status_request_unpked {
			uint32_t engine_id;  // ID of the ML engine to get status for
			uint32_t end_of_data_marker;  // END OF DATA marker
		} get_ml_engine_status_request;

		struct _gard_discovery_request_unpked {
			uint32_t dummy[0];  // No data present in the discovery command body
		} gard_discovery_request;

		// struct capture_rescaled_image_request is to be used when
		// command_id is CAPTURE_RESCALED_IMAGE.
		struct _capture_rescaled_image_request_unpked {
			uint8_t  camera_id;           // Camera to take the image.
			uint32_t end_of_data_marker;  // END OF DATA marker
		} capture_rescaled_image_request_unpked;

		// struct resume_pipeline is to be used when
		// command_id is RESUME_PIPELINE.
		struct _resume_pipeline_request_unpked {
			uint8_t  camera_id;           // Camera whose pipeline to resume.
			uint32_t end_of_data_marker;  // END OF DATA marker
		} resume_pipeline_request_unpked;
	};
};

struct _host_responses_unpked {
	union {
		// struct send_data_to_gard_for_offset_response is to be used for
		// sending an optional ACK as response to command
		// SEND_DATA_TO_GARD_FOR_OFFSET.
		struct _send_data_to_gard_for_offset_response_unpked {
			uint8_t
				opt_ack;  // Optional ACK byte to send after command execution
		} send_data_to_gard_for_offset_response;

		// struct recv_data_from_gard_at_offset_response is to be used when
		// command_id is RECV_DATA_FROM_GARD_AT_OFFSET.
		struct _recv_data_from_gard_at_offset_response_unpked {
			uint32_t start_of_data_marker;  // START OF DATA marker
			uint32_t data_size;             // Size of data sent by GARD;
			uint8_t  data[0];               // Data received from GARD

			struct {
				uint32_t end_of_data_marker;  // END OF DATA marker
				uint32_t opt_crc;             // Optional CRC for data integrity
			} eod;
		} recv_data_from_gard_at_offset_response;

		// struct read_reg_value_from_gard_at_offset_response is to be used when
		// command_id is READ_REG_VALUE_FROM_GARD_AT_OFFSET.
		struct _read_reg_value_from_gard_at_offset_response_unpked {
			uint32_t start_of_data_marker;  // START OF DATA marker
			uint32_t
				reg_value;  // Register value read from GARD at specified offset
			uint32_t end_of_data_marker;  // END OF DATA marker
		} read_reg_value_from_gard_at_offset_response;

		// struct write_reg_value_to_gard_at_offset_response is to be used when
		// command_id is WRITE_REG_VALUE_TO_GARD_AT_OFFSET.
		struct _write_reg_value_to_gard_at_offset_response_unpked {
			uint8_t ack;  // ACK byte to send after write completion
		} write_reg_value_to_gard_at_offset_response;

		// struct get_ml_engine_status_response is to be used when command_id is
		// GET_ML_ENGINE_STATUS.
		struct _get_ml_engine_status_response_unpked {
			uint32_t start_of_data_marker;  // START OF DATA marker
			uint32_t status_code;           // Status code of the ML engine
			uint32_t end_of_data_marker;    // END OF DATA marker
		} get_ml_engine_status_response;

		// struct y is to be used when command_id is
		// DISCOVERY.
		struct _gard_discovery_response_unpked {
			uint32_t start_of_data_marker;  // START OF DATA marker
			uint8_t  signature[10];         // Signature for discovery response
			uint32_t end_of_data_marker;    // END OF DATA marker
			// No data present in the discovery response body
		} gard_discovery_response;

		// struct capture_rescaled_image_response is to be used when
		// command_id is CAPTURE_RESCALED_IMAGE.
		struct _capture_rescaled_image_response_unpked {
			uint32_t start_of_data_marker;  // START OF DATA marker
			uint32_t image_buffer_address;  // Image buffer address.
			uint32_t image_buffer_size;     // Size of the captured image.
			uint16_t h_size;                // Horizontal size of the image.
			uint16_t v_size;                // Vertical size of the image.
			uint32_t image_format;  // Definition from enum image_formats

			struct {
				uint32_t end_of_data_marker;  // END OF DATA marker
			} eod;
		} capture_rescaled_image_response_unpked;

		// struct resume_pipeline_response is to be used when
		// command_id is RESUME_PIPELINE.
		struct _resume_pipeline_response_unpked {
			uint8_t ack_or_nak;  // Pipeline resume status
		} resume_pipeline_response_unpked;
	};
};

#endif  // GARD_HUB_IFACE_UNPACKED_H

