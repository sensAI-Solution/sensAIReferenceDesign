# -----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
# -----------------------------------------------------------------------------
# HUB Library
#
# A Python library for H.U.B. - Host-accelerated Unified Bridge
# Providing robust, user-friendly and abstracted interfaces
# Utilizing libhub for embedded applications for GARD
#
# The HUB.py is designed to provide python bindings to the
# HUB C library functions, so that python applications can use HUB natively.
#
# Interfaces of HUB and GARD class:
#
# HUB Initialization:
# 1.  [HUB] __hub_preinit() - Does initialization required for GARD discovery
#     Note: This is called by __init__(), not required to call explicitly
# 2.  [HUB] hub_init() - Does initialization & setup required for HUB to work
#     Note: This is called by __init__(), not required to call explicitly
#
# HUB GARD:
# 1.  [HUB] get_gard() - Gives GARD object based on GARD number
# 2.  [HUB] get_gard_count() - Gives count of GARDs connected to HUB
#
# HUB Operations:
# 1.  [GARD] write_register() - Writes given value at address addr of a register
# 2.  [GARD] read_register() - Reads value from given address addr of a register
# 3.  [GARD] send_data() - Sends data in bulk at address addr of size size over data bus
# 4.  [GARD] receive_data() - Receives data in bulk from address addr of size size over data bus
#
# HUB sensors:
# 1.  [HUB] get_temperature_data() - Gives list of temperature values for given temperature sensor IDs
# 2.  [HUB] get_energy_data()- Gives list of energy values for given energy sensor IDs
# -----------------------------------------------------------------------------

import os
import ctypes as ct
import logging
import threading

from .camInterface import CamInterface

ERRCODE_EXCEPTION_FAILURE = -2147483648  # int min
# Note: This Python variable value should reflect the value of
# the variable HUB_INVALID_SENSOR_VALUE in hub_sensors.h
HUB_INVALID_SENSOR_VALUE = -1


