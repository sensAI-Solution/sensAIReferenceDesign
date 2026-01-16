# HUB Python Library

This module provides Python bindings for the H.U.B. (Host-accelerated Unified Bridge) C library, enabling Python applications to interact with GARD devices and sensors via the HUB infrastructure.

## Features

-   **HUB Exposure**: Automatically discovers and initializes GARD devices exposing HUB library with abstraction and object-oriented approach.
-   **GARD Access**: Retrieve GARD objects and interact with their registers and data buses.
-   **Sensor Data**: Read temperature and energy sensor data from onboard sensors.
-   **Robust Error Handling**: Custom exceptions for initialization, version mismatch, and library loading errors.
-   **Logging**: Configurable logging with optional colorized output.

## Main Classes

### HUB
-   Initializes the HUB and discovers attached GARDs.
-   Loads the required shared libraries (`libhub.so`, `libcjson.so`, `libusb-1.0.so`).
-   Provides methods:
    -   `get_gard_count()`: Returns the number of connected GARDs.
    -   `get_gard(gard_num)`: Returns a `GARD` object for the specified GARD number.
    -   `get_temperature_data(sensor_ids)`: Reads temperature data for given sensor IDs.
    -   `get_energy_data(sensor_ids)`: Reads energy data (voltage, current, power) for given sensor IDs.

### GARD
-   Represents a single GARD device.
-   Provides methods:
    -   `write_register(addr, value)`: Writes a value to a register.
    -   `read_register(addr)`: Reads a value from a register.
    -   `send_data(addr, data, size)`: Sends bulk data to the device.
    -   `receive_data(addr, size)`: Receives bulk data from the device.

## Exception Classes

-   `GardNotFoundError`
-   `GardObjectsExistsError`
-   `LoadSOFileError`
-   `InitFailureError`
-   `VersionMismatchError`

## Usage

-   Refer to [Python example](src/hub/examples/py_example) for sample usage.
-   Refer to [app.py](src/hub/hub_apps/edgeHUB/app.py) for extensive usage.
