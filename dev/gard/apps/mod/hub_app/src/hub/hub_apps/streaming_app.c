/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

/**
 * Application to demonstrate APP Data Streaming callback feature
 *
 * This application does the following:
 * 1. Pre-initializes HUB with host config JSON file and directory of GARD
 *    JSON files.
 * 2. Discovers GARDs in the system.
 * 3. Initializes HUB.
 * 4. Gets a handle to a specific GARD (GARD #0 in this case).
 * 5. Sets up an application data callback on the GARD handle.
 * 6. Waits for events to occur and the user callback to be invoked.
 * 7. Cleans up and exits.
 *
 * Note:
 * * 1. appdata_user_cb is a sample user callback function that gets invoked
 *      when data is available from the GARD. Feel free to modify it as needed
 * 		or provide a different callback function.
 * * 2. When data is not provided due to an error, the user callback is invoked
 *     	with size = 0.
 * * 3. Use Ctrl+ C to kill the application.
 */

/* For sleep */
#include <unistd.h>

/* For signal handler */
#include <signal.h>

#include "hub.h"

/* Global variable for hub_handle_t */
static hub_handle_t g_hub = NULL;

#define BUFFER_SIZE_FOR_CB (100)

/* Buffer for appdata_user_cb */
char appdata_buffer[BUFFER_SIZE_FOR_CB] = {0};

/* Signal handler to catch Ctrl+C and perform graceful cleanup */
void sigint_handler(int sig)
{
	printf("\nCtrl+C received. Exiting application gracefully.\n");
	hub_fini(g_hub);
	_exit(0);
}

/**
 * This is a sample user appdata callback function illustrating the use of the
 * HUB appdata shipping interface.
 *
 * This function API should follow the definition of hub_cb_handler_t.
 * This sample callback function assumes that the buffer is filled with string data for 
 * mere testing purposes. The Host Application running on HUB is expected to know the format 
 * and contents of the (opaque) buffer, and parse and/or interpret it accordingly.
 */
void *appdata_user_cb(void *params, void *p_buffer, uint32_t size)
{
	printf("User callback %s triggered.\n", __func__);

	if (params) {
		printf("App User Callback Context : %s\n", (char *)params);
	}

	if (p_buffer) {
		char *t = (char *)p_buffer;
		printf("App Buffer Size : %d\n", size);
		printf("App Buffer Content : %s\n", t);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	enum hub_ret_code ret;

	/* HUB handle - used for all transactions using HUB */
	hub_handle_t hub;
	/* GARD handle - used for all transactions using a particualr GARD */
	gard_handle_t grd;

	uint32_t      gard_num = 0;

	printf("Welcome to H.U.B. v%s\n", hub_get_version_string());
	if (argc < 3) {
		printf("Usage: %s <host_cfg_json_file> <directory of GARD jsons>\n",
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

	g_hub = hub;

	/* Set up the signal handler for graceful exit */
	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		printf("Failed to set up signal handler\n");
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
	printf("%d GARD(s) discovered.\n", hub_get_num_gards(hub));

	/* Initialize HUB */
	printf("\nRunning hub_init()...\n");
	ret = hub_init(hub);
	if (HUB_SUCCESS != ret) {
		printf("Error in hub_init!\n");
		goto err_app_1;
	}

	/* Get a handle to a specific GARD */
	printf("\nGetting GARD handle to GARD #%d...\n", gard_num);
	grd = hub_get_gard_handle(hub, gard_num);
	if (NULL == grd) {
		printf("Could not get GARD handle for GARD index %d\n", gard_num);
		goto err_app_1;
	}

	printf("Setting up appdata callback handler for data streaming...\n");

	ret = hub_setup_appdata_cb(
		grd, appdata_user_cb, NULL /* NULL user callback context */,
		(void *)&appdata_buffer[0], sizeof(appdata_buffer));
	if (HUB_SUCCESS != ret) {
		printf("Failed to set up appdata callback: %d!\n", ret);
		goto err_app_1;
	}

	printf("\nApplication is running. Monitoring events.\n");
	printf("Press Ctrl+C to exit and perform graceful cleanup.\n");

	/* Wait loop */
	while (1) {
		/**
		 * The main thread can perform other tasks here or simply sleep.
		 * All event handling is managed by HUB threads.
		 */
		sleep(1);
	}

	return 0;

err_app_1:
	printf("ERROR - running hub_fini()!\n");
	ret = hub_fini(hub);
	if (HUB_SUCCESS != ret) {
		printf("HUB fini failed!!\n");
		return -1;
	}

	return -1;
}
