/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "types.h"
#include "assert.h"
#include "ospi_support.h"
#include "fw_core.h"
#include "fw_globals.h"
#include "memmap.h"
#include "utils.h"
#include "rfs.h"
#include "hw_regs.h"
#include "gpio.h"
#include "gpio_support.h"
#include "ml_info.h"
#include "pipeline_ops.h"

/**
 * This file defines the ML operations related interfaces used by the App
 * Module.
 *
 * These APIs provide a consistent and hardware-agnostic way for the App Module
 * to interact with the underlying GARD hardware. By abstracting
 * hardware-specific details, this interface layer ensures that the same
 * application logic can be reused across different hardware implementations
 * without requiring changes.
 *
 */

/**
 * p_networks_handler points to the App Module registered networks. This
 * variable is used throughout the firmware run to access the networks. The
 * space for the networks list is in the App Module space as he is the one
 * initializes it.
 */
static struct networks *p_networks_handler           = NULL;

/**
 * next_network_to_run is the UID of the next network scheduled to run on the
 * ML engine.
 */
static ml_network_handle_t next_network_to_run       = INVALID_NETWORK_HANDLE;

/**
 * currently_running_network is the UID of the network that is currently running
 * on the ML engine.
 */
static ml_network_handle_t currently_running_network = INVALID_NETWORK_HANDLE;

/**
 * last_executed_network is the UID of the last network that was executed on the
 * ML engine. This variable is used internally and is not exposed to App Module.
 */
static ml_network_handle_t last_executed_network     = INVALID_NETWORK_HANDLE;

/**
 * Currently we support a maximum of 5 networks hence we can have up to
 * (5 * 2=) 10 slots to hold dis-jointed input and output buffers across all
 * networks.
 */
#define MAX_FREE_MEM_SLOTS 10

/**
 * struct mem_range defines a memory range with a start address and size. It is
 * used in register_networks and its called to track free memory slots.
 */
struct mem_range {
	uint32_t start_addr;
	uint32_t size;
};

/**
 * create_empty_slot_at_index() creates an empty slot at the specified index. If
 * needed it also shifts up the existing occupied slots up to make space for the
 * new slot.
 *
 * @param index_of_empty_slot is the index at which the new empty slot should be
 * 							  created.
 * @param free_mem_slots is a pointer to the array of memory slots in which the
 * 						  new empty slot should be created.
 * @param valid_slots is a pointer to the variable that holds the count of valid
 * 					  slots in the free_mem_slots array.
 *
 * @return None
 */
static void create_empty_slot_at_index(uint32_t          index_of_empty_slot,
									   struct mem_range *free_mem_slots,
									   uint32_t         *valid_slots)
{
	uint32_t offset  = *valid_slots;

	*valid_slots    += 1;
	GARD__DBG_ASSERT((index_of_empty_slot < MAX_FREE_MEM_SLOTS) &&
						 (NULL != free_mem_slots) && (NULL != valid_slots) &&
						 (*valid_slots < MAX_FREE_MEM_SLOTS),
					 "Invalid memory range parameters");

	while (offset-- > index_of_empty_slot) {
		free_mem_slots[offset + 1] = free_mem_slots[offset];
	}
}

/**
 * delete_empty_slot_at_index() deletes the empty slot at the specified index.
 * If there are used slots after this one then the routine shifts down those
 * slots in effect creating an impression of deleting the slot.
 *
 * @param index_of_slot_to_del is the index of the slot that should be deleted.
 * @param free_mem_slots is a pointer to the array of memory slots in which the
 * 						  slot should be deleted.
 * @param valid_slots is a pointer to the variable that holds the count of valid
 * 					  slots in the free_mem_slots array.
 *
 * @return None
 */
static void delete_empty_slot_at_index(uint32_t          index_of_slot_to_del,
									   struct mem_range *free_mem_slots,
									   uint32_t         *valid_slots)
{
	*valid_slots -= 1;
	GARD__DBG_ASSERT((index_of_slot_to_del < MAX_FREE_MEM_SLOTS) &&
						 (NULL != free_mem_slots) && (NULL != valid_slots) &&
						 (*valid_slots < MAX_FREE_MEM_SLOTS),
					 "Invalid memory range parameters");

	while (index_of_slot_to_del < *valid_slots) {
		free_mem_slots[index_of_slot_to_del] =
			free_mem_slots[index_of_slot_to_del + 1];
		index_of_slot_to_del++;
	}
}

