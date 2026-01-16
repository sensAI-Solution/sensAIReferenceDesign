# /******************************************************************************
#  * Copyright (c) 2025 Lattice Semiconductor Corporation
#  *
#  * SPDX-License-Identifier: UNLICENSED
#  *
#  ******************************************************************************/

"""
A PY test application of HUB with GARD usage

Description:

Uses HUB class demonstrating it's usage and

1. read write operations over registers with buffers
2. send receive bulk data
3. read temperature and energy sensors' data

with data verity test

Usage:

1. Create a virtual Environment of Python
2. pip install .whl hub package
3. go hub_apps directory
4. run "python app.py"
"""

import os
import struct

try:
    import hub
except ImportError:
    print(
        "\nHub library is not installed. \nHave you installed HUB in activated Python environment? Use HUB wheel package to install lscc-hub.\n"
    )
    exit()


"""
A helper function to find a mismatched index of two bytearrays
Returns the index if matched else -1

@param: b1 & b2 (bytearray) - bytearray 1 and bytearray 2
@returns: index(int) - integer or -1
"""


def find_mismatch_index(b1: bytearray, b2: bytearray) -> int:
    min_len = min(len(b1), len(b2))
    for i in range(min_len):
        if b1[i] != b2[i]:
            return i
    if len(b1) != len(b2):
        return min_len
    return -1


"""
Implementation of Application

Majorly it is divided into following sections

1.  Variable declarations
2.  HUB & GARD instantiation
    This involves -
    - Creating hub instance with default files
    - Setuping hub with a HUB object (preinit-discovery-init)
    - Finding active GARD count
    - Creating a GARD object for given GARD number
    - Creating logger instance
3.  Register Operations
    This involves -
    - Creating a random buffer of data size
    - Writing specified registers with created buffer
    - Reading from same registers 
    - Comparing if data has matched - data verity test
4.  Data Operations
    This involves -
    - Using same previously created random buffer
    - Sending bulk data over data bus at given address
    - Reading bulk data from same address
    - Comparing if the data is matched - data verity test
5.  Sensor Interfacing
    This involves - 
    - reading both temperature sensors' data on SOM
    - reading both energy sensors' data - power, voltage, current on CM5 & SOM
"""

def print_sensor_data(hub_instance, sensor_type, sensors, values):
    if sensor_type == "temp":
        hub_instance.logger.info(
            "Temperature read from {} sensor(s)".format(len(sensors))
            )
        
        for index, sensor in enumerate(sensors):
            output_str = "Sensor : {} >> Temperature : {} °C".format(
                sensor,
                values[index] if values[index] else "NA"
            )
            hub_instance.logger.info(output_str)
    else:
        hub_instance.logger.info(
            "Energy values read from {} sensor(s)".format(len(sensors))
        )

        for index, sensor in enumerate(sensors):           
            output_str = "Sensor : {} >> Voltage : {} V, Current : {} A, Power : {} W".format(
                sensor,
                f'{values[index]["voltage"]:.4f}' if (values[index]["voltage"] is not None) else "NA",
                f'{values[index]["current"]:.4f}' if (values[index]["current"] is not None) else "NA",
                f'{values[index]["power"]:.4f}' if (values[index]["power"] is not None) else "NA",
            )
            hub_instance.logger.info(output_str)