class HUB:
    # default variables
    hub_dir_default_path = "/opt/hub"
    so_dependencies_list = ["libcjson.so", "libusb-1.0.so", "libgpiod.so"]
    host_cfg_file = "host_config.json"
    hublib_file_name = "libhub.so"

    # __init__ is the constructor for HUB class.
    # This constructor sets up the HUB infrastructure to service its consumers.
    # It configures logger, discovers attached GARDs and creates objects
    # for every GARD device that it discovers.
    def __init__(
        self, host_config_file: str = None, config_file_path: str = None
    ) -> None:
        host_config_file = host_config_file or (
            os.path.join(self.hub_dir_default_path, "config", self.host_cfg_file)
        )
        config_file_path = config_file_path or os.path.join(
            self.hub_dir_default_path, "config"
        )

        self.configure_logging()

        # Throws Exception, handled in function
        retcode = self.load_hub_library()
        if not retcode:
            self.logger.error(f"Failed to init HUB object @ load_hub_library().")

        # TBD-SSP: Handle errors per func changes
        self.gards: dict[int, GARD] = {}
        self.__set_lib_args()

        # Throws Exception, handled in function
        retcode = self.__hub_preinit(host_config_file, config_file_path)
        if retcode:
            self.logger.error(
                f"Failed to init HUB object @ __hub_preinit() [{retcode}]"
            )
            raise InitFailureError("HUB Initialization")

        retcode = self.hub_lib.hub_discover_gards(self.hub)
        if retcode:
            self.logger.error(
                f"Failed to init HUB object @ __hub_discover_gards() [{retcode}]"
            )
            raise InitFailureError("HUB Initialization")
        else:
            self.logger.info("GARD Discovery is successful!")

        # Throws Exception, handled in function
        retcode = self.hub_init()
        if retcode:
            self.logger.error(f"Failed to init HUB object @ hub_init() [{retcode}]")
            raise InitFailureError("HUB Initialization")

        try:
            self.__create_gard_objects()
        except Exception as e:
            self.logger.error(f"Failed to create GARD objects: {e}")
            raise InitFailureError("HUB Initialization")

        # Create a CamInterface instance
        self.cam_instance = CamInterface(self.logger)

    # configure_logging uses "logger" to configure logging.
    # Logger is required to log messages in certain way.
    # The object contains self.logger which can be utilized
    # further in GARD and user applications to log in the same way.
    #
    # Sets format of logging as follows:
    # %(asctime)s - %(name)s - %(levelname)s - %(message)s
    # e.g. "2025-08-23 12:35:35,842 - HUB.HUB - INFO - Welcome to H.U.B v1.0.0"
    #
    # @param:     None
    # @returns:   None
    # @raises:    None
    def configure_logging(self) -> None:
        logging.basicConfig(
            level=logging.INFO,
            format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
        )
        self.name = self.__class__.__name__
        self.logger = logging.getLogger(f"HUB.{self.name}")

    # __del__ is the destructor of HUB class.
    # This gets automatically called when lifecycle of "self"/ object ends.
    # libhub's hub_fini() takes care of freeing memory, closing open buses, etc.
    # This calls fini for given hub handle.
    def __del__(self) -> None:
        self.hub_lib.hub_fini.restype = ct.c_int
        if self.hub:
            retcode = self.hub_lib.hub_fini(self.hub)

            if retcode:
                self.logger.error(
                    "Failed to run hub fini. Error code : {}".format(retcode)
                )
        else:
            self.logger.error("Failed to run hub fini")

    # HUB fini cleanup function
    def hub_fini(self) -> None:
        self.logger.debug("Starting HUB graceful termination.")

        if (
            hasattr(self, "hub_thread_appdata_context")
            and "stop_event" in self.hub_thread_appdata_context
        ):
            context = self.hub_thread_appdata_context

            context["stop_event"].set()
            self.logger.debug("Stop event set for appdata worker.")

            context["worker_thread"].join(timeout=5)

            if context["worker_thread"].is_alive():
                self.logger.warning(
                    "Worker thread did not terminate gracefully after 5s."
                )

        self.hub_lib.hub_fini.restype = ct.c_int
        if self.hub:
            retcode = self.hub_lib.hub_fini(self.hub)

            if retcode:
                self.logger.error(
                    "Failed to run hub fini. Error code : {}".format(retcode)
                )
        else:
            self.logger.error("Failed to run hub fini")

        self.logger.info("HUB cleanup complete.")
        os._exit(1)

    # _validate_version Validates version compatibility.
    # Since libhub and PY library has their own versioning,
    # it is required to compare and validate versions for interoperability.
    #
    # A version is defined as major.minor.bugfix.
    # When all 3 are same, both are compatible to work. Returns True.
    # When major and minor are not same, there are chances of incompatibility.
    #     Hence raises VersionMismatch Exception to warn user for upgrade.
    # When major and minor are same but bugfix doesn't match,
    #     it will NOT raise an exception and let user use HUB.
    #     It is assumed that compatibilities are maintained across bugfix
    #     mismatches. However it is recommented to use updated versions.
    #     In this case it returns False to notify partial match.
    #
    # @param: None
    #
    # @return: bool - version matched or not
    # @raises: VersionMismatchError
    def _validate_version(self, libhub_version, version) -> bool:
        self.logger.info(
            "HUB lib @ {}, HUB Py Library @ {}".format(libhub_version, version)
        )
        if libhub_version == version:
            self.logger.debug("Libhub and hub are at same versions")
            return True
        else:
            if version and libhub_version:
                if (
                    int(libhub_version.split(".")[0]) != int(version.split(".")[0])
                ) and (int(libhub_version.split(".")[1]) != int(version.split(".")[1])):
                    raise VersionMismatchError(
                        libhub_version,
                        version,
                        "Major version mismatch, upgrade to latest",
                    )
                else:
                    if int(libhub_version.split(".")[2]) > int(version.split(".")[2]):
                        self.logger.info(
                            "Libhub is at latest version, install latest hub library"
                        )
                    else:
                        self.logger.info(
                            "Hub py library is at latest version, install latest libhub"
                        )
            return False

    # load_hub_library loads libhub.so and it's dependencies.
    # It loades .so files from predefined paths /opt/hub on Linux.
    # self.so_dependencies_list have dependencies listed for libhub's so to work.
    # So this also loades dependency .so files from default paths.
    #
    # @param: None
    #
    # @return: bool - True on SUCCESS, False on FAILURE code
    # @raises: LoadSOFileError - On failure to load .so file
    def load_hub_library(self) -> bool:
        self.base_path = os.path.dirname(os.path.realpath(__file__))

        # TBD: this will be removed in future release as hublib will load it's dependencies
        for so_file in self.so_dependencies_list:
            try:
                self.hub_dependencies_path = self.hub_dir_default_path + "/lib/"
                _ = ct.CDLL(self.hub_dependencies_path + so_file, mode=ct.RTLD_GLOBAL)
            except Exception as e:
                self.logger.error("Unable to load {} file".format(so_file))
                raise LoadSOFileError(so_file, e)

        try:
            self.hublib_file_path = self.hub_dir_default_path + "/lib/"
            self.hub_lib = ct.CDLL(self.hublib_file_path + self.hublib_file_name)
        except Exception as e:
            self.logger.error("Unable to load {} file".format(self.hublib_file_name))
            raise LoadSOFileError("libhub.so", e)

        # GET versions
        # Calls native hub_get_version_string() function to read libhub version.
        # UTF-8 encoding is required to decode in readable string format.
        #
        # libhub_version will have the libhub's version available across
        self.hub_lib.hub_get_version_string.restype = ct.c_char_p

        version_bytes = self.hub_lib.hub_get_version_string()
        libhub_version = version_bytes.decode("utf-8")

        # This is the version of the HUB py package itself.
        # While creating the package __version__ in __init__.py gets assigned.
        # This reads it and stores in self.version variable.
        # Since HUB py package and libhub could be at different versions,
        # maintaing versioning of HUB package is important.
        try:
            from . import __version__

            self.version = str(__version__)
        except ImportError:
            self.version = ""

        if not (libhub_version and self.version):
            self.logger.error("Couldn't fetch version strings!")
            return False

        if not self._validate_version(libhub_version, self.version):
            return False

        return True

    # __set_lib_args is setter for arguments and return types of libhub functions.
    # Function to initialize argument and return types of the libhub functions.
    # To use C-types, function definitions needs to be created.
    # argtype sets the definition of the arguments required for function
    # restype sets the return type of the required function
    # This is needed before one can call any of the libhub C functions
    # These arguments and return types follow the C API's as defined in hub.h
    #
    # @param:     None
    # @returns:   None
    # @raises:    None
    def __set_lib_args(self) -> None:
        # TBD-SSP: check for errors, it the functions defined here doesnn't exists
        # Catch Attribute error and add decorator
        self.hub_lib.hub_preinit.argtypes = [
            ct.c_char_p,
            ct.c_char_p,
            ct.POINTER(ct.c_void_p),
        ]
        # C function Prototype:
        # enum hub_ret_code hub_preinit(void *p_config, char *p_gard_json_dir, hub_handle_t *hub);
        self.hub_lib.hub_preinit.restype = ct.c_int

        # C function Prototype:
        # enum hub_ret_code hub_discover_gards(hub_handle_t hub);
        self.hub_lib.hub_discover_gards.argtypes = [ct.c_void_p]
        self.hub_lib.hub_discover_gards.restype = ct.c_int

        # C function Prototype:
        # uint32_t hub_get_num_gards(hub_handle_t hub);
        self.hub_lib.hub_get_num_gards.argtypes = [ct.c_void_p]
        self.hub_lib.hub_get_num_gards.restype = ct.c_int

        # C function Prototype:
        # enum hub_ret_code hub_init(hub_handle_t hub);
        self.hub_lib.hub_init.argtypes = [ct.c_void_p]
        self.hub_lib.hub_init.restype = ct.c_int
        self.hub_lib.hub_get_gard_handle.restype = ct.c_void_p

        # C function Prototype:
        # uint32_t hub_get_temperature_from_onboard_sensors(
        #       hub_handle_t hub,
        #       uint8_t     *p_sensor_ids,
        #       float       *p_temp,
        #       uint8_t      num_sensors);
        self.hub_lib.hub_get_temperature_from_onboard_sensors.argtypes = [
            ct.c_void_p,
            ct.POINTER(ct.c_uint8),
            ct.POINTER(ct.c_float),
            ct.c_int,
        ]
        self.hub_lib.hub_get_temperature_from_onboard_sensors.restype = ct.c_int

        energy_sensor_fields = {
            "voltage": ct.c_float,
            "current": ct.c_float,
            "power": ct.c_float,
        }

        fields = list(energy_sensor_fields.items())

        # type function to create a new class dynamically
        self.EnergyValuesStruct = type(
            "EnergyValuesStruct", (ct.Structure,), {"_fields_": fields}
        )

        # C function Prototype:
        # uint32_t hub_get_energy_from_onboard_sensors(
        #       hub_handle_t                     hub,
        #       uint8_t                         *p_sensor_ids,
        #       struct hub_energy_sensor_values *p_values,
        #       uint8_t                          num_sensors);
        self.hub_lib.hub_get_energy_from_onboard_sensors.argtypes = [
            ct.c_void_p,
            ct.POINTER(ct.c_uint8),
            ct.POINTER(self.EnergyValuesStruct),
            ct.c_int,
        ]
        self.hub_lib.hub_get_energy_from_onboard_sensors.restype = ct.c_int

    # __hub_preinit is libhub's Pre-Init interface.
    # Pre-initialize HUB given a host configuration file and a GARD
    # configuration directory. Host configuration file is a JSON listing
    # details of HUB communication channels, profiles, etc. which are
    # essential for hub to run. GARD config folder holds JSONs, detailing
    # the GARD configurations for every GARD that HUB supports.
    # Ensure the JSON configs and the host-config path is UTF-8 encoded.
    #
    # self.config_path - stores config folder path
    # self.host_cfg_path - stores host config file path
    #
    # @param:     None
    #
    # @returns:   Status Code - 0: SUCCESS, Negetive: FAILURE code
    # @raises:    InitFailureError - On failure to run and fetch preinit function
    def __hub_preinit(self, host_cfg_path: str, config_path: str) -> int:
        self.host_cfg_path = host_cfg_path
        self.config_path = config_path

        self.hub = ct.c_void_p(None)
        config_path = ct.c_char_p(self.config_path.encode("utf-8"))
        host_cfg_path = ct.c_char_p(self.host_cfg_path.encode("utf-8"))

        preinit_result = self.hub_lib.hub_preinit(
            host_cfg_path, config_path, ct.byref(self.hub)
        )

        if preinit_result:
            self.logger.error(f"HUB Preinit failed with error code {preinit_result}")
            raise InitFailureError("PreInit")
        else:
            self.logger.debug("HUB Preinit is successful!")

        return preinit_result

    # get_gard_count is a function to find count of available GARDs.
    # Calls libhub's hub_get_num_gards() which returns number of GARDs
    # connected to given hub handle.
    #
    # @param:     None
    #
    # @returns:   Number of GARDs connected
    # @raises:    None
    def get_gard_count(self) -> int:
        self.gard_count = self.hub_lib.hub_get_num_gards(self.hub)

        return self.gard_count

    # hub_init is libhub's INIT interface.
    # It initializes HUB's internal data structures by calling libhub's
    # hub_init() for given hub handle.
    #
    # @param:     None
    #
    # @returns:   Status Code - 0: SUCCESS, Negetive: FAILURE code
    # @raises:    None
    def hub_init(self) -> int:
        init_result = self.hub_lib.hub_init(self.hub)

        if init_result:
            self.logger.error(f"HUB Init failed with error code {init_result}")
            raise InitFailureError("Init")
        else:
            self.logger.debug("init_result = {}".format(init_result))

        return init_result

    # __create_gard_objects creates GARD objects.
    # Creates a GARD python object for every gard handle returned by libhub.
    # Custom exception handles partial failures of GARD get handles.
    #
    # @param:     None
    #
    # @return:    None
    # @raises:    GardObjectsExistsError
    def __create_gard_objects(self) -> None:
        # self.gards is a dictionary of all GARD objects, mapping gard number to
        # GARD object. If this already exists, it means GARD objects have already
        # created or partially operated. Hence it raises GardObjectsExistsError.
        if self.gards:
            raise GardObjectsExistsError(-1)

        for i in range(self.get_gard_count()):
            try:
                if self.hub:
                    gard_ptr = self.hub_lib.hub_get_gard_handle(self.hub, i)

                    if gard_ptr is None:
                        raise RuntimeError(f"Failed to get GARD handle")

                    gard_obj = GARD(self, i, gard_ptr)
                    self.gards[i] = gard_obj
                else:
                    self.logger.error(
                        "HUB handle is missing. Corrupted HUB object, run __hub_preinit()"
                    )

            except RuntimeError as e:
                raise GardObjectsExistsError(i, str(e))
            except Exception as e:
                raise GardObjectsExistsError(i)

    # get_gard getter for GARD object from given gard number.
    #
    # @param:     gard_num - GARD number
    #
    # @returns:   GARD object
    def get_gard(self, gard_num: int) -> "GARD":
        if gard_num not in self.gards:
            self.logger.error(
                "Invalid GARD handle number is given to get_gard(): {}".format(gard_num)
            )
            return None
        else:
            return self.gards[gard_num]
        # return gard_obj._GARD__gard_handle - storing for future reference

    # get_temperature_data reads Temperature Data.
    # For a given list of sensor IDs, read and return temperature data in a list.
    # Function has sanitization for given list of sensors:
    # - unknown sensor IDs:
    #     Flags as sensor not found and skips to read data from it
    #     e.g. [1,2,3] - 3 is unknown
    # - checks if input is a list or not.
    #
    # example -
    # Input: [1,2,3]
    # Output: [48.5703125, 46.78125, None]
    #
    # @param:     given_sensors (list) - Value is None if not given
    #
    # @returns:   temperatures (list)
    # @raises:    None
    def get_temperature_data(self, given_sensors: list) -> list:
        if not given_sensors:
            self.logger.error("No sensor IDs given to fetch temperature info")
            return None

        if not isinstance(given_sensors, list):
            self.logger.error("Given input sensor ids should be an instance of list")
            return None

        num_expected_sensors = len(given_sensors)
        arr_expected_sensor_ids = ct.c_uint8 * num_expected_sensors

        arr_temperatures = (ct.c_float * num_expected_sensors)()

        # Function reads the sensor data over I2C and returns the count of sensors whose data
        # it was able to read successfully.
        # For every sensor that it was not able to fetch the temperature from, it returns HUB_INVALID_SENSOR_VALUE
        # for that sensor data's offset in the output buffer.
        try:
            # C function Prototype:
            # uint32_t hub_get_temperature_from_onboard_sensors(
            #     hub_handle_t hub,
            #     uint8_t     *p_sensor_ids,
            #     float       *p_temp,
            #     uint8_t      num_sensors)
            num_read_sensors = self.hub_lib.hub_get_temperature_from_onboard_sensors(
                self.hub,
                arr_expected_sensor_ids(*given_sensors),
                arr_temperatures,
                ct.c_int(num_expected_sensors),
            )

            # converts raw data from ctypes format
            ret_temperatures = list(arr_temperatures)

            # If value is HUB_INVALID_SENSOR_VALUE we replace it with None
            if num_read_sensors and num_read_sensors <= num_expected_sensors:
                if num_read_sensors != num_expected_sensors:
                    # TBD-SSP: find out which ones
                    self.logger.warning("Few of the sensors have not reported values")

                self.logger.info(
                    "Temperature read from {}/{} sensor(s) - {}".format(
                        num_read_sensors, num_expected_sensors, given_sensors
                    )
                )
                for index, given_sensor in enumerate(given_sensors):
                    output_str = ""

                    if ret_temperatures[index] == HUB_INVALID_SENSOR_VALUE:
                        output_str = "Sensor : {} >> Temperature : NA".format(
                            given_sensor
                        )
                        ret_temperatures[index] = None
                    else:
                        output_str = "Sensor : {} >> Temperature : {:.4f} °C".format(
                            given_sensor,
                            ret_temperatures[index],
                        )

                    self.logger.debug(output_str)

            else:
                self.logger.error(
                    f"Failed to read temperature from {given_sensors} sensors."
                )
                ret_temperatures = [None] * len(given_sensors)

        except Exception as e:
            self.logger.error(
                "Exception occured while reading temperature from sensors {}. Error - {}".format(
                    given_sensors, e
                )
            )
            ret_temperatures = [None] * len(given_sensors)

        return ret_temperatures

    # get_energy_data reads Energy Data.
    # For a given list of sensor IDs, read and return energy data in a list of dictionary.
    #
    # Function has sanitization for given list of sensors:
    # - unknown sensor IDs:
    #     Flags as sensor not found and skips to read data from it
    #     e.g. [1,2,3] - 3 is unknown
    # - checks if input is a list or not.
    #
    # This function creates a list of dictionary -
    # {"voltage": None, "current": None, "power": None} where if
    # the value is not present writes it with None at the same location of given sensors.
    #
    # example -
    # Input: [1,2,3]
    # Output:
    # [{'voltage': 5.139199733734131, 'current': 0.57952880859375, 'power': 3.37890625},
    # {'voltage': 5.142399787902832, 'current': 0.167236328125, 'power': 0.859375},
    # {'voltage': None, 'current': None, 'power': None}]
    #
    # @param:     given_sensors (list)
    #
    # @returns:   energies (list of dict)
    # @raises:    None
    def get_energy_data(self, given_sensors: list) -> list[dict]:
        if not given_sensors:
            self.logger.error("No sensor IDs given to fetch temperature info")
            return None

        if not isinstance(given_sensors, list):
            self.logger.error("Given input sensor ids should be an instance of list")
            return None

        num_expected_sensors = len(given_sensors)
        arr_expected_sensor_ids = ct.c_uint8 * num_expected_sensors

        # self.EnergyValuesStruct is a structure that stores returns data of current,
        # voltage and power with memory allocation. It creates a class dynamically
        # for this structure and stores data in place.
        arr_energy_values = (self.EnergyValuesStruct * num_expected_sensors)()

        # used to convert raw data from ctypes format
        ret_energy_values = [
            {"voltage": None, "current": None, "power": None}
            for _ in range(len(given_sensors))
        ]

        # Function reads the sensor data over I2C and returns the count of
        # sensors whose data it was able to read successfully.
        # For every sensor that it was not able to fetch the temperature from,
        # it returns HUB_INVALID_SENSOR_VALUE
        # for that sensor data's offset in the output buffer.
        try:
            # C function Prototype:
            #   uint32_t hub_get_energy_from_onboard_sensors(
            #    hub_handle_t                     hub,
            #    uint8_t                         *p_sensor_ids,
            #    struct hub_energy_sensor_values *p_values,
            #    uint8_t                          num_sensors);
            num_read_sensors = self.hub_lib.hub_get_energy_from_onboard_sensors(
                self.hub,
                arr_expected_sensor_ids(*given_sensors),
                arr_energy_values,
                ct.c_int(num_expected_sensors),
            )

            # If value is HUB_INVALID_SENSOR_VALUE we replace it with NAN
            if num_read_sensors and num_read_sensors <= num_expected_sensors:
                if num_read_sensors != num_expected_sensors:
                    # TBD-SSP: find out which ones
                    self.logger.warning("Few of the sensors have not reported values")

                self.logger.info(
                    "Energy values read from {}/{} sensor(s) - {}".format(
                        num_read_sensors, num_expected_sensors, given_sensors
                    )
                )

                for index, given_sensor in enumerate(given_sensors):
                    output_str = ""

                    if arr_energy_values[index].voltage == HUB_INVALID_SENSOR_VALUE:
                        ret_energy_values[index]["voltage"] = None
                    else:
                        ret_energy_values[index]["voltage"] = arr_energy_values[
                            index
                        ].voltage
                    if arr_energy_values[index].current == HUB_INVALID_SENSOR_VALUE:
                        ret_energy_values[index]["current"] = None
                    else:
                        ret_energy_values[index]["current"] = arr_energy_values[
                            index
                        ].current
                    if arr_energy_values[index].power == HUB_INVALID_SENSOR_VALUE:
                        ret_energy_values[index]["power"] = None
                    else:
                        ret_energy_values[index]["power"] = arr_energy_values[
                            index
                        ].power

                    output_str = "Sensor : {} >> Voltage : {} V, Current : {} A, Power : {} W".format(
                        given_sensor,
                        (
                            f'{ret_energy_values[index]["voltage"]:.4f}'
                            if (ret_energy_values[index]["voltage"] is not None)
                            else "NA"
                        ),
                        (
                            f'{ret_energy_values[index]["current"]:.4f}'
                            if (ret_energy_values[index]["current"] is not None)
                            else "NA"
                        ),
                        (
                            f'{ret_energy_values[index]["power"]:.4f}'
                            if (ret_energy_values[index]["power"] is not None)
                            else "NA"
                        ),
                    )

                    self.logger.debug(output_str)
            else:
                self.logger.error(
                    f"Failed to read energy values from {given_sensors} sensors."
                )
        except Exception as e:
            self.logger.error(
                "Exception occured while reading energy values from sensors {}. Error - {}".format(
                    given_sensors, e
                )
            )

        return ret_energy_values

    # appdata_events_handler is worker thread function to monitor appdata events.
    # It runs in a loop until stop_event is set in the context.
    # For every iteration it calls libhub's hub_get_appdata_on_event_handler()
    # to monitor and get appdata.
    # On receiving appdata, it calls user callback function with context and
    # received appdata buffer.
    def appdata_events_handler(self, hub_thread_appdata_context):
        self.logger.debug("Launched worker thread for monitoring.")

        while not hub_thread_appdata_context["stop_event"].is_set():
            try:
                # C function Prototype:
                # int64_t hub_get_appdata_on_event_handler(gard_handle_t     p_gard_handle,
                # 											int 				line_offset,
                # 											void 				*buffer,
                # 											uint32_t 			size)
                # Returns: data_size (> 0) on success, error code (<= 0) on failure
                self.hub_lib.hub_get_appdata_on_event_handler.argtypes = [
                    ct.c_void_p,
                    ct.c_int,
                    ct.c_void_p,
                    ct.c_uint32,
                ]
                self.hub_lib.hub_get_appdata_on_event_handler.restype = ct.c_int64

                BufferArrayType = ct.c_char * len(hub_thread_appdata_context["buffer"])
                ctypes_buffer = BufferArrayType.from_buffer(
                    hub_thread_appdata_context["buffer"]
                )

                app_callback_result = self.hub_lib.hub_get_appdata_on_event_handler(
                    hub_thread_appdata_context["gard_handle"],
                    hub_thread_appdata_context["line_offset"],
                    ct.byref(ctypes_buffer),
                    ct.c_uint32(len(hub_thread_appdata_context["buffer"])),
                )

                if app_callback_result < 0:
                    self.logger.error(
                        f"Failed to get appdata. Error code - {app_callback_result}"
                    )
                    return False

                else:
                    # received positive value - so it is size read
                    # TBD-SSP : this needs to be implemented in hub & Gard
                    if app_callback_result > 0:
                        user_callback_result = hub_thread_appdata_context["cb_handler"](
                            hub_thread_appdata_context["cb_ctx"],
                            hub_thread_appdata_context["buffer"][:app_callback_result],
                        )

                        if user_callback_result and user_callback_result != 0:
                            self.logger.info(
                                f"User callback function returned with code - {user_callback_result}"
                            )
                    else:
                        user_callback_result = hub_thread_appdata_context["cb_handler"](
                            hub_thread_appdata_context["cb_ctx"],
                            hub_thread_appdata_context["buffer"],
                        )
            except KeyboardInterrupt:
                hub_thread_appdata_context["stop_event"].set()

            except Exception as e:
                self.logger.error(f"Internal error in worker thread: {e}")
                break

        self.logger.debug("Shutting down worker thread.")

    # setup_appdata_callback sets up application data callback.
    # It registers a callback handler for application data events from GARD.
    # It creates a worker thread to monitor and get appdata events.
    def setup_appdata_callback(self, gard_obj, cb_handler, ctx, buffer):
        if (not gard_obj) or not isinstance(gard_obj, GARD):
            self.logger.error("Invalid GARD object given")
            return

        if (not cb_handler) or not callable(cb_handler):
            self.logger.error("Invalid callback handler given")
            return

        if not buffer:
            self.logger.error("Invalid buffer given")
            return

        if not ctx:
            self.logger.warning("Invalid ctx given or is empty")

        hub_thread_appdata_context = {
            "gard_handle": gard_obj.get_gard_handle(),
            "cb_handler": cb_handler,
            "buffer": buffer,
            "stop_event": threading.Event(),
            "cb_ctx": ctx,
        }

        worker_thread = threading.Thread(
            target=self.appdata_events_handler,
            name=f"HUB Appdata Worker",
            kwargs={"hub_thread_appdata_context": hub_thread_appdata_context},
        )
        hub_thread_appdata_context["worker_thread"] = worker_thread

        try:
            # C function Prototype:
            # enum hub_ret_code hub_setup_appdata_cb_for_pyhub(
            #                          gard_handle_t    gard)
            self.hub_lib.hub_setup_appdata_cb_for_pyhub.argtypes = [ct.c_void_p]
            self.hub_lib.hub_setup_appdata_cb_for_pyhub.restype = ct.c_int

            # Call the C function to register the callback.
            app_callback_result = self.hub_lib.hub_setup_appdata_cb_for_pyhub(
                hub_thread_appdata_context["gard_handle"]
            )

            if app_callback_result < 0:
                self.logger.error(f"Failed to setup appdata init function.")
                return False
            else:
                hub_thread_appdata_context["line_offset"] = app_callback_result
                worker_thread.start()

        except KeyboardInterrupt:
            hub_thread_appdata_context["stop_event"].set()
            return True

        except Exception as e:
            self.logger.error(f"Exception occurred during setup: {e}")

            if worker_thread:
                worker_thread.join()
                hub_thread_appdata_context["stop_event"].set()

            return False

        return True