/**
 * allocate_space_for_buffer_in_mem_range() allocates space for a buffer in the
 * fragmented free memory range captured in free_mem_slots. The routine checks
 * the slot in which the buffer should be placed. If the slot is found then the
 * memory is set aside for this buffer by adjusting the slot dimensions to
 * account for the buffer that is being allocated.
 *
 * @param buffer_offset is the offset in the memory where the buffer should be
 * 						located in memory.
 * @param buffer_size is the size of the buffer that should be set aside in the
 * 						memory.
 * @param free_mem_slots is a pointer to the array of free memory slots
 * @param mem_slot_count is the count of total slots in the free_mem_slots
 * @param valid_slots is a pointer to the variable that holds the count of valid
 * 					  slots in the free_mem_slots array.
 *
 * @return true if the space for the buffer was successfully allocated,
 * 		  false otherwise.
 */
static bool
	allocate_space_for_buffer_in_mem_range(uint32_t          buffer_offset,
										   uint32_t          buffer_size,
										   struct mem_range *free_mem_slots,
										   uint32_t          mem_slot_count,
										   uint32_t         *valid_slots)
{
	uint32_t          idx;
	struct mem_range *fms = free_mem_slots;

	GARD__DBG_ASSERT((0U != buffer_size) && (NULL != free_mem_slots),
					 "Invalid memory range parameters");

	if (buffer_offset == USING_INTERNAL_BUFFERS) {
		/* Nothing to allocate when internal buffers are used. */
		return true;
	}

#if defined(ML_ENGINE_MEM_ALIGNMENT_BYTES) &&                                  \
	(ML_ENGINE_MEM_ALIGNMENT_BYTES > 1U)
	/**
	 * Ensure the offset satisfies the alignment requirements of the ML engine.
	 * If the size is not aligned then we will allocate space with extra bytes
	 * to satisfy the alignment requirements.
	 */
	GARD__DBG_ASSERT(((buffer_offset % ML_ENGINE_MEM_ALIGNMENT_BYTES) == 0),
					 "Buffer offset not aligned to ML engine requirements");
	if (0U != (buffer_size % ML_ENGINE_MEM_ALIGNMENT_BYTES)) {
		buffer_size += (ML_ENGINE_MEM_ALIGNMENT_BYTES -
						(buffer_size % ML_ENGINE_MEM_ALIGNMENT_BYTES));
	}
#endif

	for (idx = 0; idx < mem_slot_count; idx++) {
		if (fms->start_addr <= buffer_offset &&
			(fms->size >= (buffer_offset - fms->start_addr) + buffer_size)) {
			/* Found a slot that can accommodate the buffer */
			if (fms->start_addr == buffer_offset) {
				/* Buffer starts at the beginning of the slot */
				if (fms->size == buffer_size) {
					/* Release this free mem slot entry.*/
					delete_empty_slot_at_index(idx, free_mem_slots,
											   valid_slots);
				} else {
					fms->start_addr += buffer_size;
					fms->size       -= buffer_size;
				}
			} else if (fms->size ==
					   (buffer_offset - fms->start_addr + buffer_size)) {
				/* Buffer aligns to the end of the slot */
				fms->size = (buffer_offset - fms->start_addr);
			} else {
				/**
				 * Buffer is in the middle of the slot. We need to create an
				 * additional slot in this case.
				 *
				 * We do 2 things:
				 * 1. Adjust the size of the current slot size to account for
				 *    the buffer that is being allocated.
				 * 2. Create a new slot that holds the remaining memory that was
				 *    present in the original slot after taking out the space
				 *    for buffer.
				 */
				create_empty_slot_at_index(idx + 1, free_mem_slots,
										   valid_slots);

				/**
				 * Make a copy of current slot values in the new slot. We will
				 *  fix it soon.
				 */
				fms[1]             = *fms;

				fms->size          = (buffer_offset - fms->start_addr);

				fms[1].start_addr  = buffer_offset + buffer_size;
				fms[1].size       -= fms->size + buffer_size;
			}
			return true;
		}
		fms++;
	}
	return false;
}

