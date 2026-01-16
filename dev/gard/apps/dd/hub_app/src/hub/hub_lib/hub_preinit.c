/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include <cjson/cJSON.h>

/* HUB User-facing and GARD-facing APIs implementation */
#include "hub.h"

/* HUB utils uses */
#include "hub_utils.h"

/* HUB versioning and globals needed */
#include "hub_version.h"
#include "hub_globals.h"

/* Bus related */
#include "hub_i2c.h"
#include "hub_uart.h"
#include "hub_usb.h"

#include "gard_info.h"

/* Static function listing */
static enum hub_ret_code hub_print_bus_details(struct hub_gard_bus *p_bus);
static enum hub_ret_code hub_parse_host_config(char *p_host_config_file,
											   struct hub_ctx *p_hub);

/**
 * HUB INIT internal function
 *
 * Print out details of the Host side busses obtained from host config file.
 *
 * @param: p_bus is the HUB-GARD bus whose details will be printed
 *
 * @return: hub_ret_code
 * 		HUB_SUCCESS on success
 * 		HUB_FAILURE_INIT on failure
 */
static enum hub_ret_code hub_print_bus_details(struct hub_gard_bus *p_bus)
{
	enum hub_gard_bus_types bus_type;

	/* Pathological case */
	if (NULL == p_bus) {
		return HUB_SUCCESS;
	}

	bus_type = p_bus->types;
	switch (bus_type) {
	case HUB_GARD_BUS_I2C:
		hub_pr_dbg("\tbus_type: %s\n", hub_gard_bus_strings[bus_type]);
		hub_pr_dbg("\t\tbus_num: %u\n", p_bus->i2c.num);
		hub_pr_dbg("\t\tslave_id: %u\n", p_bus->i2c.slave_id);
		hub_pr_dbg("\t\tspeed: %u\n", p_bus->i2c.speed);
		break;
	case HUB_GARD_BUS_UART:
		hub_pr_dbg("\tbus_type: %s\n", hub_gard_bus_strings[bus_type]);
		hub_pr_dbg("\t\tbus_dev: %s\n", p_bus->uart.bus_dev);
		hub_pr_dbg("\t\tbaudrate: %u\n", p_bus->uart.baudrate);
		break;
	case HUB_GARD_BUS_USB:
		hub_pr_dbg("\tbus_type: %s\n", hub_gard_bus_strings[bus_type]);
		hub_pr_dbg("\t\tvendor_id: 0x%x\n", p_bus->usb.vendor_id);
		hub_pr_dbg("\t\tproduct_id: 0x%x\n", p_bus->usb.product_id);
		break;
	default:
		hub_pr_err("Unknown bus\n");
		return HUB_FAILURE_INIT;
	}

	return HUB_SUCCESS;
}

/**
 * HUB INIT internal function
 *
 * Parse a given host config json file and fill up HUB bus structures.
 *
 * @param: p_host_config_file is the full path to the host config file
 * @param: p_hub is the hub_ctx passed in for hub_init()
 *
 * @return: hub_ret_code
 *		HUB_SUCCESS on success
 *		HUB_FAILURE_PARSE_JSON on failure
 */
static enum hub_ret_code hub_parse_host_config(char *p_host_config_file,
											   struct hub_ctx *p_hub)
{
	enum hub_ret_code    ret;
	int                  i, j;
	int                  num_busses          = 0;

	struct hub_gard_bus *bus_props           = NULL;

	/* top-level cJSON object */
	cJSON *p_host_json                       = NULL;
	/* generic cJSON object */
	cJSON *p_json_obj                        = NULL;

	/* Holds the context of a file to be opened and read in by HUB */
	struct hub_file_ctx host_config_file_ctx = {
		.p_file_name    = p_host_config_file,
		.p_file_content = NULL,
		.file_size      = 0,
	};

	/* Read a given file indicated by the file context into memory */
	ret = hub_read_file_into_memory(&host_config_file_ctx);
	if (HUB_SUCCESS != ret) {
		goto parse_err_1;
	}

	/* Parse a given memory buffer - run it through cJSON parser */
	ret = hub_parse_json(&host_config_file_ctx, &p_host_json);
	if (HUB_SUCCESS != ret) {
		goto parse_err_2;
	}

