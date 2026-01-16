/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include <cjson/cJSON.h>

#include <unistd.h>  // Temporary

/* HUB User-facing and GARD-facing APIs implementation */
#include "hub.h"
#include "gard_hub_iface.h"

/* HUB utils uses */
#include "hub_utils.h"

/* HUB globals needed */
#include "hub_globals.h"

#include "gard_info.h"

#include "hub_threading.h"
#include "hub_gpio.h"

/* Static functions listing */
static enum hub_ret_code hub_send_discover_command(struct hub_gard_bus *p_bus);
static enum hub_ret_code hub_parse_gard_json(char *p_gard_json_filename,
											 struct hub_ctx       *p_hub,
											 struct hub_gard_info *p_gard_hdl);
static enum hub_ret_code hub_get_gard_profile_id(struct hub_ctx *p_hub,
												 int32_t *gard_profile_id);

/**
 * HUB INIT internal function
 *
 * Send a discover command on a HUB-GARD bus and determine
 * if there is a GARD connected to it
 *
 * Uses the gard_hub_iface.h for command and packet definitions.
 *
 * @param: p_bus is the HUB-GARD bus to send the discovery command on
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_GARD_DISCOVER on failure
 */
static enum hub_ret_code hub_send_discover_command(struct hub_gard_bus *p_bus)
{
	enum hub_ret_code       ret;
	int                     bus_hdl;
	ssize_t                 nread, nwrite;
	enum hub_gard_bus_types bus_type;

	void *p_bus_ctx = NULL;

	struct _host_requests  discover_cmd      = {0};
	struct _host_responses discover_response = {0};

	discover_cmd.command_id = GARD_DISCOVERY;

	bus_type = p_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		p_bus_ctx = (void *)&p_bus->i2c;
		break;
	case HUB_GARD_BUS_UART:
		p_bus_ctx = (void *)&p_bus->uart;
		break;
	default:
		hub_pr_dbg("%s: Bus not supported for discovery!\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_gard_discover_1;
	}

	/* Lock the bus mutex before bus operations */
	hub_mutex_lock(&p_bus->bus_mutex);

	/* Bus open */
	bus_hdl = p_bus->fops.device_open(p_bus_ctx);
	if (bus_hdl < 0) {
		hub_pr_err("Error opening probe bus %s\n",
				   hub_gard_bus_strings[bus_type]);
		goto err_gard_discover_2;
	}

	nwrite = p_bus->fops.device_write(bus_hdl, &discover_cmd.command_id,
									  sizeof(discover_cmd.command_id));
	if (sizeof(discover_cmd.command_id) != nwrite) {
		hub_pr_err("Error sending discover cmd\n");
		goto err_gard_discover_2;
	}

	/* Bus response collect */
	uint32_t intRead;

	int i = 0;
	while(discover_response.gard_discovery_response.start_of_data_marker != START_OF_DATA_MARKER && i++ < 50)
	{
		uint8_t oneRead;
		int j = 0;
		while(oneRead != (uint8_t)(START_OF_DATA_MARKER&0xff) && j++ < 50)
		{		
			nread = p_bus->fops.device_read(bus_hdl, (void *)&oneRead, sizeof(oneRead));
			if(nread != sizeof(oneRead)){
				hub_pr_err("Error getting discover response data marker\n");
				goto err_gard_discover_3;
			}
		}
		nread = p_bus->fops.device_read(bus_hdl, (void *)&intRead, 3);
		if(nread != 3){
			hub_pr_err("Error getting discover response data marker\n");
			goto err_gard_discover_3;
		}
		discover_response.gard_discovery_response.start_of_data_marker = (intRead<<8) | oneRead;
	}
		
	ssize_t remainingSize = sizeof(discover_response.gard_discovery_response)-sizeof(intRead);
	nread = p_bus->fops.device_read(
		bus_hdl, (void *)&discover_response.gard_discovery_response.signature,
		remainingSize);
	if (remainingSize != nread) {
		hub_pr_err("Error getting discover response\n");
		goto err_gard_discover_3;
	}

	/* We don't close any of the opened busses now as we will be using them for
	 * other operations */

	/* Unlock after successful discovery transaction (bus stays open) */
	hub_mutex_unlock(&p_bus->bus_mutex);

	hub_pr_info("\n\t bus_type: %d, discover_response.gard_discovery_response: %lu, nread %lu\n", bus_type, sizeof(discover_response.gard_discovery_response), nread);
	hub_pr_info("\n\t discover_response.gard_discovery_response.start_of_data_marker: %x\n (START_OF_DATA_MARKER=%x)\n", discover_response.gard_discovery_response.start_of_data_marker, START_OF_DATA_MARKER);
	hub_pr_info("\n\t discover_response.gard_discovery_response.end_of_data_marker:%x\n (END_OF_DATA_MARKER=%x)\n", discover_response.gard_discovery_response.end_of_data_marker, END_OF_DATA_MARKER);
    