/**
 * allocate_space_for_network_in_mem_range() allocates space for a network in
 * the fragmented free memory range captured in free_mem_slots. The routine
 * adjusts the slot whose memory was used to hold the network.
 *
 * @param network_size is the size of the network that should be set aside in
 * 					   the memory.
 * @param free_mem_slots is a pointer to the array of free memory slots
 * @param mem_slot_count is the count of total slots in the free_mem_slots
 * @param valid_slots is a pointer to the variable that holds the count of valid
 * * 				  slots in the free_mem_slots array.
 *
 * @return The start address of the allocated space for the network in HRAM or
 * 			0 if space was not found.
 */
static uint32_t
	allocate_space_for_network_in_mem_range(uint32_t          network_size,
											struct mem_range *free_mem_slots,
											uint32_t          mem_slot_count,
											uint32_t         *valid_slots)
{
	uint32_t          idx;
	struct mem_range *fms        = free_mem_slots;
	uint32_t          start_addr = 0U;

	GARD__DBG_ASSERT((0U != network_size) && (NULL != free_mem_slots),
					 "Invalid memory range parameters");

#if defined(ML_ENGINE_MEM_ALIGNMENT_BYTES) &&                                  \
	(ML_ENGINE_MEM_ALIGNMENT_BYTES > 1U)
	/**
	 * If the size is not aligned then we will allocate space with extra bytes
	 * to satisfy the alignment requirements.
	 */
	if (0U != (network_size % ML_ENGINE_MEM_ALIGNMENT_BYTES)) {
		network_size += (ML_ENGINE_MEM_ALIGNMENT_BYTES -
						 (network_size % ML_ENGINE_MEM_ALIGNMENT_BYTES));
	}
#endif

	for (idx = 0; idx < mem_slot_count; idx++) {
		if (network_size == fms->size) {
			start_addr = fms->start_addr;

			/**
			 * This slot is no more needed. Shift down the rest of the free mem
			 * slot entries.
			 */
			delete_empty_slot_at_index(idx, free_mem_slots, valid_slots);
			break;
		} else if (network_size < fms->size) {
			/**
			 * Network fits in the slot but leaves some space.
			 * We need to adjust the size of the current slot size to
			 * account for the network that is being allocated.
			 */
			start_addr       = fms->start_addr;

			fms->size       -= network_size;
			fms->start_addr += network_size;
			break;
		}
		fms++;
	}

#if defined(ML_ENGINE_MEM_ALIGNMENT_BYTES) &&                                  \
	(ML_ENGINE_MEM_ALIGNMENT_BYTES > 1U)
	/**
	 * Ensure the start address satisfies the alignment requirements of the ML
	 * engine. Logically the assert should never fail unless we have not covered
	 * all the bases to ensure alignment.
	 */
	GARD__DBG_ASSERT(((start_addr % ML_ENGINE_MEM_ALIGNMENT_BYTES) == 0),
					 "Start address not aligned to ML engine requirements");
#endif

	return start_addr;
}

/**
 * register_networks() takes a stock of networks that the App Module will be
 * needing and figures out a way to optimally distribute the available
 * hardware resources for running these networks.
 *
 * @param p_networks is a pointer to the struct networks that contains
 * information about the networks that the App Module will be using.
 *
 * @return true if the networks were successfully registered, false
 * otherwise.
 */
