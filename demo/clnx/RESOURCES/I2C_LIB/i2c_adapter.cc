#include "i2c_adapter.h"
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <iostream>
#include <thread>
#include <iomanip> // Required for std::put_time

// #include "stdbool.h"

extern "C"
{
#include <unistd.h>        //Needed for I2C port
#include <fcntl.h>         //Needed for I2C port
#include <sys/ioctl.h>     //Needed for I2C port
#include <linux/i2c-dev.h> //Needed for I2C port
#include <i2c/smbus.h>
#include "i2c_intr_gpio.h"
}

namespace
{
    constexpr uint32_t SMBUS_BLOCK_LEN = 32;

    typedef enum
    {
        ISH_INT_HP = 0,
        ISH_INT_FID,
        ISH_INT_ALS,
        ISH_INT_DATA_TRANSMISSION
    } IshInterruptType;

    typedef struct
    {
        uint8_t sum0; // value of sum0 variable so far
        uint8_t sum1; // value of sum1 variable so far
        size_t size;  // Number of bytes processed
    } fletcher_checksum_t;

    //-----------------------------------------------------------------------------
    // Initialize a fletcher_checksum_t variable.
    fletcher_checksum_t CreateFletcherChecksum()
    {
        fletcher_checksum_t checksum = {
            .sum0 = 0,
            .sum1 = 0,
            .size = 0};
        return checksum;
    }

    //-----------------------------------------------------------------------------
    // Compute the 16 bytes Fletcher's checkum from the partial computation results
    // stored in checksum.
    int16_t FletcherCheckSumAsInt16(fletcher_checksum_t checksum)
    {
        return checksum.sum0 + checksum.sum1 * 256;
    }

    fletcher_checksum_t AddInt32ToChecksum(
        fletcher_checksum_t checksum, int32_t data)
    {
        uint8_t *ptr = (uint8_t *)&data;

        for (size_t i = 0; i < 4; ++i)
        {
            checksum.sum0 = (checksum.sum0 + ptr[i]) % 255;
            checksum.sum1 = (checksum.sum1 + checksum.sum0) % 255;
        }

        checksum.size += 4;
        return checksum;
    }

    fletcher_checksum_t AddInt32ArrayToChecksum(
        fletcher_checksum_t checksum, const int32_t *data, size_t size)
    {
        for (size_t i = 0; i < size; ++i)
        {
            checksum = AddInt32ToChecksum(checksum, data[i]);
        }
        return checksum;
    }

    constexpr uint32_t D2H_PAGES[] = {4, 5};
    constexpr uint32_t H2D_PAGES[] = {2, 3};
    constexpr uint32_t DEBUG_PAGES[] = {1};
    constexpr uint32_t ISH_BLOCK_LEN = 0x10;
    constexpr uint32_t ISH_DATA_SIZE = ISH_BLOCK_LEN * sizeof(D2H_PAGES) / sizeof(uint32_t);
    constexpr uint32_t ISH_BLOCK_OFFSET = 0x10;

    struct som_i2c_packet_t
    {
        uint8_t data[(ISH_DATA_SIZE - 1) * 4];
        uint8_t idx;
        uint8_t length;
        int16_t checksum;
    };

    int16_t get_checksum(som_i2c_packet_t packet)
    {
        fletcher_checksum_t checksum = CreateFletcherChecksum();
        checksum = AddInt32ToChecksum(checksum, packet.idx);
        checksum = AddInt32ToChecksum(checksum, packet.length);
        checksum = AddInt32ArrayToChecksum(checksum, (int32_t *)packet.data, ((packet.length + 3) & ~3) / 4);
        return FletcherCheckSumAsInt16(checksum);
    }
}

void I2CDevice::init(uint8_t adapter_number, uint8_t device_number, uint8_t irq_pin)
{
    char filename[20];

    snprintf(filename, 19, "/dev/i2c-%d", adapter_number);
    _file = open(filename, O_RDWR);
    if (_file < 0)
    {
        throw std::runtime_error("failed to open i2c\n");
    }
    if (ioctl(_file, I2C_SLAVE, device_number) < 0)
    {
        /* ERROR HANDLING; you can check errno to see what went wrong */
        throw std::runtime_error("failed to ioctl i2c\n");
    }

    init_i2c_intr_gpio(irq_pin);
    clear_device_interrupt();
}

I2CDevice::I2CDevice(uint8_t adapter_number, uint8_t device_number, uint8_t irq_pin)
{
	init(adapter_number, device_number, irq_pin);
}

I2CDevice::I2CDevice(uint8_t adapter_number, uint8_t device_number)
{
	// default IRQ pin
	init(adapter_number, device_number, 17);
}