	/* Parse discovery response */
	if ((START_OF_DATA_MARKER !=
		 discover_response.gard_discovery_response.start_of_data_marker) ||
		(END_OF_DATA_MARKER !=
		 discover_response.gard_discovery_response.end_of_data_marker)) {
		hub_pr_err("Error in discover response\n");
		goto err_gard_discover_1;
	}

	hub_pr_dbg("Read result -> %s\n",
			   discover_response.gard_discovery_response.signature);

	if (0 !=
		strncmp(
			HUB_GARD_DISCOVER_SIGNATURE,
			(const char *)discover_response.gard_discovery_response.signature,
			sizeof(HUB_GARD_DISCOVER_SIGNATURE))) {
		hub_pr_err("Error in discovery signature\n");
		hub_pr_dbg("Read result -> %s\n",
			   discover_response.gard_discovery_response.signature);
		printf("\nraw response:\n");
		
		printf("0x%x ", discover_response.gard_discovery_response.start_of_data_marker);
		for( int i=0; i<10; i++ ) {
			printf("0x%x(%c) ", discover_response.gard_discovery_response.signature[i], discover_response.gard_discovery_response.signature[i]);
		}
		printf("0x%x ", discover_response.gard_discovery_response.end_of_data_marker);
		printf("\n");
		goto err_gard_discover_1;
	}

	return HUB_SUCCESS;

err_gard_discover_3:
	ret = p_bus->fops.device_close(bus_hdl);
	(void)ret;
err_gard_discover_2:
	hub_mutex_unlock(&p_bus->bus_mutex);
err_gard_discover_1:
	return HUB_FAILURE_GARD_DISCOVER;
}

/**
 * HUB INIT internal function
 *
 * This is a dummy function, pending implementation in GARD FW
 * Right now it returns a canned profile ID 12345 *
 *
 * Get a profile ID from the attached GARD.
 *
 * @param: TBD
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		TBD on failure
 */
static enum hub_ret_code hub_get_gard_profile_id(struct hub_ctx *p_hub,
												 int32_t *gard_profile_id)
{
	*gard_profile_id = 12345;

	return HUB_SUCCESS;
}

/**
 * HUB INIT internal function
 *
 * Parse a given GARD json file and get GARD detail
 * Takes in a GARD handle, and on success, fills it up with all
 * details read from JSON, and initializes its busses
 *
 * @param: p_gard_json_filename is the GARD json filename
 * @param: p_hub is the hub_ctx (obtained from hub_preinit()
 * @param: p_gard_hdl is the GARD handle passed in
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_GARD_PROBE on failure
 */
static enum hub_ret_code hub_parse_gard_json(char *p_gard_json_filename,
											 struct hub_ctx       *p_hub,
											 struct hub_gard_info *p_gard_hdl)
{
	int                  i, j;
	enum hub_ret_code    ret;
	struct hub_gard_bus *p_gard_busses;

	/* top-level cJSON object */
	cJSON *p_gard_json = NULL;
	/* generic cJSON object */
	cJSON *p_json_obj = NULL;

	uint32_t num_gpio_inputs  = 0;
	uint32_t num_gpio_outputs = 0;

	/* Holds the context of a file to be opened and read in by HUB */
	struct hub_file_ctx json_file_ctx = {
		.p_file_name    = p_gard_json_filename,
		.p_file_content = NULL,
		.file_size      = 0,
	};

	/* Read a given file indicated by the file context into memory */
	ret = hub_read_file_into_memory(&json_file_ctx);
	if (HUB_SUCCESS != ret) {
		goto gard_parse_err_1;
	}

	/* Parse a given memory buffer - run it through cJSON parser */
	ret = hub_parse_json(&json_file_ctx, &p_gard_json);
	if (HUB_SUCCESS != ret) {
		goto gard_parse_err_2;
	}

	p_gard_busses = p_hub->p_bus_props;

