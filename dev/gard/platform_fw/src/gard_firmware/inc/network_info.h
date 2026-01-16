/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef NETWORK_INFO_H
#define NETWORK_INFO_H

#include "gard_types.h"

/**
 * This file defines the structure whose variables the App Module needs to
 * provide to the FW Core to register the ML networks.
 */

/**
 * If internal IP memory, that is not accessible over the AXI bus, is used for
 * holding the buffer data then the App Module should define those buffers
 * (pointed by input_offset and output_offset) in struct network_info with the
 * value USING_INTERNAL_BUFFERS.
 * This indicate to the FW Core that this memory is not from HRAM and hence
 * valuable space from HRAM need not be reserved for these buffers. Also the
 * completion routines of App Module such as app_ml_done() and
 * app_image_processing_done() need not be called as there is nothing that these
 * processing routines can work with.
 */
#define USING_INTERNAL_BUFFERS ((uint32_t)~0)

/**
 * The struct network_info defines the information about a single ML network.
 * This structure is used by the App Module to provide the FW Core with the
 * necessary details about each ML network it intends to use.
 * At the end of this structure is a variable named fw_core_data which contains
 * information consumed by FW Core for this network. The App Module should not
 * modify this variable.
 */
struct network_info {
	/**
	 * ML firmware ID as it is assigned to this network when placing it in the
	 * flash memory. This ID is used by the FW Core to identify the network.
	 */
	ml_network_handle_t network;

	/* Buffer offset where the network will read the input image from
	 * and will write the output data.
	 */
	uint32_t inout_offset;

	/* Size of the input image and the output data present at input output
	 * buffer offset.
	 */
	uint32_t inout_size;

	/**
	 * This data is used by the FW Core for managing this network. App Module
	 * should not write or depend on the contents of the following variables.
	 */
	struct {
		/* Flash address where the network is located. */
		uint32_t addr_of_network_in_flash;

		/* Size of network in bytes. */
		uint32_t network_size_in_bytes;

		/* Flag indicating if the network has been loaded in RAM. */
		bool loaded_into_ram;

		/* RAM address where this network has been loaded. */
		uint32_t addr_of_network_in_ram;
	} fw_core_data;
};

/* INVALID_NETWORK_HANDLE should be used instead of -1 in the code.*/
#define INVALID_NETWORK_HANDLE ((ml_network_handle_t) - 1)

/**
 * Application Module needs to define a variable of this structure within one of
 * its source files and fill the variable with the information about the
 * networks that it will be using during its run. This structure is then passed
 * to FW Core by calling the function register_networks() in app_init().
 *
 * For example:
 * struct networks my_networks = {
 *		// We have 3 networks in this structure.
 *		.count_of_networks = 3,
 *
 *		.networks = {
 *			// First network information.
 *			{
 *				.network = FIRST_NETWORK_HANDLE,
 *				.input_offset = 0x1000,
 *				.input_size = 0x200,
 *				.output_offset = 0x3000,
 *				.output_size = 0x400,
 *			},
 *
 *			// Second network information.
 *			{
 *				.network = SECOND_NETWORK_HANDLE,
 *				.input_offset = 0x5000,
 *				.input_size = 0x600,
 *				.output_offset = 0x7000,
 *				.output_size = 0x800,
 *			},
 *
 * 			// Third network information.
 *			{
 * 				.network = THIRD_NETWORK_HANDLE,
 * 				.input_offset = 0x9000,
 * 				.input_size = 0xA00,
 * 				.output_offset = 0xB000,
 * 				.output_size = 0xC00,
 *			},
 *     },
 * };
 */

/**
 * The struct networks is used to hold multiple network_info structures, each of
 * which contain information about every ML network to be used by the App
 * Module.
 * The App Module should define a variable of this type and fill it with
 * the information about the networks it will be using. This structure is then
 * passed to the FW Core by calling the function register_networks() in
 * app_init().
 */
struct networks {
	/* Total networks that follow this variable. */
	uint32_t count_of_networks;

	/* Networks that the FW Core should manage. */
	struct network_info *networks;
};

#endif /* NETWORK_INFO_H */