int32_t I2CDevice::write_data(uint8_t page, uint8_t address, uint8_t *data, uint32_t length)
{
    // select page
    i2c_smbus_write_byte_data(_file, 0x4, page);
    uint32_t blocks = length / SMBUS_BLOCK_LEN;
    uint32_t bytes_written = 0;

    for (uint32_t i = 0; i < blocks; ++i)
    {
        bytes_written += i2c_smbus_write_i2c_block_data(_file, address + i * SMBUS_BLOCK_LEN, SMBUS_BLOCK_LEN, data + i * SMBUS_BLOCK_LEN);
    }
    if (length % SMBUS_BLOCK_LEN != 0)
    {
        bytes_written += i2c_smbus_write_i2c_block_data(_file, address + SMBUS_BLOCK_LEN * blocks, length % SMBUS_BLOCK_LEN, data + SMBUS_BLOCK_LEN * blocks);
    }

    return bytes_written;
}

int32_t I2CDevice::read_data(uint8_t page, uint8_t address, uint8_t *data, uint32_t length)
{
    // select page
    uint8_t ret = i2c_smbus_write_byte_data(_file, 0x4, page);
    uint8_t expected_page = i2c_smbus_read_byte_data(_file, 0x4);

    uint32_t blocks = length / SMBUS_BLOCK_LEN;
    uint32_t bytes_read = 0;

    for (uint32_t i = 0; i < length / SMBUS_BLOCK_LEN; ++i)
    {
        bytes_read += i2c_smbus_read_i2c_block_data(_file, address + i * SMBUS_BLOCK_LEN, SMBUS_BLOCK_LEN, data + i * SMBUS_BLOCK_LEN);
    }
    if (length % SMBUS_BLOCK_LEN != 0)
    {
        bytes_read += i2c_smbus_read_i2c_block_data(_file, address + blocks * SMBUS_BLOCK_LEN, length % SMBUS_BLOCK_LEN, data + blocks * SMBUS_BLOCK_LEN);
    }
    return bytes_read;
}

I2CDevice::interrupt_register_t I2CDevice::read_device_interrupt()
{
    uint8_t ret;
    uint8_t interrupt_val = 0;
    bool interrupt_ready = false;

    if (is_i2c_intr_asserted())
    {
        ret = i2c_smbus_write_byte_data(_file, 0x4, 0);
        interrupt_val = i2c_smbus_read_byte_data(_file, 0x3);
        interrupt_ready = interrupt_val & 1;
        interrupt_val = interrupt_val >> 1 & 0x7f;
    }

    return {.interrupt_val = interrupt_val, .interrupt_ready = interrupt_ready};
}

void I2CDevice::write_device_interrupt(uint8_t interrupt)
{
    uint8_t ret = i2c_smbus_write_byte_data(_file, 0x4, 0);

    uint8_t interrupt_val = i2c_smbus_write_byte_data(_file, 0x2, interrupt);
}

void I2CDevice::clear_device_interrupt()
{
    clear_i2c_intr_asserted();

    uint8_t ret = i2c_smbus_write_byte_data(_file, 0x4, 0);

    uint8_t interrupt_val = i2c_smbus_write_byte_data(_file, 0x3, 1);
}

std::vector<uint8_t> I2CDevice::read_from_device()
{
    std::vector<uint8_t> data;
    auto read_packet = [&]()
    {
        som_i2c_packet_t packet;
        for (int i = 0; i < sizeof(D2H_PAGES) / sizeof(D2H_PAGES[0]); ++i){
            read_data(D2H_PAGES[i], 0x10, (uint8_t *)&packet + i * ISH_BLOCK_LEN * 4, ISH_BLOCK_LEN * 4);
        }
        return packet;
    };

    interrupt_register_t interrupt = read_device_interrupt();

    if (!interrupt.interrupt_ready)
    {
        return data;
    }
    if (interrupt.interrupt_val != ISH_INT_DATA_TRANSMISSION)
    {
        return data; // wrong interrupt value - how did that happen?
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2)); // wait for page to refresh

    uint32_t packet_idx = 0xffff;
    while (packet_idx != 1)
    {
        som_i2c_packet_t packet = read_packet();

        int16_t expected_checksum = packet.checksum;

        int16_t calculated_checksum = get_checksum(packet);
        if (expected_checksum != calculated_checksum)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        else
        {
            if (packet.idx == packet_idx)
            {
                // page didn't update yet, this is the old packet
                continue;
            }
            std::chrono::steady_clock::time_point ack_time = std::chrono::steady_clock::now();

            // write checksum into debug reg to acknowledge
            uint32_t checksum_4bytes = calculated_checksum;
            write_data(0x1, 0x10, (uint8_t *)&checksum_4bytes, 4);
        }

        packet_idx = packet.idx;
        for (int i = 0; i < packet.length; ++i){
        }
        data.insert(data.end(), packet.data, packet.data + packet.length);
		
		//clear interrupt
		clear_device_interrupt();
		
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // wait for page to refresh
    }


    return data;
}