	/**
	 * TBD-DPN: Error checks missing, to be coded in
	 *
	 * A GARD json file contains a gard_id, a gard_name and which busses it
	 * uses for what purpose:
	 * 1. control_bus for reg-read-write
	 * 2. data_bus for send_data, recv_data
	 *
	 * Get the values for these keys from the GARD json
	 *
	 * Then, depending on the bus type detected, point the control_bus /
	 * data_bus to the appropriate entry in the bus_props[] array already
	 * populated during hub_preinit()
	 */
	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "gard_id");
	p_gard_hdl->gard_id = p_json_obj->valueint;
	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "gard_name");
	snprintf(p_gard_hdl->gard_name, strlen(p_json_obj->valuestring) + 1, "%s",
			 p_json_obj->valuestring);

	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "control_bus");
	for (i = 0; i < HUB_GARD_NR_BUSSES; i++) {
		if (!strncmp(p_json_obj->valuestring, hub_gard_bus_strings[i],
					 strlen(p_json_obj->valuestring))) {
			for (j = 0; j < p_hub->num_busses; j++) {
				if (i == p_gard_busses[j].types) {
					p_gard_hdl->control_bus = &p_gard_busses[j];
					break;
				}
			}
		}
	}
	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "data_bus");
	for (i = 0; i < HUB_GARD_NR_BUSSES; i++) {
		if (!strncmp(p_json_obj->valuestring, hub_gard_bus_strings[i],
					 strlen(p_json_obj->valuestring))) {
			for (j = 0; j < p_hub->num_busses; j++) {
				if (i == p_gard_busses[j].types) {
					p_gard_hdl->data_bus = &p_gard_busses[j];
					break;
				}
			}
		}
	}

	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "gpio_chip");
	snprintf(p_gard_hdl->gpio_chip, strlen(p_json_obj->valuestring) + 1, "%s",
			 p_json_obj->valuestring);
	hub_pr_dbg("GPIO chip: %s\n", p_gard_hdl->gpio_chip);

	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "gpio_inputs");
	num_gpio_inputs = cJSON_GetArraySize(p_json_obj);
	hub_pr_dbg("Num GPIO inputs = %d\n", num_gpio_inputs);

	if (num_gpio_inputs) {
		p_gard_hdl->gpio_inputs =
			(uint32_t *)calloc(num_gpio_inputs, sizeof(uint32_t));
		if (NULL == p_gard_hdl->gpio_inputs) {
			hub_pr_err("Cannot allocate memory for gpio_inputs.\n");
			goto gard_parse_err_2;
		}
	}

	p_gard_hdl->num_gpio_inputs = num_gpio_inputs;

	for (i = 0; i < num_gpio_inputs; i++) {
		cJSON *p_pin = cJSON_GetArrayItem(p_json_obj, i);

		p_gard_hdl->gpio_inputs[i] = p_pin->valueint;
		hub_pr_dbg("Pin name:%s, Pin no:%d\n", p_pin->string,
				   p_gard_hdl->gpio_inputs[i]);
	}

	p_json_obj = cJSON_GetObjectItemCaseSensitive(p_gard_json, "gpio_outputs");
	num_gpio_outputs = cJSON_GetArraySize(p_json_obj);
	hub_pr_dbg("Num GPIO outputs = %d\n", num_gpio_outputs);

	if (num_gpio_outputs) {
		p_gard_hdl->gpio_outputs =
			(uint32_t *)calloc(num_gpio_outputs, sizeof(uint32_t));
		if (NULL == p_gard_hdl->gpio_outputs) {
			hub_pr_err("Cannot allocate memory for gpio_outputs.\n");
			goto gard_parse_err_3;
		}
	}
	p_gard_hdl->num_gpio_outputs = num_gpio_outputs;

	for (i = 0; i < num_gpio_outputs; i++) {
		cJSON *p_pin = cJSON_GetArrayItem(p_json_obj, i);

		p_gard_hdl->gpio_outputs[i] = p_pin->valueint;
		hub_pr_dbg("Pin name:%s, Pin no:%d\n", p_pin->string,
				   p_gard_hdl->gpio_outputs[i]);
	}

	/**
	 * Free up the cJSON parsing variable created during the parse operation
	 */
	cJSON_Delete(p_gard_json);
	/**
	 * Free up file buffer variable malloc'd when we read file into memory
	 */
	free(json_file_ctx.p_file_content);

	return HUB_SUCCESS;

gard_parse_err_3:
	free(p_gard_hdl->gpio_inputs);
gard_parse_err_2:
	free(json_file_ctx.p_file_content);
gard_parse_err_1:
	return HUB_FAILURE_GARD_PROBE;
}

/******************************************************************************
	HUB publicly exposed APIs
 ******************************************************************************/
/**
 * Given a HUB handle, run GARD discovery commands and discover GARDs,
 * fill up the HUB context internals accordingly.
 *
 * @param: hub is a HUB handle
 *
 * @return: hub_ret_code
		HUB_SUCCESS on success
		HUB_FAILURE_GARD_DISCOVER on failure
 */
enum hub_ret_code hub_discover_gards(hub_handle_t hub)
{
	int                     i, j, bus_hdl;
	enum hub_ret_code       ret;
	enum hub_gard_bus_types bus_type;

	struct hub_gard_info *p_gard;
	char                 *p_gard_json_dir;

	/* Structure to track discovered buses with their profile IDs */
	struct discovered_bus {
		struct hub_gard_bus *p_bus;
		int32_t              profile_id;
		bool                 is_unique;
	} *discovered_busses = NULL;

	struct hub_ctx *p_hub = (struct hub_ctx *)hub;
	if ((NULL == p_hub) || (p_hub->hub_state != HUB_PREINIT_DONE)) {
		hub_pr_err("Invalid HUB handle passed in or hub_preinit() not done!\n");
		goto hub_discover_err_1;
	}

