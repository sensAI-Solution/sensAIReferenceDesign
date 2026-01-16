# generates the shared library object for the host side of i2c communication.
# If you're using python ctypes, use the i2c_pure_c.h header for a pure C wrapper - see ctypes_example.py for usage.
# If you're using C++, you can use i2c_adapter.h directly
# Note: no toolchain installed because it's assumed you're compiling on the raspberry pi
i2c_lib.so: i2c_adapter.cc i2c_pure_c.cc i2c_intr_gpio.c
	g++ -shared -o libi2c_lib.so -fPIC i2c_adapter.cc i2c_pure_c.cc i2c_intr_gpio.c -li2c -l wiringPi
