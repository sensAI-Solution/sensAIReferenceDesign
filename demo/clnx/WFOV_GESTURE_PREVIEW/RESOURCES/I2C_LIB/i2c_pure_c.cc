//=============================================================================
//
// Copyright(c) 2025 Mirametrix Inc. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by MMX and
// are protected by copyright law. They may not be disclosed
// to third parties or copied or duplicated in any form, in whole or
// in part, without the prior written consent of MMX.
//
//=============================================================================
#include "i2c_pure_c.h"
#include "i2c_adapter.h"
#include <vector>

extern "C" {

I2CDeviceHandle i2c_device_create(uint8_t adapter_number, uint8_t device_number) {
    return new I2CDevice(adapter_number, device_number);
}

void i2c_device_destroy(I2CDeviceHandle handle) {
    delete static_cast<I2CDevice*>(handle);
}

int32_t i2c_write_data(I2CDeviceHandle handle, uint8_t page, uint8_t address, uint8_t* data, uint32_t length) {
    return static_cast<I2CDevice*>(handle)->write_data(page, address, data, length);
}

int32_t i2c_read_data(I2CDeviceHandle handle, uint8_t page, uint8_t address, uint8_t* data, uint32_t length) {
    return static_cast<I2CDevice*>(handle)->read_data(page, address, data, length);
}

int32_t i2c_write_to_device(I2CDeviceHandle handle, uint8_t* data, uint32_t length) {
    return static_cast<I2CDevice*>(handle)->write_to_device(data, length);
}

int32_t i2c_read_from_device(I2CDeviceHandle handle, uint8_t* out_data, uint32_t max_length) {
    std::vector<uint8_t> result = static_cast<I2CDevice*>(handle)->read_from_device();
    uint32_t count = (result.size() < max_length) ? result.size() : max_length;
    for (uint32_t i = 0; i < count; ++i) {
        out_data[i] = result[i];
    }
    return count;
}

interrupt_register_t i2c_read_device_interrupt(I2CDeviceHandle handle) {
    auto cpp_result = static_cast<I2CDevice*>(handle)->read_device_interrupt();
    interrupt_register_t c_result;
    c_result.interrupt_val = cpp_result.interrupt_val;
    c_result.interrupt_ready = cpp_result.interrupt_ready;
    return c_result;
}

void i2c_write_device_interrupt(I2CDeviceHandle handle, uint8_t interrupt) {
    static_cast<I2CDevice*>(handle)->write_device_interrupt(interrupt);
}

}