	p_gard_json_dir = p_hub->gard_json_dir;
	if (NULL == p_gard_json_dir) {
		hub_pr_err("GARD json dir is NULL; cannot continue!\n");
		goto hub_discover_err_1;
	}

	/* Allocate temporary array to track discovered buses */
	discovered_busses = (struct discovered_bus *)calloc(
		p_hub->num_busses, sizeof(struct discovered_bus));
	if (NULL == discovered_busses) {
		hub_pr_err("Error allocating memory for discovered buses\n");
		goto hub_discover_err_1;
	}

	/**
	 * Fail count keeps track of how many buses couldn't be discovered
	 * for given GARD. If fail count matches the number of buses, then means
	 * no buses are discovered for given GARD and we cannot communicate
	 * with that GARD. Then ONLY fail the discovery.
	 */
	uint32_t fail_count = 0;
	/* First pass: Discover GARDs on all buses and get their profile IDs */
	for (i = 0; i < p_hub->num_busses; i++) {
		/* Initialize the bus mutex */
		ret = hub_mutex_init(&p_hub->p_bus_props[i].bus_mutex);
		if (HUB_SUCCESS != ret) {
			hub_pr_err("Failed to initialize bus mutex\n");
			goto hub_discover_err_2;
		}

		ret = hub_send_discover_command(&p_hub->p_bus_props[i]);
		if (HUB_SUCCESS == ret) {
			int32_t profile_id;

			/* Get profile ID using stub function (same for all buses) */
			ret = hub_get_gard_profile_id(p_hub, &profile_id);
			if (HUB_SUCCESS != ret) {
				hub_pr_err("Failed to get profile ID from bus %d\n", i);
				fail_count++;
				continue;
			}

			discovered_busses[i].p_bus      = &p_hub->p_bus_props[i];
			discovered_busses[i].profile_id = profile_id;
			discovered_busses[i].is_unique  = true;

			hub_pr_dbg("Discovered GARD on bus %d with profile ID = %d\n", i,
					   profile_id);

			/* Check if this profile ID was already seen on a previous bus */
			for (j = 0; j < i; j++) {
				/* Only check buses that had successful discoveries */
				if (discovered_busses[j].p_bus != NULL &&
					discovered_busses[j].profile_id == profile_id) {
					/* Duplicate profile ID found - mark as not unique */
					discovered_busses[i].is_unique = false;
					hub_pr_dbg("Profile ID %d already seen on bus %d, skipping "
							   "duplicate on bus %d\n",
							   profile_id, j, i);
					break;
				}
			}

			/* Count unique GARDs */
			if (discovered_busses[i].is_unique) {
				p_hub->num_gards++;
			}
		} else {
			fail_count++;
		}
	}

	if (fail_count == p_hub->num_busses) {
		hub_pr_err("All buses failed discovery!\n");
		goto hub_discover_err_2;
	}

	/* Allocate memory for unique GARD info */
	p_hub->p_gards = (struct hub_gard_info *)calloc(
		p_hub->num_gards, sizeof(struct hub_gard_info));
	if (NULL == p_hub->p_gards) {
		hub_pr_err("Error allocating memory for GARDs\n");
		goto hub_discover_err_2;
	}

	/* Second pass: Process only unique GARDs */
	j = 0;
	for (i = 0; i < p_hub->num_busses; i++) {
		if (!discovered_busses[i].is_unique) {
			continue; /* Skip duplicate profile IDs */
		}

		int32_t gard_profile_id = discovered_busses[i].profile_id;
		/**
		 * gard_json_file's size needs to be twice the max size of
		 * the equivalent variable in HUB handle p_hub->gard_json_dir
		 * which has been defined as PATH_MAX to make sure it can hold
		 * the possible addition of 2 such strings
		 */
		char gard_json_file[2 * PATH_MAX] = {0};

		p_gard = &p_hub->p_gards[j++];

		/* Every GARD handle has a pointer to the HUB handle */
		p_gard->hub = hub;

		hub_pr_dbg("Processing GARD with profile id = %d\n", gard_profile_id);

		sprintf(gard_json_file, "%s/gard_%d.json", p_gard_json_dir,
				gard_profile_id);

		ret = hub_parse_gard_json(gard_json_file, p_hub, p_gard);
		if (HUB_SUCCESS != ret) {
			hub_pr_err("Error parsing %s\n", gard_json_file);
			continue;
		}

		/**
		 * TBD-DPN: Need to make this code more elegant.
		 *
		 * Open the GARD's control busses if found
		 * TBD-DPN: To handle error cases and non-opens
		 */
		bus_type = p_gard->control_bus->types;
		switch (bus_type) {
		case HUB_GARD_BUS_I2C:
			bus_hdl = p_gard->control_bus->fops.device_open(
				(void *)&p_gard->control_bus->i2c);
			break;
		case HUB_GARD_BUS_UART:
			bus_hdl = p_gard->control_bus->fops.device_open(
				(void *)&p_gard->control_bus->uart);
			break;

		case HUB_GARD_BUS_USB:
			/**
			 * TBD-DPN: USB is not opened in hub_init() - needs to be
			 * treated uniformly in future
			 */
			hub_pr_dbg("USB is treated differently - not opening for now!\n");
			break;
		default:
			/* Ignoring for now; in future, we will flag it as a dead bus */
			hub_pr_err("Bus not supported for open!\n");
		};

		/**
		 * TBD-DPN: Need to make this code more elegant.
		 *
		 * Open the GARD's data busses if found
		 * TBD-DPN: To handle error cases and non-opens
		 */
		bus_type = p_gard->data_bus->types;
		switch (bus_type) {
		case HUB_GARD_BUS_I2C:
			bus_hdl = p_gard->data_bus->fops.device_open(
				(void *)&p_gard->data_bus->i2c);
			break;
		case HUB_GARD_BUS_UART:
			bus_hdl = p_gard->data_bus->fops.device_open(
				(void *)&p_gard->data_bus->uart);
			break;
		case HUB_GARD_BUS_USB:
			bus_hdl = p_gard->data_bus->fops.device_open(
				(void *)&p_gard->data_bus->usb);
			break;
		default:
			/* Ignoring for now; in future, we will flag it as a dead bus */
			hub_pr_err("Bus not supported for open!\n");
		};
	}
	(void)bus_hdl;