int32_t I2CDevice::write_to_device(uint8_t *data, uint32_t length)
{
    // first block write
    som_i2c_packet_t packet;

    auto write_packet_pages = [&](som_i2c_packet_t packet)
    {
        uint8_t *packet_data = (uint8_t *)&packet;
        for (int i = 0; i < sizeof(H2D_PAGES) / sizeof(H2D_PAGES[0]); ++i){
            write_data(H2D_PAGES[i], 0x10, packet_data + i * ISH_BLOCK_LEN * 4, ISH_BLOCK_LEN * 4);
        }
    };

    auto send_packet = [&packet, write_packet_pages](uint8_t *data, uint32_t length)
    {
        bool partial_last_packet = length % (sizeof(packet.data)) != 0;
        packet.idx = length / (sizeof(packet.data));
        if (partial_last_packet)
        {
            ++packet.idx;
        }

        if (length > sizeof(packet.data))
        {
            packet.length = sizeof(packet.data);
        }
        else
        {
            packet.length = length;
        }
        for (size_t i = 0; i < packet.length; ++i)
        {
            packet.data[i] = data[i];
        }
        packet.checksum = get_checksum(packet);
        write_packet_pages(packet);
        return packet.length;
    };

    // first block:
    uint32_t data_sent = send_packet(data, length);
    data = data + data_sent;
    length = length - data_sent;
    // trigger interrupt to notify device that data is waiting
    write_device_interrupt(ISH_INT_DATA_TRANSMISSION << 1 | 0x1);
    std::this_thread::sleep_for(std::chrono::milliseconds(2)); // wait for page to refresh

    auto read_checksum_page = [&]()
    {
        uint32_t buf;

        read_data(1, 0x10, (uint8_t *)&buf, 4);
        return (int16_t)buf;
    };

    while (read_checksum_page() != packet.checksum)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2)); // wait for page to refresh
    }

    // enter loop to send the remaining data
    while (length > 0)
    {
        data_sent = send_packet(data, length);
        data = data + data_sent;
        length = length - data_sent;
        while (read_checksum_page() != packet.checksum)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // wait for page to refresh
        }
    }
    // clear interrupt now that we're done
    write_device_interrupt(0x0);

    return length;
}

#if 0 // For testing purposes
int main()
{
    I2CDevice dev = I2CDevice(1, 0x30);

    // write some data to the device and read it back
    char buf[] = "The Raspberry Pi is a single board computer with now 4 revisions and a minimalistic zero variant. It is a popular choice for different projects because of its small size, efficient power consumption, processing speed and by being a full Linux based computer.\n\nOne way to connect multiple single-board computers and/or microcontrollers is direct wiring. For this purpose, the most common used protocols are I2C, SPI and UART. Preceding articles in the blog series explained the principles of these protocols, and presented specific C-libraries for the Arduino. In this article, I will explain C++ libraries that enable to work with these protocols on a Raspberry Pi. For each protocol, I researched useable libraries, and give a short explanation and code example. Please not that the examples are not developed by me, but come from the libraries documentation, and should serve as a basis for a concrete working example.\n\nThis article originally appeared at my blog admantium.com.\n\nI2C\n\nI2C can be supported with the help of the SMBus protocol, which is described as a specific variant of the I2C bus. This protocol is available as Linux Kernel module. To use it, you need to configure your Raspberry Pi. In a terminal, run raspi-config, select 3 Interfacing Options and P5 I2C.\n\nFollowing the code example from kernel.org, you will need to open the device file that represents the connected I2C device, and then send SMBus commands by writing to the devices registers.";
    uint8_t *buf_ptr = (uint8_t *)buf;
    uint32_t len = sizeof(buf);

    // dev.write_to_device(buf_ptr, len);
    while (true)
    {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        auto data = dev.read_from_device();
        if (data.size() == 0){ continue;;}
        for (uint8_t ch : data)
        {
            printf("%d ", ch);
            if (ch == 0)
            {
                //std::cout << '0';
            }
            // for (int i = 0; i < 4; ++i)
            // {
            //     char ch = static_cast<char>((val >> (i * 8)) & 0xFF);
            //     if (ch == '\0')
            //         //std::cout << "0"; // Stop at null terminator if present
            //     //std::cout << ch;
            // }
        }
        std::cout << "\n";
        printf("data length is %d bytes\n", data.size());
        std::cout << "time for data: " << std::chrono::duration_cast<std::chrono::milliseconds>(start - std::chrono::steady_clock::now()).count() << "ms\n";

    }
}
#endif