bool register_networks(struct networks *p_networks)
{
	uint32_t             ntwrk_idx;
	struct network_info *ntwrk;
	struct mem_range     network_mem_slots[MAX_FREE_MEM_SLOTS] = {0};
	struct mem_range     io_mem_slots[MAX_FREE_MEM_SLOTS]      = {0};
	uint32_t             network_valid_slots                   = 0;
	uint32_t             io_valid_slots                        = 0;
	uint32_t             ntwrk_addr;
	uint32_t             inout_start;
	uint32_t             inout_end;
	uint32_t             io_region_start;
	uint32_t             io_region_end;
	uint32_t             ntwrk_start;
	uint32_t             ntwrk_end;
	uint32_t             network_region_start;
	uint32_t             network_region_end;

	GARD__DBG_ASSERT((NULL != p_networks) &&
						 (0U != p_networks->count_of_networks) &&
						 (NULL != p_networks->networks),
					 "Invalid networks parameters");

	/**
	 * Following tasks are performed by this function:
	 * 1. Get an inventory of all the hardware resources available for
	 * running the networks.
	 * 2. Validate that the input and output buffers of individual networks
	 * do not overlap with each other. In future when parallel processing is
	 *    enabled, this routine will also ensure that the none of the input
	 * and output buffers across all the networks overlap with each other.
	 * 3. After memory has been set aside for the input and output buffers,
	 * and surplus memory is still available check if the networks can be
	 * loaded into this excess memory to improve performance. At least try
	 * to get the first network loaded into the HRAM.
	 */

	/**
	 * TBD-SRP
	 * The current version of the code assumes the following:
	 * - Enough HRAM is available to hold all the networks and their input
	 * and output buffers.
	 * - The input and output buffers of the networks do not overlap each
	 * other.
	 * - The buffers are assigned from the start of the HRAM for each
	 * network with the buffers of the network at the lowest offset (in the
	 * array) located at the lowest address of the HRAM.
	 */

	/* Start by assigning complete memory to the 1st array element. */

#if defined(ML_ENGINE_MEM_ALIGNMENT_BYTES) &&                                  \
	(ML_ENGINE_MEM_ALIGNMENT_BYTES > 1U)
	GARD__DBG_ASSERT(((HRAM_START_ADDR % ML_ENGINE_MEM_ALIGNMENT_BYTES) == 0),
					 "HRAM_START_ADDR not aligned to ML engine requirements");
	GARD__DBG_ASSERT(((HRAM_SIZE % ML_ENGINE_MEM_ALIGNMENT_BYTES) == 0),
					 "HRAM_SIZE not aligned to ML engine requirements");
#endif

	/* Seed the allocator with the ML network partition. */
	network_mem_slots[0].start_addr = HRAM_ML_NETWORKS_START_ADDR;
	network_mem_slots[0].size       = HRAM_ML_NETWORKS_SIZE;
	network_valid_slots             = 1;

	/* Seed the allocator with the ML IO partition. */
	io_mem_slots[0].start_addr      = HRAM_ML_IO_START_ADDR;
	io_mem_slots[0].size            = HRAM_ML_IO_SIZE;
	io_valid_slots                  = 1;

	for (ntwrk_idx = 0; ntwrk_idx < p_networks->count_of_networks;
		 ntwrk_idx++) {
		ntwrk = &p_networks->networks[ntwrk_idx];

		/* Locate networks in flash */
		GARD__DBG_ASSERT(
			locate_module_id(sd, ntwrk->network,
							 &ntwrk->fw_core_data.addr_of_network_in_flash,
							 &ntwrk->fw_core_data.network_size_in_bytes),
			"Register ML network failed.");

		/**
		 * Allocate space for the IO buffer in HRAM.
		 */
		if (USING_INTERNAL_BUFFERS != ntwrk->inout_offset) {
			/* Verify the inout buffer is within the ML IO region. */
			inout_start     = ntwrk->inout_offset;
			inout_end       = inout_start + ntwrk->inout_size;
			io_region_start = HRAM_ML_IO_START_ADDR;
			io_region_end   = io_region_start + HRAM_ML_IO_SIZE;

			GARD__DBG_ASSERT((inout_start >= io_region_start) &&
								 (inout_end <= io_region_end),
							 "Input buffer not within ML IO region");
		}

		allocate_space_for_buffer_in_mem_range(
			ntwrk->inout_offset, ntwrk->inout_size, io_mem_slots,
			GET_ARRAY_COUNT(io_mem_slots), &io_valid_slots);

		/* Remember the network is still not loaded in RAM. */
		ntwrk->fw_core_data.loaded_into_ram = false;
	}

	/**
	 * At this point we have reserved space for both input and output buffers in
	 * the HRAM for all the networks. The remaining HRAM memory can be used to
	 * place networks.
	 */

	/** We start to assign memory for holding the networks in HRAM. Currently
	 * we just take the first available memory and assign it to the network.
	 * Going forward we should find the most optimal way to place the networks
	 * in HRAM, since they could be of different sizes and we will need to have
	 * additional logic to ensure that all the networks find space in HRAM.
	 */

	for (ntwrk_idx = 0; ntwrk_idx < p_networks->count_of_networks;
		 ntwrk_idx++) {
		ntwrk      = &p_networks->networks[ntwrk_idx];

		ntwrk_addr = allocate_space_for_network_in_mem_range(
			ntwrk->fw_core_data.network_size_in_bytes, network_mem_slots,
			GET_ARRAY_COUNT(network_mem_slots), &network_valid_slots);

		GARD__DBG_ASSERT(0U != ntwrk_addr,
						 "Failed to allocate HRAM for ML network");

		/* Verifies the network allocation is within the networks region. */
		ntwrk_start = ntwrk_addr;
		ntwrk_end   = ntwrk_start + ntwrk->fw_core_data.network_size_in_bytes;
		network_region_start = HRAM_ML_NETWORKS_START_ADDR;
		network_region_end   = network_region_start + HRAM_ML_NETWORKS_SIZE;

		GARD__DBG_ASSERT((ntwrk_start >= network_region_start) &&
							 (ntwrk_end <= network_region_end),
						 "Network allocation outside ML networks region");

		GARD__ASSERT(
			ospi_read_from_flash(sd, (void *)ntwrk_addr,
								 ntwrk->fw_core_data.addr_of_network_in_flash,
								 ntwrk->fw_core_data.network_size_in_bytes) ==
				ntwrk->fw_core_data.network_size_in_bytes,
			"Load ML network failed");

		/* The network is now loaded in RAM for quicker access.*/
		ntwrk->fw_core_data.loaded_into_ram        = true;
		ntwrk->fw_core_data.addr_of_network_in_ram = ntwrk_addr;
	}

	p_networks_handler        = p_networks;
	next_network_to_run       = p_networks->networks[0].network;
	currently_running_network = INVALID_NETWORK_HANDLE;

	return true;
}

