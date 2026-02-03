#      Copyright(c) 2025 Mirametrix Inc. All rights reserved.
#
#      These coded instructions, statements, and computer programs contain
#      unpublished proprietary information written by Mirametrix and
#      are protected by copyright law. They may not be disclosed
#      to third parties or copied or duplicated in any form, in whole or
#      in part, without the prior written consent of Mirametrix.
#

#Example usage of the pure C wrapper library for ctypes

from ctypes import *

# Load the shared library
lib = CDLL("./libi2c_lib.so")


# Define the handle type
I2CDeviceHandle = c_void_p

# Define function signatures
lib.i2c_device_create.argtypes = [c_uint8, c_uint8]
lib.i2c_device_create.restype = I2CDeviceHandle

lib.i2c_device_destroy.argtypes = [I2CDeviceHandle]

lib.i2c_write_to_device.argtypes = [I2CDeviceHandle, POINTER(c_uint8), c_uint32]
lib.i2c_write_to_device.restype = c_int32

lib.i2c_read_from_device.argtypes = [I2CDeviceHandle, POINTER(c_uint8), c_uint32]
lib.i2c_read_from_device.restype = c_int32

# Create device
device = lib.i2c_device_create(1, 0x30)

# # Prepare data to write
# write_data = (c_uint8 * 4)(10, 20, 30, 40)
# write_len = 4

# # Write to device
# result = lib.i2c_write_to_device(device, write_data, write_len)
# print(f"Write result: {result}")

# Prepare buffer to read into
read_buffer = (c_uint8 * 1000)()
read_len = 1000

# Read from device
while True:
    read_count = lib.i2c_read_from_device(device, read_buffer, read_len)
    if read_count > 0:
        print(f"Read {read_count} values: {[read_buffer[i] for i in range(read_count)]}")

# Clean up
lib.i2c_device_destroy(device)

    