	/**
	 * In the preinit case, we are parsing a host_config.json file
	 * which gives information about the various busses
	 * This information is read into a bus_props variable, which
	 * will be later attached to the HUB context.
	 */
	bus_props = (struct hub_gard_bus *)calloc(HUB_GARD_NR_BUSSES,
											  sizeof(struct hub_gard_bus));
	if (NULL == bus_props) {
		hub_pr_err("Error allocating memory for bus props\n");
		goto parse_err_3;
	}

	p_json_obj        = cJSON_GetObjectItemCaseSensitive(p_host_json, "busses");
	num_busses        = cJSON_GetArraySize(p_json_obj);

	/* We found these many number of busses in the host_config.json file */
	p_hub->num_busses = num_busses;

	/**
	 * Loop through all the busses and populate the bus_props structure
	 * accordingly
	 */
	for (i = 0; i < num_busses; i++) {
		cJSON *p_bus = cJSON_GetArrayItem(p_json_obj, i);
		cJSON *p_bus_field;

		/* Get the bus_type string from host_config.json */
		p_bus_field = cJSON_GetObjectItemCaseSensitive(p_bus, "bus_type");

		/**
		 * Find a match for this string from valid bus strings present in
		 * the variable hub_gard_bus_strings[]
		 * If a match is found, update the current bus_props' bus_type variable
		 */
		for (j = 0; j < HUB_GARD_NR_BUSSES; j++) {
			if (!strncmp(p_bus_field->valuestring, hub_gard_bus_strings[j],
						 strlen(p_bus_field->valuestring))) {
				bus_props[i].types = j;
				break;
			}
		}

		/**
		 * TBD-DPN: Check for (expected) busses not found in json and for which
		 * the function pointers would be NULL
		 *
		 * Fill in other properties of the current bus_props:
		 * 1. For I2C, we get bus_num, device_num, and speed
		 * 2. For UART, we get bus_dev and uart_baudrate
		 * 3. For USB, we get the vendor and product IDs
		 *
		 * Assign invalid (-1) value to the bus_hdl variable, and
		 * negate/falsify the is_open variable.
		 *
		 * Also, update the function pointers for open, write, read, close
		 * for each bus_props per the bus_type found.
		 */
		switch (bus_props[i].types) {
		case HUB_GARD_BUS_I2C:
			p_bus_field = cJSON_GetObjectItemCaseSensitive(p_bus, "bus_num");
			bus_props[i].i2c.num = p_bus_field->valueint;
			p_bus_field = cJSON_GetObjectItemCaseSensitive(p_bus, "device_num");
			bus_props[i].i2c.slave_id = p_bus_field->valueint;
			p_bus_field = cJSON_GetObjectItemCaseSensitive(p_bus, "i2c_speed");
			bus_props[i].i2c.speed         = p_bus_field->valueint;

			bus_props[i].i2c.bus_hdl       = -1;
			bus_props[i].i2c.is_open       = false;
			bus_props[i].fops.device_open  = hub_i2c_device_open;
			bus_props[i].fops.device_read  = hub_i2c_device_read;
			bus_props[i].fops.device_write = hub_i2c_device_write;
			bus_props[i].fops.device_close = hub_i2c_device_close;

			break;
		case HUB_GARD_BUS_UART:
			p_bus_field = cJSON_GetObjectItemCaseSensitive(p_bus, "bus_dev");
			snprintf(bus_props[i].uart.bus_dev,
					 strlen(p_bus_field->valuestring) + 1, "%s",
					 p_bus_field->valuestring);
			p_bus_field =
				cJSON_GetObjectItemCaseSensitive(p_bus, "uart_baudrate");
			bus_props[i].uart.baudrate     = p_bus_field->valueint;

			bus_props[i].uart.bus_hdl      = -1;
			bus_props[i].uart.is_open      = false;
			bus_props[i].fops.device_open  = hub_uart_device_open;
			bus_props[i].fops.device_read  = hub_uart_device_read;
			bus_props[i].fops.device_write = hub_uart_device_write;
			bus_props[i].fops.device_close = hub_uart_device_close;

			break;
		case HUB_GARD_BUS_USB:
			p_bus_field =
				cJSON_GetObjectItemCaseSensitive(p_bus, "usb_vendor_id");
			bus_props[i].usb.vendor_id =
				(uint32_t)strtol(p_bus_field->valuestring, NULL, 0);
			p_bus_field =
				cJSON_GetObjectItemCaseSensitive(p_bus, "usb_product_id");
			bus_props[i].usb.product_id =
				(uint32_t)strtol(p_bus_field->valuestring, NULL, 0);

			bus_props[i].usb.bus_hdl       = -1;
			bus_props[i].usb.is_open       = false;
			bus_props[i].fops.device_open  = hub_usb_device_open;
			bus_props[i].fops.device_read  = hub_usb_device_read;
			bus_props[i].fops.device_write = hub_usb_device_write;
			bus_props[i].fops.device_close = hub_usb_device_close;

			break;
		default:
			hub_pr_err("Unknown bus\n");
			goto parse_err_3;
		}
	}