	/* Free temporary array */
	free(discovered_busses);

	p_hub->hub_state = HUB_DISCOVER_DONE;
	return HUB_SUCCESS;

hub_discover_err_2:
	/* Unlock and destroy all bus mutexes before cleanup */
	for (i = 0; i < p_hub->num_busses; i++) {
		/* Try to unlock in case a bus operation was interrupted */
		(void)hub_mutex_try_unlock(&p_hub->p_bus_props[i].bus_mutex);
		/* Destroy the mutex */
		(void)hub_mutex_destroy(&p_hub->p_bus_props[i].bus_mutex);
	}
	free(discovered_busses);
hub_discover_err_1:
	return HUB_FAILURE_GARD_DISCOVER;
}

/**
 * hub_init is a HUB initialization function.
 *
 * Given a HUB handle, this
 * 1. Starts a monitoring thread for GPIOs defined in the GARD json file.
 * 2. Allocates and initializes worker thread contexts.
 *
 * @param: hub is a HUB handle
 *
 * @return: hub_ret_code
		HUB_SUCCESS on success
		HUB_FAILURE_INIT on failure
 */
enum hub_ret_code hub_init(hub_handle_t hub)
{
	int               num_gpio_inputs;
	enum hub_ret_code ret;

	struct hub_gpio_mon_ctx   *p_hub_gpio_mon_ctx   = NULL;
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx = NULL;

	struct hub_ctx *p_hub = (struct hub_ctx *)hub;

	if ((NULL == p_hub) || (NULL == p_hub->p_gards)) {
		hub_pr_err("Invalid HUB handle.\n");
		goto hub_init_err_1;
	}

	num_gpio_inputs = p_hub->p_gards->num_gpio_inputs;
	if (!num_gpio_inputs) {
		/* TBD-SSP: Handle if GPIO pins are not given in GARD JSON
		 * currently setting init done and returning */
		hub_pr_err("GARD configuration doesn't have GPIO inputs.\n");
		p_hub->hub_state = HUB_INIT_DONE;
		return HUB_SUCCESS;
	}

	/* MONITOR THREAD */
	/* Allocate memory for HUB monitor context */
	p_hub_gpio_mon_ctx =
		(struct hub_gpio_mon_ctx *)calloc(1, sizeof(struct hub_gpio_mon_ctx));
	if (NULL == p_hub_gpio_mon_ctx) {
		hub_pr_err("Failed to allocate memory for hub_gpio_mon_ctx.\n");
		goto hub_init_err_1;
	}

	/* Open the GPIO chip */
	p_hub_gpio_mon_ctx->p_chip =
		gpiod_chip_open_by_name(p_hub->p_gards->gpio_chip);
	if (NULL == p_hub_gpio_mon_ctx->p_chip) {
		hub_pr_err("Failed to open %s GPIO chip.\n", p_hub->p_gards->gpio_chip);
		goto hub_init_err_2;
	}

	/* Get the chip's max nmumber of GPIO lines */
	p_hub_gpio_mon_ctx->num_chip_lines =
		gpiod_chip_num_lines(p_hub_gpio_mon_ctx->p_chip);
	if (p_hub_gpio_mon_ctx->num_chip_lines < 0) {
		hub_pr_err("Failed to get lines of %s GPIO chip.\n",
				   p_hub->p_gards->gpio_chip);
		goto hub_init_err_3;
	} else if (p_hub_gpio_mon_ctx->num_chip_lines == 0) {
		hub_pr_err("No lines on %s GPIO chip.\n", p_hub->p_gards->gpio_chip);
		gpiod_chip_close(p_hub_gpio_mon_ctx->p_chip);
		goto hub_init_err_3;
	}

