/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

/* Sample test application of HUB .so usage */

#include "hub.h"

/* For srand() */
#include <time.h>

/* Test enablement support */

/* Define this to run register read-write test */
#define REG_READ_WRITE_TEST

/* Define this to run data buffer (send_data, recv_data) test */
#define SEND_RECV_DATA_TEST

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

	/* Now we use this GARD handle for all transactions with that GARD */

	/* Data size for tests */
	uint32_t data_sz = 1 * 1024;
	/* Register address in GARD memory map */
	uint32_t reg_addr;
	(void)reg_addr;

	/* Buffer for write tests */
	int32_t *p_value_write = (int32_t *)calloc(data_sz, sizeof(int32_t));
	if (NULL == p_value_write) {
		printf("Cannot allocate memory for reg-write test\n");
		goto err_app_1;
	}

	/* Seed the random number generator */
	srand(time(NULL));

	/* Buffer for read tests */
	uint32_t *p_value_read = (uint32_t *)calloc(data_sz, sizeof(uint32_t));
	if (NULL == p_value_read) {
		printf("Cannot allocate memory for reg-read test\n");
		goto err_app_1;
	}

#ifdef REG_READ_WRITE_TEST
	/* Register read/write accesses */

	/* Register write access */
	/* Note that the reg_addr needs to be 4-aligned! */
	reg_addr = 0x0C000000;

	/* Fill the write buffer with some random bytes */
	for (int i = 0; i < data_sz; i++) {
		p_value_write[i] = rand();
	}

	printf("REG-WRITE test [%d uint32's]:\n", data_sz);
	for (int i = 0; i < data_sz; i++) {
		ret = hub_write_gard_reg(grd, reg_addr, p_value_write[i]);
		if (HUB_SUCCESS != ret) {
			printf("Write error!\n");
			goto err_app_1;
		}
		printf("Writing %x @ 0x%x\r", p_value_write[i], reg_addr);
		reg_addr += 4;
	}
	printf("\n");

	/* Register read access */

	/* Note that the reg_addr needs to be 4-aligned! */
	reg_addr = 0x0C000000;

	printf("REG-READ test [%d uint32's]:\n", data_sz);
	for (int i = 0; i < data_sz; i++) {
		ret = hub_read_gard_reg(grd, reg_addr, &p_value_read[i]);
		if (HUB_SUCCESS != ret) {
			printf("Read error!\n");
			goto err_app_1;
		}
		printf("Reading %x from 0x%x\r", p_value_read[i], reg_addr);
		reg_addr += 4;
	}
	printf("\n");

	/* Data verity test */
	printf("REG-WRITE REG-READ Data Verity test:\n");
	for (int i = 0; i < data_sz; i++) {
		if (p_value_write[i] != p_value_read[i]) {
			printf("Data mismatch @ %d: write[0x%x], read[0x%x]\n", i,
				   p_value_write[i], p_value_read[i]);
			goto err_app_1;
		}
	}
	printf("Data matched!\n\n");
#endif /* REG_READ_WRITE_TEST */

#ifdef SEND_RECV_DATA_TEST
	/* Buffer read-write access */

	/* Send data access */
	/* Note that the reg_addr needs to be 4-aligned! */
	reg_addr = 0x0C000000;

	/* Fill the write buffer with some random bytes */
	for (int i = 0; i < data_sz; i++) {
		p_value_write[i] = rand();
	}

	printf("SEND_DATA test:\n");
	printf("Writing %ld bytes at GARD memory address 0x%x\n",
		   data_sz * sizeof(uint32_t), reg_addr);
	ret = hub_send_data_to_gard(grd, p_value_write, reg_addr,
								data_sz * sizeof(uint32_t));
	if (HUB_SUCCESS != ret) {
		printf("Send data error!\n");
		goto err_app_1;
	}

	/* Recv data access */
	/* Note that the reg_addr needs to be 4-aligned! */
	reg_addr = 0x0C000000;

	printf("RECV_DATA test:\n");
	printf("Reading %ld bytes from GARD memory address 0x%x\n",
		   data_sz * sizeof(uint32_t), reg_addr);
	ret = hub_recv_data_from_gard(
		grd, p_value_read, reg_addr, data_sz * sizeof(uint32_t));
	if (HUB_SUCCESS != ret) {
		printf("Recv data error!\n");
		goto err_app_1;
	} else {
		printf("Successfully received %lu bytes\n",
			   data_sz * sizeof(uint32_t));
	}

	/* Data verity test */
	printf("SEND_DATA RECV_DATA Data Verity test:\n");
	for (int i = 0; i < data_sz; i++) {
		if (p_value_write[i] != p_value_read[i]) {
			printf("Data mismatch @ %d: write[0x%x], read[0x%x]\n", i,
				   p_value_write[i], p_value_read[i]);
			goto err_app_1;
		}
	}

	printf("Data matched!\n\n");
