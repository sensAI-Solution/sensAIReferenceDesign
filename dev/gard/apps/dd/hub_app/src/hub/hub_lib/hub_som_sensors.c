/******************************************************************************
 * Copyright (c) 2024 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_som_sensors.h"

/**
 * HUB SENSORS internal function
 *
 * Get the I2C slave address of the temperature sensor given its sensor ID
 *
 * @param: sensor_id is the ID of the sensor
 *
 * @return: I2C aaddress of the said sensor if there is a match in the mapping
 * 			HUB_INVALID_SENSOR_ID if there is no match
 */
static uint8_t hub_get_temperature_sensor_i2c_slave_addr(uint8_t sensor_id)
{
	uint8_t slave_id = 0;

	switch (sensor_id) {
	case 1:
		slave_id = HUB_SOM_TEMP_SENSOR_1_SLAVE_ID;
		break;
	case 2:
		slave_id = HUB_SOM_TEMP_SENSOR_2_SLAVE_ID;
		break;
	default:
		slave_id = HUB_INVALID_SENSOR_ID;
	}

	return slave_id;
}

/**
 * HUB SENSORS internal function
 *
 * Get the I2C slave address of the energy sensor given its sensor ID
 *
 * @param: sensor_id is the ID of the sensor
 *
 * @return: I2C address of the said sensor if there is a match in the mapping
 * 			HUB_INVALID_SENSOR_ID if there is no match
 */
static uint8_t hub_get_energy_sensor_i2c_slave_addr(uint8_t sensor_id)
{
	uint8_t slave_id = 0;

	switch (sensor_id) {
	case 1:
		slave_id = HUB_SOM_ENERGY_SENSOR_1_SLAVE_ID;
		break;
	case 2:
		slave_id = HUB_SOM_ENERGY_SENSOR_2_SLAVE_ID;
		break;
	default:
		slave_id = HUB_INVALID_SENSOR_ID;
	}

	return slave_id;
}

/**
 * Read the temperature from the sensor(s) with given sensor IDs.
 *
 * Assumption: The sensor is attached on the same I2C bus as the GARD
 *
 * @param: hub is the HUB handle returned from a hub_init() call
 * @param: p_sensor_ids is an array of sensors IDs to read
 * @param: p_temp is a float array of termperatures (filled in on read)
 * @param: num_sensors is the number of sensors to read from
 *
 * @return: uint32_t indicating how many sensors were successfully read
 */
uint32_t hub_get_temperature_from_onboard_sensors(hub_handle_t hub,
												  uint8_t     *p_sensor_ids,
												  float       *p_temp,
												  uint8_t      num_sensors)
{
	int                  i;
	int                  bus_hdl;
	ssize_t              nread, nwrite;
	uint8_t              slave_id;
	uint32_t             num_sensors_read = 0;

	uint16_t             temp_int;
	uint8_t              raw_temp[2];

	struct hub_ctx      *p_hub;

	uint8_t              reg_addr  = TMP118_REG_TEMP;

	struct hub_gard_bus *p_bus_ctx = NULL;

	/* Handle pathological cases */
	if (0 == num_sensors) {
		printf("No sensors to read!\n");
		return 0;
	}

	if (NULL == hub) {
		hub_pr_err("Invalid HUB handle\n");
		return 0;
	}

	if (NULL == p_temp) {
		hub_pr_err("NULL pointer sent as input for temperature values.\n");
		return 0;
	}

	if (NULL == p_sensor_ids) {
		hub_pr_err("NULL variable sent as input for sensor IDs.\n");
		return 0;
	}

	p_hub = (struct hub_ctx *)hub;

	/* Get the I2C bus context from HUB handle - explicitly search for I2C bus only */
	for (i = 0; i < p_hub->num_busses; i++) {
		if (HUB_GARD_BUS_I2C == p_hub->p_bus_props[i].types) {
			p_bus_ctx = &p_hub->p_bus_props[i];
			bus_hdl   = p_bus_ctx->i2c.bus_hdl;
			hub_pr_dbg("Found I2C bus (bus_num: %u, bus_hdl: %d) for temperature sensor reads\n",
					   p_bus_ctx->i2c.num, bus_hdl);
			break;
		}
	}

	if (NULL == p_bus_ctx) {
		hub_pr_err("I2C bus not found! Temperature sensors require I2C bus.\n");
		return 0;
	}

	/* Validate that the I2C bus handle is valid and open */
	if (bus_hdl < 0 || !p_bus_ctx->i2c.is_open) {
		hub_pr_err("I2C bus handle is invalid or not open (bus_hdl: %d, is_open: %d)\n",
				   bus_hdl, p_bus_ctx->i2c.is_open);
		return 0;
	}

	/* Lock the bus mutex before bus operations */
	hub_mutex_lock(&p_bus_ctx->bus_mutex);

	for (i = 0; i < num_sensors; i++) {
		// Initialize value to invalid
		p_temp[i] = HUB_INVALID_SENSOR_VALUE;
		slave_id = hub_get_temperature_sensor_i2c_slave_addr(p_sensor_ids[i]);
		if (HUB_INVALID_SENSOR_ID == slave_id) {
			hub_pr_err("Invalid temperature sensor ID [%d}]\n",
					   p_sensor_ids[i]);
			continue;
		}

		/* Set the sensor's slave address for the bus transaction */
		if (ioctl(bus_hdl, I2C_SLAVE, slave_id) < 0) {
			hub_pr_err("IOCTL for temp sensor (%d) failed\n", p_sensor_ids[i]);
			continue;
		}

		/* Write the register that we want to read */
		nwrite = p_bus_ctx->fops.device_write(bus_hdl, &reg_addr,
											  (sizeof(reg_addr)));
		if (sizeof(reg_addr) != nwrite) {
			hub_pr_err("Failed to write temp sensor [%d] register address\n",
					   p_sensor_ids[i]);
		}

		/* Read the value of the temperature register */
		nread = p_bus_ctx->fops.device_read(bus_hdl, (void *)&raw_temp,
											sizeof(raw_temp));
		if (sizeof(raw_temp) != nread) {
			hub_pr_err("Failed to read temperature from sensor [%d]",
					   p_sensor_ids[i]);
			p_temp[i] = HUB_INVALID_SENSOR_VALUE;
		} else {
			temp_int  = (raw_temp[0] << 8) | raw_temp[1];
			p_temp[i] = (float)(temp_int)*TMP118_MULTIPLIER;
			num_sensors_read++;
		}

		/* Restore the slave address on the bus to the original */
		if (ioctl(bus_hdl, I2C_SLAVE, p_bus_ctx->i2c.slave_id) < 0) {
			hub_pr_err("IOCTL for restoring slave address on bus %d failed\n",
					   bus_hdl);
		}
	}

	hub_mutex_unlock(&p_bus_ctx->bus_mutex);
	return num_sensors_read;
}