class GARD:

    # __init__ is a GARD Constructor.
    # It is a parameterized constructor with gard number, gard handle as inputs.
    # self.__gard_handle stores GARD handle pointer for given GARD.
    # self.gard_num stores GARD number.
    # self.hub_obj stores HUB class object to which GARD is connected.
    # self.logger uses HUB's logger for uniformity.
    # Then it creates arguments and return type definitions for the libhub
    # function calls used within this class.
    def __init__(self, HUB, gard_num: int, gard_obj_ptr: ct.c_void_p):
        self.name = self.__class__.__name__
        self.__gard_handle = gard_obj_ptr
        self.gard_num = gard_num
        self.hub_obj = HUB
        self.logger = HUB.logger
        self.__set_lib_args()

    # __del__ is a GARD Destructor.
    # Just unlinks GARD's handle from this this object. Actual freeing
    # will be taken care by HUb's hub_fini().
    def __del__(self):
        if self.__gard_handle:
            # call underlying library to delete gard pointer - discuss
            self.__gard_handle = None
        else:
            self.logger.info("GARD instance is already freed or not initialized")

    def get_gard_handle(self):
        return self.__gard_handle

    # __set_lib_args is a setter for arguments and return types of libhub functions.
    # Function to initialize argument and return types of the libhub functions
    # argtype sets the definition of the arguments required for function
    # restype sets the return type of the required function
    # This is needed before one can call any of the libhub C functions
    # These arguments and return types follow the C API's as defined in hub.h
    #
    # @param:     None
    # @returns:   None
    # @raises:    None
    def __set_lib_args(self) -> None:
        # C function Prototype:
        # enum hub_ret_code hub_write_gard_reg(gard_handle_t  p_gard_handle,
        # 								 uint32_t       reg_addr,
        # 								 const uint32_t value);
        self.hub_obj.hub_lib.hub_write_gard_reg.argtypes = [
            ct.c_void_p,
            ct.c_int,
            ct.c_int,
        ]
        self.hub_obj.hub_lib.hub_write_gard_reg.restype = ct.c_int

        # C function Prototype:
        # enum hub_ret_code hub_read_gard_reg(gard_handle_t p_gard_handle,
        # 							uint32_t      reg_addr,
        # 							uint32_t     *p_value);
        self.hub_obj.hub_lib.hub_read_gard_reg.argtypes = [
            ct.c_void_p,
            ct.c_int,
            ct.POINTER(ct.c_uint),
        ]
        self.hub_obj.hub_lib.hub_read_gard_reg.restype = ct.c_int

        # C function Prototype:
        # enum hub_ret_code hub_send_data_to_gard(gard_handle_t p_gard_handle,
        # 								const void   *p_buffer,
        # 								uint32_t      addr,
        # 								uint32_t      count);
        self.hub_obj.hub_lib.hub_send_data_to_gard.argtypes = [
            ct.c_void_p,
            ct.c_void_p,
            ct.c_int,
            ct.c_int,
        ]
        self.hub_obj.hub_lib.hub_send_data_to_gard.restype = ct.c_int

        # C function Prototype:
        # enum hub_ret_code hub_recv_data_from_gard(gard_handle_t p_gard_handle,
        # 										   void         *p_buffer,
        # 										   uint32_t      addr,
        # 										   uint32_t      count);
        self.hub_obj.hub_lib.hub_recv_data_from_gard.argtypes = [
            ct.c_void_p,
            ct.c_void_p,
            ct.c_int,
            ct.c_int,
        ]
        self.hub_obj.hub_lib.hub_recv_data_from_gard.restype = ct.c_int

    # write_register writes value to a register within GARD's AXI map.
    # Calls libhub's hub_write_gard_reg() which requires GARD handle,
    # address to which one should write, and a value to write.
    # Exception occurs when inputs provided to function calls are
    # uninterpretable, which often leads to segmentation faults too.
    #
    # Returns ERRCODE_EXCEPTION_FAILURE in case of exceptions.
    #
    # @param:     address (int)
    # @param:     value (int)
    #
    # @returns:   Status Code - 0: SUCCESS, Negetive: FAILURE code
    # @raises:    None
    def write_register(self, addr: int, value: int) -> int:
        reg_write_result = ERRCODE_EXCEPTION_FAILURE
        try:
            reg_write_result = self.hub_obj.hub_lib.hub_write_gard_reg(
                self.__gard_handle, ct.c_int(addr), value
            )

            if reg_write_result == 0:
                self.logger.debug(
                    "Written data '{:#x}' over register at {:#x}. Response : = {}".format(
                        value, addr, reg_write_result
                    )
                )
            elif reg_write_result < 0:
                self.logger.error(
                    "HUB failed to write data '{:#x}' over register at {:#x}. Response : = {}".format(
                        value, addr, reg_write_result
                    )
                )
        except Exception as _:
            self.logger.error(
                "Unable to write data '{:#x}' over register at {:#x}".format(
                    value, addr
                )
            )

        return reg_write_result

    # read_register reads value from a register within GARD's AXI map.
    # Calls libhub's hub_read_gard_reg() which requires GARD handle,
    # address to which one should read, and a placeholder for value.
    # Exception occurs when inputs provided to function calls are
    # uninterpretable, which often leads to segmentation faults too.
    #
    # Returns ERRCODE_EXCEPTION_FAILURE in case of exceptions.
    #
    # @param:     address (int)
    #
    # @returns:   Status Code - 0: SUCCESS, Negetive: FAILURE code
    # @returns:   read value (int)
    # @raises:    None
    def read_register(self, addr: int) -> tuple[int, int]:
        value = ct.c_uint()
        reg_read_result = ERRCODE_EXCEPTION_FAILURE

        try:
            reg_read_result = self.hub_obj.hub_lib.hub_read_gard_reg(
                self.__gard_handle, ct.c_int(addr), ct.byref(value)
            )

            if reg_read_result == 0:
                self.logger.debug(
                    "Read data '{:#x}' over register at {:#x}. Response : = {}".format(
                        value.value, addr, reg_read_result
                    )
                )
            elif reg_read_result < 0:
                self.logger.error(
                    "HUB failed to read data '{:#x}' over register at {:#x}. Response : = {}".format(
                        value.value, addr, reg_read_result
                    )
                )

            return reg_read_result, value.value
        except Exception as _:
            self.logger.error(
                "Unable to read data '{:#x}' over register at {:#x}".format(
                    value.value, addr
                )
            )

            return reg_read_result, None

    # send_data write data over data bus.
    # Uses libhub's hub_send_data_to_gard() to send data at given address
    # of given size. It returns the status code
    #
    # @param:     address (int)
    # @param:     data (bytes, bytearray, array, list)
    # @param:     size (int)
    #
    # @returns:   Status Code - 0: SUCCESS, Negetive: FAILURE code
    # @raises:    None
    def send_data(self, addr: int, data, size: int) -> int:
        reg_write_result = ERRCODE_EXCEPTION_FAILURE

        try:
            if not isinstance(data, (bytes, bytearray, list)):
                data = bytearray(data)

            if isinstance(data, bytes):
                processed_data = ct.create_string_buffer(data)
            else:
                processed_data = ct.create_string_buffer(bytes(data))

            reg_write_result = self.hub_obj.hub_lib.hub_send_data_to_gard(
                self.__gard_handle, processed_data, ct.c_int(addr), ct.c_int(size)
            )

            if reg_write_result == 0:
                self.logger.debug(
                    "Written data at {:#x}. Response : = {}".format(
                        addr, reg_write_result
                    )
                )
            elif reg_write_result < 0:
                self.logger.error(
                    "HUB failed to write data at {:#x}. Response : = {}".format(
                        addr, reg_write_result
                    )
                )
        except TypeError as te:
            self.logger.error("Data type error, {}".format(te))
        except Exception as e:
            self.logger.error("Unable to write data at {:#x}, {}".format(addr, e))

        return reg_write_result

    # receive_data reads data over data bus.
    # Uses libhub's hub_recv_data_from_gard() to receive data at given address
    # of given size. It returns the data size received and read data.
    #
    # @param:     address (int)
    # @param:     size (int)
    #
    # @returns:   error code (int) - 0 on success, error code on failure
    # @returns:   data (bytes)
    # @raises:    None
    def receive_data(self, addr: int, size: int) -> tuple[int, bytes]:
        reg_read_result = ERRCODE_EXCEPTION_FAILURE

        try:
            data = ct.create_string_buffer(size)
            reg_read_result = self.hub_obj.hub_lib.hub_recv_data_from_gard(
                self.__gard_handle, data, ct.c_int(addr), ct.c_int(size)
            )

            if reg_read_result == 0:
                self.logger.debug(
                    "Read data at {:#x}. Received size: {}".format(addr, reg_read_result)
                )
                return reg_read_result, data.raw
            else:
                self.logger.error(
                    "HUB failed to read data at {:#x}. Response : = {}".format(
                        addr, reg_read_result
                    )
                )
                return reg_read_result, b""
        except Exception as e:
            self.logger.error("Unable to read data at {:#x} , {}".format(addr, e))

        return reg_read_result, b""