/**
 * get_network_info_for_uid() retrieves the network information for the given
 * network UID.
 *
 * @param network is the UID of the network for which the information is
 * 				  requested.
 *
 * @return A pointer to the network_info structure if the network is found,
 *         NULL otherwise.
 */
static struct network_info *
	get_network_info_for_uid(ml_network_handle_t network)
{
	uint32_t idx;

	GARD__DBG_ASSERT((NULL != p_networks_handler) &&
						 (NULL != p_networks_handler->networks),
					 "Networks not registered");

	for (idx = 0; idx < p_networks_handler->count_of_networks; idx++) {
		if (p_networks_handler->networks[idx].network == network) {
			return &p_networks_handler->networks[idx];
		}
	}

	return NULL;
}

/**
 * schedule_network_to_run() is invoked by routines in the App Module to
 * schedule the ML network that should be executed next on the ML engine. The
 * execution of this network could either be triggered by the App Module or
 * could be automatically done by the FW Core.
 *
 * @param network is the UID of the already registered network that should to be
 *                run next on the ML engine.
 *
 * @return true if the network was successfully scheduled to run, false
 *         otherwise.
 */
bool schedule_network_to_run(ml_network_handle_t network)
{
	uint32_t idx;
	bool     found_network = false;

	GARD__DBG_ASSERT(NULL != p_networks_handler, "Networks not registered");

	/**
	 * Search for network in the network list. If the network is found and needs
	 * to be loaded in RAM then do it now.
	 * If the network is not already loaded in RAM, then it is likely because
	 * there was no space in RAM to load it. In such a case we will postpone
	 * loading the network till the time the network is scheduled to run on ML
	 * engine.
	 * In either case we set the scheduled network offset to the
	 */

	for (idx = 0; idx < p_networks_handler->count_of_networks; idx++) {
		if (p_networks_handler->networks[idx].network == network) {
			/* Found the network to run */
			found_network = true;
			break;
		}
	}

	/* The following NOP is just to satisfy the compiler. */
	found_network = found_network;

	GARD__DBG_ASSERT(found_network,
					 "Network not found in the registered networks");

	/* Set network to be run next. */
	next_network_to_run = network;

	return true;
}