	/* Free up the cJSON parsing variable created during the parse operation */
	cJSON_Delete(p_host_json);
	/* Free up the file buffer variable malloc'd */
	free(host_config_file_ctx.p_file_content);

	/* Attach the bus_props variable so create to the HUB context */
	p_hub->p_bus_props = bus_props;

	return HUB_SUCCESS;

parse_err_3:
	cJSON_Delete(p_host_json);
parse_err_2:
	free(host_config_file_ctx.p_file_content);
parse_err_1:
	return HUB_FAILURE_PARSE_JSON;
}

/******************************************************************************
	HUB publicly exposed APIs
 ******************************************************************************/

/**
 * Pre-initialize HUB and get a configured hub handle
 *
 * Options for p_config
 * 1. If p_config is NULL, perform a system scan on all busses.
 * This is currently not supported
 * 2. If p_config is non-NULL, assume it is a valid host config file
 * parse it and fill up HUB handle's bus components
 *
 * --- Memory is allocated to input hub handle on success - should be freed by
 * hub_fini() ---
 *
 * @param: p_config is a pointer to a host config file.
 * @param: p_gard_json_dir is the directory of GARD jsons
 * @param: hub is a pointer to a hub_handle_t
 *
 *  * @return: hub_ret_code
 *		HUB_SUCCESS on success
 *		HUB_FAILURE_PREINIT on failure
 */
enum hub_ret_code
	hub_preinit(void *p_config, char *p_gard_json_dir, hub_handle_t *hub)
{
	enum hub_ret_code ret;
	int               i;

	char             *p_host_config_file = (char *)p_config;

	if (NULL == p_config) {
		hub_pr_err("NULL p_config detected! Not supported in this version\n");
		hub_pr_err(
			"Please make sure you pass a pointer to a host config file for "
			"now!\n");
		return HUB_FAILURE_PREINIT;
	}

	/**
	 * No NULL check for p_gard_json_dir put here since this will be used in
	 * hub_discover_gards()
	 */

	struct hub_ctx *p_hub = (struct hub_ctx *)calloc(1, sizeof(struct hub_ctx));
	if (NULL == p_hub) {
		hub_pr_err("Error allocating memroy for hub_ctx !\n");
		goto hub_preinit_err_1;
	}

	ret = hub_parse_host_config(p_host_config_file, p_hub);
	if (HUB_SUCCESS != ret) {
		goto hub_preinit_err_2;
	}

	p_hub->hub_state = HUB_PREINIT_DONE;
	*hub             = p_hub;

	hub_pr_dbg("HUB bus config - %d busses found:\n", p_hub->num_busses);
	for (i = 0; i < p_hub->num_busses; i++) {
		hub_print_bus_details(&p_hub->p_bus_props[i]);
	}

	/**
	 * Critical: Need to copy the input (char *) into HUB handle variable 
	 * memory: p_hub->gard_json_dir
	 */
	snprintf(p_hub->gard_json_dir, strlen(p_gard_json_dir) + 1, "%s",
			 p_gard_json_dir);

	return HUB_SUCCESS;

hub_preinit_err_2:
	free(p_hub);
hub_preinit_err_1:
	return HUB_FAILURE_PREINIT;
}

/**
 * Get a string containing the current version of HUB.
 * This string is a global variable defined in hub_globals.c.
 * Only this function is supposed to use this string.
 *
 * @param: none
 *
 * @result: const char string of the version
 */
const char *hub_get_version_string()
{
	sprintf(hub_version_string, "%s", HUB_VERSION_STRING);

	return hub_version_string;
}