def main():
    # SECTION 1 : VARIABLE DECLARATIONS

    # Please change base address in case ML pipeline is operational 
    # from HRAM since HRAM is being consumed by ML engine.
    addr = 0x0C000000
    read_chunk = bytearray()
    data_size = 1024
    num_bytes = data_size * 4

    # SECTION 2: HUB & GARD INSTANTIATION
    # create hub instance
    hub_instance = hub.HUB(None, None)
    hub_instance.logger.info("Welcome to H.U.B v{}".format(hub_instance.version))

    gard_num = hub_instance.get_gard_count()
    if not gard_num:
        hub_instance.logger.info("No GARDs discovered, cannot continue!")
        return
    else:
        hub_instance.logger.info(f"{gard_num} GARD(s) discovered")

    # get gard instance
    # Since we have only 1 gard, currently passing gard0.
    # In furture there will be support for multiple gards connected to hub.
    gard_num = 0
    gard0 = hub_instance.get_gard(gard_num)
    if gard0:
        hub_instance.logger.info(
            f"Gard is acquired! Gard {gard_num} is selected for further operations."
        )
    else:
        return

    # SECTION 3: REGISTER OPERATIONS
    hub_instance.logger.info(f"--------------------------------------")
    hub_instance.logger.info(f"Read Write Operations of Buffer on Register")
    hub_instance.logger.info(f"--------------------------------------")

    # write register handling
    write_buffer = bytearray(os.urandom(num_bytes))
    write_chunk = list(struct.unpack(f"<{data_size}I", write_buffer))

    hub_instance.logger.info(f"Writing to registers for data size of {data_size}")
    for i in range(data_size):
        retcode = gard0.write_register(addr, write_chunk[i])
        if retcode:
            hub_instance.logger.warning(
                f"Writing to register failed with error code {retcode}. Not writing further"
            )
            break
        addr += 4

    hub_instance.logger.info(f"Reading from registers for data size of {data_size}")

    # read register handling
    # register addresses should be 4 aligned
    addr = 0x0C000000
    read_chunk = []
    for i in range(data_size):
        retcode, read_val = gard0.read_register(addr)
        if retcode:
            hub_instance.logger.warning(
                f"Reading from a register failed with error code {retcode}. Not reading further"
            )
            break
        read_chunk.append(read_val)
        addr += 4

    # data verity test
    hub_instance.logger.info("")
    hub_instance.logger.info("REG-WRITE and REG-READ Data Verity test: ")
    if write_chunk == read_chunk:
        hub_instance.logger.info(
            f"Written and read buffer values of size {len(write_chunk)} are matched!"
        )
    else:
        miss_idx = find_mismatch_index(write_chunk, read_chunk)
        hub_instance.logger.critical(
            f"Written and read buffer values didn't match! \n"
            f"Index : {miss_idx}, Write : 0x{(write_chunk[miss_idx]):x}, Read : 0x{(read_chunk[miss_idx]):x}"
        )

    # SECTION 4: DATA OPERATIONS
    # Read and writing data
    hub_instance.logger.info(f"--------------------------------------")
    hub_instance.logger.info(f"Read Write Operations of Bulk/Buffer Data")
    hub_instance.logger.info(f"--------------------------------------")

    # write data
    # register addresses should be 4 aligned
    addr = 0x0C000000
    hub_instance.logger.info(f"Writing bulk data for data size of {len(write_buffer)}")
    retcode = gard0.send_data(addr, write_buffer, len(write_buffer))
    if retcode:
        hub_instance.logger.warning(
            f"Failed to write bulk data with error code {retcode}"
        )

    # read data
    hub_instance.logger.info(f"Reading bulk data for data size of {len(write_buffer)}")
    retcode, read_buffer = gard0.receive_data(addr, len(write_buffer))
    if retcode:
        hub_instance.logger.warning(
            f"Reading bulk data failed with error code {retcode}"
        )

    # data verity test
    hub_instance.logger.info("")
    hub_instance.logger.info("DATA-SEND and DATA-RECV Data Verity test:")
    if write_buffer == read_buffer:
        hub_instance.logger.info(
            f"Written and read buffer values of size {len(write_buffer)} are matched!"
        )
    else:
        miss_idx = find_mismatch_index(write_buffer, read_buffer)
        hub_instance.logger.critical(
            f"Written and read buffer values didn't match! \n"
            f"Index : {miss_idx}, Write : 0x{(write_buffer[miss_idx]):x}, Read : 0x{(read_buffer[miss_idx]):x}"
        )

    # SECTION 5 : SENSORS INTERFACING
    hub_instance.logger.info(f"--------------------------------------")
    hub_instance.logger.info(f"Read Temperature Sensors Data")
    hub_instance.logger.info(f"--------------------------------------")
    sensors = [3, 1, 2, 2, 3, 1]
    data = hub_instance.get_temperature_data(sensors)
    print_sensor_data(hub_instance, "temp", sensors, data)

    hub_instance.logger.info(f"--------------------------------------")
    hub_instance.logger.info(f"Read Energy Sensors Data")
    hub_instance.logger.info(f"--------------------------------------")
    sensors = [3, 1, 2, 2, 3, 1]
    data = hub_instance.get_energy_data(sensors)
    print_sensor_data(hub_instance, "energy", sensors, data)

    hub_instance.logger.info(f"--------------------------------------")


if __name__ == "__main__":
    main()