/**
 * get_uid_of_next_network_to_run() is called by the App Module to
 * retrieve the UID of the next ML network that will be run on the ML engine.
 *
 * @return The UID of the next network to run, or INVALID_NETWORK_HANDLE if no
 *         networks are registered.
 */
ml_network_handle_t get_uid_of_next_network_to_run(void)
{
	GARD__DBG_ASSERT((NULL != p_networks_handler), "Networks not registered");

	return next_network_to_run;
}

/**
 * get_uid_of_currently_running_network() is called by the App Module
 * to retrieve the UID of the currently running ML network on the ML engine.
 *
 * @return The UID of the currently running network, or INVALID_NETWORK_HANDLE
 *         if no networks are running on the ML engine.
 */
int32_t get_uid_of_currently_running_network(void)
{
	GARD__DBG_ASSERT((NULL != p_networks_handler), "Networks not registered");

	return currently_running_network;
}

/**
 * start_ml_engine() starts the ML engine to execute the network whose UID is
 * present in scheduled_network_to_run. If no network is scheduled, which will
 * happen only when the networks are not registered, then the function will
 * assert.
 *
 * @return None
 */
void start_ml_engine(void)
{
	struct network_info *p_network_to_start;
	struct network_info *p_network_last_run;

	GARD__DBG_ASSERT((NULL != p_networks_handler) ||
						 (INVALID_NETWORK_HANDLE != next_network_to_run),
					 "Networks not registered");

	/**
	 * A few things need to be done as a part of starting the engine:
	 * 1) If last run network in RAM overlaps with the address to hold the
	 *    new network then we mark the last run network as not loaded in RAM and
	 *    load the new network in RAM for execution.
	 * 2) If the network to be run is already loaded in RAM then no loading is
	 *    needed, if not then we load the new network from flash to RAM.
	 * 3) Initialize the ML engine registers to run the new network.
	 * 4) Start the ML engine.
	 */
	p_network_to_start = get_network_info_for_uid(next_network_to_run);

	if (last_executed_network != next_network_to_run &&
		INVALID_NETWORK_HANDLE != last_executed_network) {
		p_network_last_run = get_network_info_for_uid(last_executed_network);
		/**
		 * If we find an overlap of memory addresses between the last run
		 * network and the new network to be started then we mark the last
		 * network as not loaded in RAM so that it can be reloaded in RAM when
		 * it is scheduled to run again. This is because we are going to load
		 * the new network in RAM and thus overwrite the contents of the last
		 * run network.
		 */
		if (((p_network_last_run->fw_core_data.addr_of_network_in_ram <=
			  p_network_to_start->fw_core_data.addr_of_network_in_ram) &&
			 (p_network_last_run->fw_core_data.addr_of_network_in_ram +
				  p_network_last_run->fw_core_data.network_size_in_bytes >
			  p_network_to_start->fw_core_data.addr_of_network_in_ram)) ||
			((p_network_to_start->fw_core_data.addr_of_network_in_ram <
			  p_network_last_run->fw_core_data.addr_of_network_in_ram) &&
			 (p_network_to_start->fw_core_data.addr_of_network_in_ram +
				  p_network_to_start->fw_core_data.network_size_in_bytes >
			  p_network_last_run->fw_core_data.addr_of_network_in_ram))) {
			/**
			 * Network addresses in RAM overlap. In this case we mark the last
			 * execute network as not loaded in RAM so that it can be
			 * reloaded in RAM when it is scheduled to run again.
			 */
			p_network_last_run->fw_core_data.loaded_into_ram = false;
		} else {
			/**
			 * In case of no overlaps we leave the last executed network loaded
			 * in RAM so that it can be reused later when needed.
			 */
		}
	}
	if (!p_network_to_start->fw_core_data.loaded_into_ram) {
		/**
		 * Load the network in RAM so that it can be used by ML engine.
		 */
		GARD__ASSERT(
			ospi_read_from_flash(
				sd,
				(void *)p_network_to_start->fw_core_data.addr_of_network_in_ram,
				p_network_to_start->fw_core_data.addr_of_network_in_flash,
				p_network_to_start->fw_core_data.network_size_in_bytes) ==
				p_network_to_start->fw_core_data.network_size_in_bytes,
			"Load ML network failed");

		p_network_to_start->fw_core_data.loaded_into_ram = true;
	}

#ifdef ML_APP_HMI
	/* Clear Move data from scaler to ML done since process completed */
	GARD__CLEAR_MOVE_DATA_FROM_SCALER_TO_ML_ENGINE();
#endif

	/* Setup ML engine to run the new network. */
	GARD__ML_ENG_CODE_BASE =
		p_network_to_start->fw_core_data.addr_of_network_in_ram;

	gpio_pin_write(&gpio_0, GPIO_PIN_2, GPIO_OUTPUT_HIGH);
	/* Start ML Engine. */
	GARD__START_ML_ENGINE();

#ifdef NO_ML_DONE_ISR
	/* TBD-SRP: To be removed when the ML Done ISR becomes available. */
	ml_engine_started = true;
#endif

	/* ML Network started. Update local variables to indicate status */
	currently_running_network = next_network_to_run;
	last_executed_network     = next_network_to_run;

	/**
	 * We leave the next_network_to_run to the same network UID, the App Module
	 * will modify if he needs to run a different network next.
	 */
}

