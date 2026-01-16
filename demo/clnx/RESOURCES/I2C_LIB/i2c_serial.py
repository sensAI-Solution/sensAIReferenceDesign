#      Copyright(c) 2025 Mirametrix Inc. All rights reserved.
#
#      These coded instructions, statements, and computer programs contain
#      unpublished proprietary information written by Mirametrix and
#      are protected by copyright law. They may not be disclosed
#      to third parties or copied or duplicated in any form, in whole or
#      in part, without the prior written consent of Mirametrix.
#


from ctypes import CDLL, POINTER, c_int32, c_uint32, c_uint8, c_void_p
import queue
import threading


class I2CSerial():
    """Implements the Serial() interface but using I2C behind the scenes."""

    def __i2c_operations_thread(read_queue, write_queue, kill_thread):
        """Function to handle receiving and transmitting data over I2C"""

        lib = CDLL("./python/standalone/i2c_adapter/rpi_i2c/libi2c_lib.so")

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
        device = lib.i2c_device_create(1, 0x30)
        read_buffer = (c_uint8 * 1000)()
        read_len = 1000

        # Start loop, read and write data forever
        while True:
            if (kill_thread.is_set()):
                return
            
            #try to read data
            data_read = lib.i2c_read_from_device(device, read_buffer,read_len)

            #try to write after we've finished reading so host and device aren't both writing at once
            if not write_queue.qsize() == 0:
                #data to write
                data_to_write = [write_queue.get() for _ in range(write_queue.qsize())]
                data_array = (c_uint8 * len(data_to_write))(*data_to_write)
                lib.i2c_write_to_device(device, data_array, len(data_to_write))

            if (data_read != 0):
                for i in range(data_read):
                    read_queue.put(read_buffer[i])


    def __init__(self):
        self.data_buf = []
        self.read_queue = queue.Queue()
        self.write_queue = queue.Queue()
        self.kill_thread = threading.Event()
        self.i2c_operations_thread = threading.Thread(target=I2CSerial.__i2c_operations_thread, 
                                                      args=(self.read_queue, self.write_queue, self.kill_thread))
        self.i2c_operations_thread.start()

    def read(self, length_to_read):
        data = [self.read_queue.get() for _ in range(length_to_read)]
        return bytes(data)
    
    def write(self, data: bytes):
        for datum in data:
            self.write_queue.put(datum)
    
    @property
    def in_waiting(self):
        return self.read_queue.qsize()
    
    def reset_input_buffer(self):
        with self.read_queue.mutex:
            self.read_queue.queue.clear()
    
    def close(self):
        if not self.kill_thread.is_set():
            self.kill_thread.set()
            self.i2c_operations_thread.join()

    def __del__(self):
        # make sure thread is cleaned up
        self.close()

    def __enter__(self):
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.close()
        return False
