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

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>

    typedef void *I2CDeviceHandle;

    I2CDeviceHandle i2c_device_create(uint8_t adapter_number, uint8_t device_number);
    void i2c_device_destroy(I2CDeviceHandle handle);

    int32_t i2c_write_data(I2CDeviceHandle handle, uint8_t page, uint8_t address, uint8_t *data, uint32_t length);
    int32_t i2c_read_data(I2CDeviceHandle handle, uint8_t page, uint8_t address, uint8_t *data, uint32_t length);

    int32_t i2c_write_to_device(I2CDeviceHandle handle, uint8_t* data, uint32_t length);
    int32_t i2c_read_from_device(I2CDeviceHandle handle, uint8_t* out_data, uint32_t max_length);

    typedef struct
    {
        uint8_t interrupt_val;
        bool interrupt_ready;
    } interrupt_register_t;

    interrupt_register_t i2c_read_device_interrupt(I2CDeviceHandle handle);
    void i2c_write_device_interrupt(I2CDeviceHandle handle, uint8_t interrupt);

#ifdef __cplusplus
}
#endif