/**
 * Read the energy values (voltage, current and power) from the sensors
 * with given sensor IDs.
 *
 * Assumption: The sensor is attached on the same I2C bus as the GARD
 *
 * @param: hub is the HUB handle returned from a hub_init() call
 * @param: p_sensor_ids is an array of sensor IDs to read from
 * @param: p_values is an array of structures containing the energy values
 * (filled on read)
 * @param: num_sensors is the number of sensors to read from
 *
 * @return: uint32_t indicating how many sensors were successfully read
 */
uint32_t hub_get_energy_from_onboard_sensors(
	hub_handle_t                     hub,
	uint8_t                         *p_sensor_ids,
	struct hub_energy_sensor_values *p_values,
	uint8_t                          num_sensors)
{
	int             i;
	int             bus_hdl;
	ssize_t         nread, nwrite;
	uint8_t         slave_id;
	uint32_t        num_sensors_read = 0;

	struct hub_ctx *p_hub;

	uint8_t         reg_addr;
	uint16_t        temp_int;
	uint8_t         raw_temp[2];

	float           max_current = SOM_MAX_CURRENT;
	float           shunt_reg   = SOM_SHUNT_REG;
	float           current_lsb = max_current / INA236_CURRENT_DIV;

	uint16_t        calibration_value =
		(uint16_t)(INA236_CURRENT_MULTIPLIER / (current_lsb * shunt_reg));

	struct hub_gard_bus *p_bus_ctx = NULL;

	/* Handle pathological cases */
	if (0 == num_sensors) {
		printf("No sensors to read!\n");
		return 0;
	}

	if (NULL == hub) {
		hub_pr_err("Invalid HUB handle\n");
		return 0;
	}

	if (NULL == p_values) {
		hub_pr_err("NULL pointer sent as input for energy values.\n");
		return 0;
	}

	if (NULL == p_sensor_ids) {
		hub_pr_err("NULL variable sent as input for sensor IDs.\n");
		return 0;
	}

	p_hub = (struct hub_ctx *)hub;

	/* Get the I2C bus context from HUB handle - explicitly search for I2C bus only */
	for (i = 0; i < p_hub->num_busses; i++) {
		if (HUB_GARD_BUS_I2C == p_hub->p_bus_props[i].types) {
			p_bus_ctx = &p_hub->p_bus_props[i];
			bus_hdl   = p_bus_ctx->i2c.bus_hdl;
			hub_pr_dbg("Found I2C bus (bus_num: %u, bus_hdl: %d) for energy sensor reads\n",
					   p_bus_ctx->i2c.num, bus_hdl);
			break;
		}
	}

	if (NULL == p_bus_ctx) {
		hub_pr_err("I2C bus not found! Energy sensors require I2C bus.\n");
		return 0;
	}

	/* Validate that the I2C bus handle is valid and open */
	if (bus_hdl < 0 || !p_bus_ctx->i2c.is_open) {
		hub_pr_err("I2C bus handle is invalid or not open (bus_hdl: %d, is_open: %d)\n",
				   bus_hdl, p_bus_ctx->i2c.is_open);
		return 0;
	}

	/* Lock the bus mutex before bus operations */
	hub_mutex_lock(&p_bus_ctx->bus_mutex);

	for (i = 0; i < num_sensors; i++) {
		slave_id = hub_get_energy_sensor_i2c_slave_addr(p_sensor_ids[i]);
		// Initialize all values to invalid
		p_values[i] = (struct hub_energy_sensor_values){
			.voltage = HUB_INVALID_SENSOR_VALUE,
			.current = HUB_INVALID_SENSOR_VALUE,
			.power   = HUB_INVALID_SENSOR_VALUE
		};

		if (HUB_INVALID_SENSOR_ID == slave_id) {
			hub_pr_err("Invalid energy sensor ID [%d}]\n", p_sensor_ids[i]);
			continue;
		}

		/* Set the sensor's slave address for the bus transaction */
		if (ioctl(bus_hdl, I2C_SLAVE, slave_id) < 0) {
			hub_pr_err("IOCTL for energy sensor (%d) failed\n",
					   p_sensor_ids[i]);
			continue;
		}

		/**
		 * The INA236 needs to be calibrated before reading the current and
		 * power.
		 *
		 * Here, we set up the calibration register.
		 * This requires the master to send the calib register address (8-bit),
		 * followed by the calibration data (16-bit) as a single write.
		 */
		uint8_t calib_buffer[3] = {INA236_REG_CALIB, calibration_value >> 8,
								   calibration_value & 0xFF};

		nwrite = p_bus_ctx->fops.device_write(bus_hdl, calib_buffer,
											  (sizeof(calib_buffer)));
		if (sizeof(calib_buffer) != nwrite) {
			hub_pr_err("Failed to write calib sensor register\n");
		}

		/* Read the current from the current register */
		reg_addr = INA236_REG_CURRENT;

		/* Send the current register address */
		nwrite   = p_bus_ctx->fops.device_write(bus_hdl, &reg_addr,
												(sizeof(reg_addr)));
		if (sizeof(reg_addr) != nwrite) {
			hub_pr_err("Failed to write calib sensor register\n");
		}

		/* Read the 16-bit raw value */
		nread = p_bus_ctx->fops.device_read(bus_hdl, (void *)&raw_temp,
											sizeof(raw_temp));
		if (sizeof(raw_temp) != nread) {
			hub_pr_err("Failed to read current from sensor[%d]",
					   p_sensor_ids[i]);
			p_values[i].current = HUB_INVALID_SENSOR_VALUE;
		} else {
			/* Convert it to the floating point representation */
			temp_int            = (raw_temp[0] << 8) | raw_temp[1];
			p_values[i].current = temp_int * current_lsb;
		}

		/* Read the voltage from the voltage register */
		reg_addr = INA236_REG_BUS_VOLTAGE;

		/* Send the current register address */
		nwrite   = p_bus_ctx->fops.device_write(bus_hdl, &reg_addr,
												(sizeof(reg_addr)));
		if (sizeof(reg_addr) != nwrite) {
			hub_pr_err("Failed to write voltage register[%d]", p_sensor_ids[i]);
		}

		/* Read the 16-bit raw value */
		nread = p_bus_ctx->fops.device_read(bus_hdl, (void *)&raw_temp,
											sizeof(raw_temp));
		if (sizeof(raw_temp) != nread) {
			hub_pr_err("Failed to read voltage from sensor[%d]",
					   p_sensor_ids[i]);
			p_values[i].voltage = HUB_INVALID_SENSOR_VALUE;
		} else {
			/* Convert it to the floating point representation */
			temp_int            = (raw_temp[0] << 8) | raw_temp[1];
			p_values[i].voltage = temp_int * INA236_VOLTAGE_MULTIPLIER;
		}

		/* Read the power from the power register */
		reg_addr = INA236_REG_POWER;

		/* Send the current register address */
		nwrite   = p_bus_ctx->fops.device_write(bus_hdl, &reg_addr,
												(sizeof(reg_addr)));
		if (sizeof(reg_addr) != nwrite) {
			hub_pr_err("Failed to write power sensor[%d]", p_sensor_ids[i]);
		}

		/* Read the 16-bit raw value */
		nread = p_bus_ctx->fops.device_read(bus_hdl, (void *)&raw_temp,
											sizeof(raw_temp));
		if (sizeof(raw_temp) != nread) {
			hub_pr_err("Failed to read power from sensor[%d]", p_sensor_ids[i]);
			p_values[i].power = HUB_INVALID_SENSOR_VALUE;
		} else {
			/* Convert it to the floating point representation */
			temp_int = (raw_temp[0] << 8) | raw_temp[1];
			p_values[i].power =
				temp_int * INA236_POWER_MULTIPLIER * current_lsb;
		}

		/* Restore the slave address on the bus to the original */
		if (ioctl(bus_hdl, I2C_SLAVE, p_bus_ctx->i2c.slave_id) < 0) {
			hub_pr_err("IOCTL for restoring slave address on bus %d failed\n",
					   bus_hdl);
		}

		num_sensors_read++;
	}

	hub_mutex_unlock(&p_bus_ctx->bus_mutex);
	return num_sensors_read;
}