/**
 * ml_engine_done_isr() is triggered upon completion of the ML engine's
 * processing of the current network. This function is called by the IRQ handler
 * of RISC-V Core.
 *
 * @param ctx is the context passed to the ISR, which can be used to access
 *            application-specific data or state.
 *
 * @return None
 */
void ml_engine_done_isr(void *ctx)
{
	GARD__DBG_ASSERT((NULL != p_networks_handler) &&
						 (NULL != p_networks_handler->networks),
					 "Networks not registered");

	GARD__CLEAR_ISR();

#ifdef ML_APP_HMI
	if (waiting_to_copy_image) {
		/**
		 * Start the image movement from the rescaling engine to the ML engine
		 * because we now have the ML engine buffers available for use by the
		 * next network.
		 * Wait for the operation to complete by polling for completion in the
		 * main loop.
		 */
		GARD__MOVE_DATA_FROM_SCALER_TO_ML_ENGINE();

		buffer_move_to_ml_started = true;

		waiting_to_copy_image     = false;
	}
#endif

	/**
	 * At this point nothing is running on the ML engine, we reflect this state
	 * of the engine by resetting the currently running network variable.
	 */
	currently_running_network = INVALID_NETWORK_HANDLE;

	/**
	 * Signal to the main loop that ML engine work is complete and any
	 * processing needed to be done on the output data can be started.
	 */
	ml_engine_work_done       = true;

	/* Mark ML completion so pause requests can latch on this boundary. */
	pipeline_stage_completed(PIPELINE_STAGE_ML_DONE);
}

/**
 * get_info_of_last_executed_network() retrieves the network_info of the last
 * executed network. This API is not available to App Module but used internally
 * by FW Core for its operations.
 *
 * @return A pointer to the network_info structure of the last executed
 *         network, or NULL if no network has been executed yet.
 */
struct network_info *get_info_of_last_executed_network(void)
{
	GARD__DBG_ASSERT((NULL != p_networks_handler), "Networks not registered");
	GARD__DBG_ASSERT((INVALID_NETWORK_HANDLE != last_executed_network),
					 "Invalid state.");

	return get_network_info_for_uid(last_executed_network);
}

/**
 * get_ml_engine_status() retrieves the status of the specified ML engine.
 * The function takes an engine ID as input and returns a status code.
 *
 * Note: This is a placeholder implementation. In a real-world scenario,
 * this function would interact with the ML engine to retrieve its status.
 *
 * @param engine_id is the ID of the ML engine whose status is to be retrieved.
 *
 * @return A 32-bit unsigned integer representing the status of the ML engine.
 *         The actual status codes would depend on the specific implementation
 *         and requirements of the ML engine.
 */
uint32_t get_ml_engine_status(uint32_t engine_id)
{
	/* TBD-SRP Replace the following dummy value with actual status of engine */
	return 0xDEADBEEFU;
}

