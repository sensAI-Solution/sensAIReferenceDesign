"""
A test file to test basic functionality of HUB with GARD
This is test framework version 1, will be improved later with more tests

Imports hub library and uses Hub class to do the said operations
Does read/write of bulk data transfer and read/write on a register
Reads temperature sensor data
"""

import os
import sys
import struct
import unittest


current_dir = os.path.dirname(os.path.abspath(__file__))
hub_library_root = os.path.join(current_dir, "..", "hub_py", "hub")
sys.path.append(hub_library_root)

try:
    import hub
except ImportError as e:
    print(f"Failed to import: {e}")
    print("Please check directory structure.")

"""
This is to get version of codebase in development mode,
as versions are assigned during packaging.

This will find versions from hub_version.h file

@param: None
@returns: version (string)
"""


def get_my_version(hub_inc_dir: str) -> str:
    version_header_path = hub_inc_dir + "/hub_version.h"
    found_all = 0
    with open(version_header_path, "r") as f:
        for line in f:
            if "#define HUB_VERSION_MAJOR" in line:
                VER_MAJOR = line.split(" ")[-1][:-1]
                found_all += 1
            elif "#define HUB_VERSION_MINOR" in line:
                VER_MINOR = line.split(" ")[-1][:-1]
                found_all += 1
            elif "#define HUB_VERSION_BUGFIX" in line:
                VER_BUGFIX = line.split(" ")[-1][:-1]
                found_all += 1

            if found_all == 3:
                break

    my_version = str(VER_MAJOR + "." + VER_MINOR + "." + VER_BUGFIX)
    return my_version


# Helper function from the original script, useful for testing
def find_mismatch_index(b1: bytearray, b2: bytearray) -> int:
    min_len = min(len(b1), len(b2))
    for i in range(min_len):
        if b1[i] != b2[i]:
            return i
    if len(b1) != len(b2):
        return min_len
    return -1


# The main test class
class TestHubGard(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        """
        Setup method that runs once for the entire test class.
        It instantiates the HUB and GARD objects.
        """
        cls.hub_instance = hub.HUB(None, None)
        cls.hub_instance.logger.info("Setting up TestHubGard suite...")

        gard_num = cls.hub_instance.get_gard_count()
        if not gard_num:
            cls.hub_instance.logger.error("No GARDs discovered, skipping tests!")
            raise unittest.SkipTest("No GARDs discovered")
        else:
            cls.hub_instance.logger.info(f"{gard_num} GARD(s) discovered")

        cls.gard0 = cls.hub_instance.get_gard(0)
        if not cls.gard0:
            cls.hub_instance.logger.error(
                "Could not get GARD instance, skipping tests!"
            )
            raise unittest.SkipTest("Could not acquire GARD instance")
        else:
            cls.hub_instance.logger.info("GARD instance acquired for testing.")

    def test_01_register_operations(self):
        """Test read/write operations of buffers on registers."""
        self.hub_instance.logger.info("--- Testing Register Operations ---")

        data_size = 1024
        num_bytes = data_size * 4
        addr = 0x80000000

        # Create and write random data
        write_buffer = bytearray(os.urandom(num_bytes))
        write_chunk = list(struct.unpack(f"<{data_size}I", write_buffer))

        for i in range(data_size):
            retcode = self.gard0.write_register(addr, write_chunk[i])
            self.assertEqual(
                retcode, 0, f"Writing register at 0x{addr:x} failed with code {retcode}"
            )
            addr += 4

        # Read the data back
        addr = 0x80000000
        read_chunk = []
        for i in range(data_size):
            retcode, read_val = self.gard0.read_register(addr)
            self.assertEqual(
                retcode, 0, f"Reading register at 0x{addr:x} failed with code {retcode}"
            )
            read_chunk.append(read_val)
            addr += 4

        # Assert that the written data matches the read data
        self.assertEqual(
            write_chunk,
            read_chunk,
            f"Register data mismatch. First error at index {find_mismatch_index(write_chunk, read_chunk)}",
        )

    def test_02_data_operations(self):
        """Test bulk data send/receive operations."""
        self.hub_instance.logger.info("--- Testing Bulk Data Operations ---")

        data_size = 1024 * 4  # A larger size for bulk data
        addr = 0x80010000

        # Create and send random data
        write_buffer = bytearray(os.urandom(data_size))

        retcode = self.gard0.send_data(addr, write_buffer, len(write_buffer))
        self.assertEqual(
            retcode, 0, f"Failed to send bulk data with error code {retcode}"
        )

        # Read the data back
        retcode, read_buffer = self.gard0.receive_data(addr, len(write_buffer))
        self.assertEqual(
            retcode, 0, f"Failed to receive bulk data with error code {retcode}"
        )

        # Assert that the sent data matches the received data
        self.assertEqual(
            write_buffer,
            read_buffer,
            f"Bulk data mismatch. First error at index {find_mismatch_index(write_buffer, read_buffer)}",
        )

    def test_03_sensor_interfacing(self):
        """Test reading temperature and energy sensor data."""
        self.hub_instance.logger.info("--- Testing Sensor Interfacing ---")

        # Test temperature sensor reads
        self.hub_instance.logger.info("Reading temperature sensors data...")
        temp_data = self.hub_instance.get_temperature_data([1, 2])
        # Assert that the returned data is a dictionary
        self.assertIsInstance(temp_data, list, "Temperature data should be a list.")
        self.assertEqual(
            len(temp_data),
            2,
            "Returned temperature data list has an unexpected length.",
        )

        # Test energy sensor reads
        self.hub_instance.logger.info("Reading energy sensors data...")
        energy_data = self.hub_instance.get_energy_data([1, 2])

        # Assertions based on the new log data
        self.assertIsInstance(energy_data, list, "Energy data should be a list.")
        self.assertEqual(
            len(energy_data), 2, "Returned energy data list has an unexpected length."
        )

    @classmethod
    def tearDownClass(cls):
        """
        Teardown method that runs once after all tests in the class have run.
        It cleans up any resources used.
        """
        cls.hub_instance.logger.info("Tearing down TestHubGard suite...")
        # a necessary cleanup, destructor does it
        pass


# This allows running the tests directly from the command line
if __name__ == "__main__":
    unittest.main()