	hub_pr_dbg("GPIO chip %s has %d lines.\n", p_hub->p_gards->gpio_chip,
			   p_hub_gpio_mon_ctx->num_chip_lines);

	/* Breaking condition for whiles loops in threads. Made true in hub_fini()
	 */
	p_hub_gpio_mon_ctx->terminate_flag = false;
	/* Monitor thread will not act unless initialized */
	p_hub_gpio_mon_ctx->mon_thread_initialized = false;

	/* Timeout wait for monitor thread */
	p_hub_gpio_mon_ctx->mon_timeout.tv_sec  = HUB_GPIO_MON_THREAD_TIMEOUT_SECS;
	p_hub_gpio_mon_ctx->mon_timeout.tv_nsec = HUB_GPIO_MON_THREAD_TIMEOUT_NSECS;

	/* doesn't return any error code */
	/* this just sets bulk->num_lines to 0 */
	gpiod_line_bulk_init(&p_hub_gpio_mon_ctx->mon_bulk);

	ret = hub_mutex_init(&p_hub_gpio_mon_ctx->mon_mutex);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize mutex for monitoring.\n");
		goto hub_init_err_3;
	}

	ret = hub_cond_var_init(&p_hub_gpio_mon_ctx->mon_cond_var);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to initialize mon_cond_var.\n");
		goto hub_init_err_4;
	}

	p_hub->p_gpio_mon_ctx = p_hub_gpio_mon_ctx;

	/* Start the monitoring thread */
	ret = hub_thread_create(&p_hub->p_gpio_mon_ctx->mon_thread_hdl,
							&p_hub->p_gpio_mon_ctx->mon_thread_attr,
							hub_gpio_monitor_thread_func, (void *)p_hub);
	if (HUB_SUCCESS != ret) {
		hub_pr_err("Failed to create monitor thread.\n");
		goto hub_init_err_5;
	}
	hub_pr_dbg("Creating monitoring thread. Handle - %ld\n",
			   p_hub->p_gpio_mon_ctx->mon_thread_hdl);

	/*  WORKER SETUP  */
	/* Allocate memory for HUB event contexts */
	p_hub_gpio_event_ctx = (struct hub_gpio_event_ctx *)calloc(
		p_hub_gpio_mon_ctx->num_chip_lines, sizeof(struct hub_gpio_event_ctx));
	if (NULL == p_hub_gpio_event_ctx) {
		hub_pr_err("Failed to allocate memory for p_hub_gpio_event_ctx.\n");
		goto hub_init_err_6;
	}

	p_hub->p_gpio_event_ctx = p_hub_gpio_event_ctx;

	p_hub->hub_state = HUB_INIT_DONE;
	return HUB_SUCCESS;

hub_init_err_6:
	/* Gracefully shut down monitor thread */
	hub_mutex_lock(&p_hub_gpio_mon_ctx->mon_mutex);
	hub_cond_var_signal(&p_hub_gpio_mon_ctx->mon_cond_var);
	hub_mutex_unlock(&p_hub_gpio_mon_ctx->mon_mutex);

	hub_thread_cancel(p_hub_gpio_mon_ctx->mon_thread_hdl);
	hub_thread_join(p_hub_gpio_mon_ctx->mon_thread_hdl, NULL);
hub_init_err_5:
	hub_cond_var_destroy(&p_hub_gpio_mon_ctx->mon_cond_var);
hub_init_err_4:
	hub_mutex_destroy(&p_hub_gpio_mon_ctx->mon_mutex);
hub_init_err_3:
	gpiod_chip_close(p_hub_gpio_mon_ctx->p_chip);
	p_hub_gpio_mon_ctx->p_chip = NULL;
hub_init_err_2:
	free(p_hub_gpio_mon_ctx);
	p_hub_gpio_mon_ctx = NULL;
hub_init_err_1:
	return HUB_FAILURE_INIT;
}

/**
 * Get the number of GARDS discovered, given a HUB handle
 *
 * @param: hub is the HUB handle
 *
 * @return: number of gARDS discovered (>=0)
 */
uint32_t hub_get_num_gards(hub_handle_t hub)
{
	struct hub_ctx *p_hub = (struct hub_ctx *)hub;
	if ((NULL == p_hub) || (p_hub->hub_state < HUB_DISCOVER_DONE)) {
		hub_pr_err("Invalid HUB handle passed in or Discovery is not done!\n");
		goto hub_get_num_gards_err_1;
	}

	return p_hub->num_gards;

hub_get_num_gards_err_1:
	return 0;
}

/**
 * Given a HUB handle, get the GARD handle corresponding to gard_num
 *
 * @param: hub is the HUB handle
 *  @param: gard_num is the GARD index
 *
 * @return: gard_handle
 * 	Will be NULL on failure
 */