#endif /* SEND_RECV_DATA_TEST */

	free(p_value_write);
	free(p_value_read);

	printf("SOM sensor data collection:\n");
	/* Get SOM sensors onboard data */

#define HUB_SOM_NUM_TEMP_SENSORS (2)
#define HUB_SOM_TEMP_SENSOR_ID_1 (1)
#define HUB_SOM_TEMP_SENSOR_ID_2 (2)

	uint32_t sensors_read;
	float    temperatures[HUB_SOM_NUM_TEMP_SENSORS];
	uint8_t  temperature_sensor_ids[HUB_SOM_NUM_TEMP_SENSORS] = {
        HUB_SOM_TEMP_SENSOR_ID_1, HUB_SOM_TEMP_SENSOR_ID_2};

	/* Get temperature from the sensors */
	sensors_read = hub_get_temperature_from_onboard_sensors(
		hub, temperature_sensor_ids, temperatures, HUB_SOM_NUM_TEMP_SENSORS);

	printf("Temperature read from %d sensors:\n", sensors_read);
	/**
	 * We print temperature values of all sensors given as inputs
	 * A -1.00 value for temperature indicates failure for that sensor.
	 */
	for (int i = 0; i < HUB_SOM_NUM_TEMP_SENSORS; i++) {
		printf("\tSensorID [%d] = %.4f C\n", temperature_sensor_ids[i],
			   temperatures[i]);
	}

#define HUB_SOM_NUM_ENERGY_SENSORS (2)
#define HUB_SOM_ENERGY_SENSOR_ID_1 (1)
#define HUB_SOM_ENERGY_SENSOR_ID_2 (2)

	struct hub_energy_sensor_values energy_values[HUB_SOM_NUM_ENERGY_SENSORS];
	uint8_t energy_sensor_ids[HUB_SOM_NUM_ENERGY_SENSORS] = {
		HUB_SOM_ENERGY_SENSOR_ID_1, HUB_SOM_ENERGY_SENSOR_ID_2};

	/* Get energy values from the sensors */
	sensors_read = hub_get_energy_from_onboard_sensors(
		hub, &energy_sensor_ids[0], &energy_values[0],
		HUB_SOM_NUM_ENERGY_SENSORS);

	printf("Energy values read from %d sensors:\n", sensors_read);
	/**
	 * We print energy values of all sensors given as inputs
	 * A -1.00 value for temperature indicates failure for that sensor.
	 */
	for (int i = 0; i < HUB_SOM_NUM_ENERGY_SENSORS; i++) {
		printf("\tSensorID [%d] = %.4f V, %.4f A, %.4f W\n",
			   energy_sensor_ids[i], energy_values[i].voltage,
			   energy_values[i].current, energy_values[i].power);
	}

	ret = hub_fini(hub);
	if (HUB_SUCCESS != ret) {
		printf("HUB fini failed!!\n");
		return -1;
	}

	return 0;

err_app_1:
	printf("App error - running hub_fini()!\n");
	ret = hub_fini(hub);
	if (HUB_SUCCESS != ret) {
		printf("HUB fini failed!!\n");
		return -1;
	}

	return -1;
}
