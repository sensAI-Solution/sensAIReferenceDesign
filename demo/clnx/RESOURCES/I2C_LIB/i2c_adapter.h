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
#include <cstdint>
#include <vector>

class I2CDevice
{
public:
    I2CDevice(uint8_t adapter_number, uint8_t device_number);
    I2CDevice(uint8_t adapter_number, uint8_t device_number, uint8_t irq_pin);

    int32_t write_data(uint8_t page, uint8_t address, uint8_t *data, uint32_t length);

    int32_t read_data(uint8_t page, uint8_t address, uint8_t *data, uint32_t length);

    struct interrupt_register_t
    {
        uint8_t interrupt_val;
        bool interrupt_ready;
    };

    int32_t write_to_device(uint8_t *data, uint32_t length); // bytes_view or something if it exists
    std::vector<uint8_t> read_from_device(); //no buffer length needed if we return std::vector

    interrupt_register_t read_device_interrupt();
    void write_device_interrupt(uint8_t interrupt);
    void clear_device_interrupt();

private:
    void init(uint8_t adapter_number, uint8_t device_number, uint8_t irq_pin);

    int _file;
};