gard_handle_t hub_get_gard_handle(hub_handle_t hub, uint8_t gard_num)
{
	uint32_t        num_gards;
	struct hub_ctx *p_hub = (struct hub_ctx *)hub;

	/* Handle pathological case */
	if ((NULL == hub) || (p_hub->hub_state != HUB_INIT_DONE)) {
		hub_pr_err("NULL HUB handle passed in or hub_init() not done!\n");
		return NULL;
	}

	num_gards = p_hub->num_gards;

	if (0 == num_gards) {
		hub_pr_err("No GARDs discovered!\n");
		return NULL;
	}

	if ((gard_num < 0) || (gard_num > (num_gards - 1))) {
		hub_pr_err("Invalid GARD num; should be between 0 and %d\n",
				   num_gards - 1);
		return NULL;
	}

	return (gard_handle_t)&p_hub->p_gards[gard_num];
}

/**
 * De-initialize the HUB library
 *
 * Frees up all memory allocated in the HUB library
 * Frees up all threads created in the HUB library
 * Destroys all mutexes and condition variables created
 * Closes all open busses found open, if any
 *
 * @param: hub is the HUB handle initialized by hub_init()
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_FINI on failure
 */
enum hub_ret_code hub_fini(hub_handle_t hub)
{
	int                     i, bus_hdl;
	enum hub_gard_bus_types bus_type;
	uint32_t                num_busses;

	struct hub_ctx            *p_hub                = NULL;
	struct hub_gpio_mon_ctx   *p_hub_gpio_mon_ctx   = NULL;
	struct hub_gpio_event_ctx *p_hub_gpio_event_ctx = NULL;

	/* Handle pathological case */
	if (NULL == hub) {
		return HUB_FAILURE_FINI;
	}

	p_hub = (struct hub_ctx *)hub;

	/* 1. INITIAL NULL CHECKS AND POINTER SETUP */

	if (!p_hub || !p_hub->p_gpio_mon_ctx) {
		/* Cannot proceed with GPIO cleanup without the main context. */
		hub_pr_err("NULL p_hub or p_hub->p_gpio_mon_ctx.\n");
		/* Proceed to final hub cleanup if p_hub is valid, but GPIO is not. */
		goto final_hub_cleanup;
	}

	p_hub_gpio_mon_ctx   = p_hub->p_gpio_mon_ctx;
	p_hub_gpio_event_ctx = p_hub->p_gpio_event_ctx;

	/* Set the termination flag to signal threads to exit gracefully. */
	p_hub_gpio_mon_ctx->terminate_flag = true;

	/* 2. STOP AND JOIN GPIO EVENT THREADS */

	if (p_hub_gpio_event_ctx) {
		for (i = 0; i < p_hub_gpio_mon_ctx->num_chip_lines; i++) {
			if (p_hub_gpio_event_ctx[i].event_thread_hdl) {
				/* Unblock the thread from waiting on the condition variable */
				hub_mutex_lock(&p_hub_gpio_event_ctx[i].event_mutex);
				hub_cond_var_signal(&p_hub_gpio_event_ctx[i].event_cond_var);
				hub_mutex_unlock(&p_hub_gpio_event_ctx[i].event_mutex);

				/* Wait for the thread to exit (JOIN) */
				hub_thread_join(p_hub_gpio_event_ctx[i].event_thread_hdl, NULL);
				hub_pr_dbg("Event thread %d joined.\n", i);
			}
		}
	}

	/* 3. STOP AND JOIN GPIO MONITOR THREAD */

	if (p_hub_gpio_mon_ctx->mon_thread_hdl) {
		/* Unblock the monitor thread if it is waiting */
		if (!p_hub_gpio_mon_ctx->mon_thread_initialized) {
			hub_mutex_lock(&p_hub_gpio_mon_ctx->mon_mutex);
			hub_cond_var_signal(&p_hub_gpio_mon_ctx->mon_cond_var);
			hub_mutex_unlock(&p_hub_gpio_mon_ctx->mon_mutex);
		}

		/* Wait for the thread to exit (JOIN) */
		hub_thread_join(p_hub_gpio_mon_ctx->mon_thread_hdl, NULL);
		hub_pr_dbg("Monitor thread joined.\n");
	}

	/* 4. RELEASE GPIO RESOURCES */

	if (p_hub_gpio_mon_ctx->p_chip) {
		/* Release bulk lines */
		if (gpiod_line_bulk_num_lines(&p_hub_gpio_mon_ctx->mon_bulk)) {
			gpiod_line_release_bulk(&p_hub_gpio_mon_ctx->mon_bulk);
		}

		/* Release individual lines for event threads */
		if (p_hub_gpio_event_ctx) {
			for (i = 0; i < p_hub_gpio_mon_ctx->num_chip_lines; i++) {
				if (p_hub_gpio_event_ctx[i].p_line) {
					gpiod_line_release(p_hub_gpio_event_ctx[i].p_line);
					p_hub_gpio_event_ctx[i].p_line = NULL;
				}
			}
		}
	}