# Exception Classes


class GardNotFoundError(Exception):
    def __init__(self):
        self.message = "Failure to find or connect given gard."
        super().__init__(self.message)


class GardObjectsExistsError(Exception):
    def __init__(self, gard: int, message: str = ""):
        if gard == -1:
            self.message = "One or more GARD objects already exists. Cannot bulk create GARD objects."
        else:
            if message:
                self.message = "Failed to create GARD {} object. Error : {}".format(
                    gard, message
                )
            else:
                self.message = "Failed to create GARD {} object.".format(gard)
        super().__init__(self.message)


class LoadSOFileError(Exception):
    def __init__(self, filename: str, e):
        self.message = "Unable to load {} file. Error: {}".format(filename, e)
        super().__init__(self.message)


class InitFailureError(Exception):
    def __init__(self, operation: str):
        self.message = f"{operation} failed. See the logs for more info"
        super().__init__(self.message)


class VersionMismatchError(Exception):
    def __init__(self, v1: str, v2: str, msg: str):
        self.message = "Versions mismatch: {} and {}. ".format(v1, v2) + msg
        super().__init__(self.message)


class LoggerWriter:
    # Define ANSI color codes
    BLUE = "\033[94m"
    RED = "\033[91m"
    RESET = "\033[0m"

    def __init__(self, logger, level):
        self.logger = logger
        self.level = level
        self.buffer = ""
        self.color_map = {logging.INFO: self.BLUE, logging.ERROR: self.RED}

    def write(self, message: str):
        """
        Writes a colorized message to the internal buffer.
        The buffer is only flushed to the logger when a newline character is present in the message.
        """
        self.buffer += message
        if "\n" in self.buffer:
            self.flush()

    def flush(self):
        """Flushes the colorized buffer to the logger."""
        if self.buffer:
            log_message = self.buffer.rstrip()
            if log_message:
                color = self.color_map.get(self.level, self.RESET)
                colored_message = f"{color}{log_message}{self.RESET}"
                # The use of ANSI color codes in LoggerWriter may not work correctly
                # in all environments (e.g., Windows without color support)
                self.logger.log(self.level, colored_message)
            self.buffer = ""