	/* 5. DESTROY SYNCHRONIZATION PRIMITIVES */

	/* Destroy Event thread sync objects */
	if (p_hub_gpio_event_ctx) {
		for (i = 0; i < p_hub_gpio_mon_ctx->num_chip_lines; i++) {
			if (p_hub_gpio_event_ctx[i].event_thread_hdl) {
				hub_cond_var_destroy(&p_hub_gpio_event_ctx[i].event_cond_var);
				hub_mutex_destroy(&p_hub_gpio_event_ctx[i].event_mutex);
				hub_pr_dbg("Event thread %d's cond_var and mutex destroyed\n",
						   i);
			}
		}
	}

	if (p_hub_gpio_mon_ctx->mon_thread_hdl) {
		hub_cond_var_destroy(&p_hub_gpio_mon_ctx->mon_cond_var);
		hub_mutex_destroy(&p_hub_gpio_mon_ctx->mon_mutex);
		hub_pr_dbg("Monitor thread's cond_var and mutex destroyed\n");
	}

	/* 6. CLOSE CHIP AND FREE MEMORY */

	if (p_hub_gpio_mon_ctx->p_chip) {
		gpiod_chip_close(p_hub_gpio_mon_ctx->p_chip);
		p_hub_gpio_mon_ctx->p_chip = NULL;
	}

	if (p_hub_gpio_event_ctx) {
		free(p_hub_gpio_event_ctx);
		p_hub->p_gpio_event_ctx = NULL;
	}

	free(p_hub_gpio_mon_ctx);
	p_hub->p_gpio_mon_ctx = NULL;

	/* 7. CLEAN UP BUS SYNCHRONIZATION PRIMITIVES AND CLOSE THE BUSSES */

	num_busses = p_hub->num_busses;

	/**
	 * First, unlock all bus mutexes to allow cleanup to proceed
	 * This is critical if the app was interrupted during a bus operation
	 */
	if (p_hub->hub_state >= HUB_DISCOVER_DONE) {
		for (i = 0; i < num_busses; i++) {
			/* Force unlock all mutexes to allow cleanup */
			(void)hub_mutex_try_unlock(&p_hub->p_bus_props[i].bus_mutex);
		}
	}

	/**
	 * Send a close() command to all busses, any open busses will be closed
	 */
	for (i = 0; i < num_busses; i++) {
		bus_type = p_hub->p_bus_props[i].types;

		switch (bus_type) {
		case HUB_GARD_BUS_I2C:
			bus_hdl = p_hub->p_bus_props[i].i2c.bus_hdl;
			p_hub->p_bus_props[i].fops.device_close(bus_hdl);
			break;
		case HUB_GARD_BUS_UART:
			bus_hdl = p_hub->p_bus_props[i].uart.bus_hdl;
			p_hub->p_bus_props[i].fops.device_close(bus_hdl);
			break;
		case HUB_GARD_BUS_USB:
			bus_hdl = p_hub->p_bus_props[i].usb.bus_hdl;
			p_hub->p_bus_props[i].fops.device_close(bus_hdl);
			break;
		default:
			hub_pr_err("Unknown bus!\n");
			return HUB_FAILURE_FINI;
		}
	}

	/**
	 * Unlock and destroy all bus mutexes after closing all buses
	 * Note: Only destroy mutexes if discovery succeeded. If discovery failed,
	 * mutexes were already destroyed in the error path (hub_discover_err_2).
	 */
	if (p_hub->hub_state >= HUB_DISCOVER_DONE) {
		/* Try to unlock in case a bus operation was interrupted */
		/* Ignore errors - mutex might already be unlocked or locked by another
		 * thread */
		for (i = 0; i < num_busses; i++) {
			(void)hub_mutex_try_unlock(&p_hub->p_bus_props[i].bus_mutex);
			/* Try to destroy the mutex */
			/* Ignore errors - if mutex is still locked, OS will clean up on
			 * process exit */
			(void)hub_mutex_destroy(&p_hub->p_bus_props[i].bus_mutex);
		}
	}
	/* If discovery failed (state < HUB_DISCOVER_DONE), mutexes were already
	 * destroyed */


final_hub_cleanup:
	/* Free up all allocated memory for the main hub structure */
	if (p_hub) {
		if (p_hub->p_gards) {
			if (p_hub->p_gards->num_gpio_inputs) {
				free(p_hub->p_gards->gpio_inputs);
			}
			if (p_hub->p_gards->num_gpio_outputs) {
				free(p_hub->p_gards->gpio_outputs);
			}
			free(p_hub->p_gards);
		}
		if (p_hub->p_bus_props) {
			free(p_hub->p_bus_props);
		}
		free(p_hub);
	}

	return HUB_SUCCESS;
}