################################################################################
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
################################################################################


################################################################################
# This file contains code that is used to build the Camera Configuration image
# for a specific camera. The generated image file is then to the Flash Image
# using the script ImageBuilder.py.
#
# The Camera Configuration image is a binary file that contains the
# configuration for a specific Make/Model of a Camera. This comfiguration is
# laid out in a format that is not only extensible but also adds a structure
# needed for quick searches. The format of this structure is defined in
# "GARD Flash Layout.xlsx" file sheet "Camera Configuration".
# The Camera Configuration commands are vendor-specific and are to be provided
# by the User.
#
# The script takes various configurations for the camera on command line. Each
# configuration is present in a separate file and all these files are provided
# on the command line. The script then builds the Camera Configuration image
# by writing the configuration files to the output file with a directory header
# referencing the configuration files within the generated image file.
#
################################################################################

import argparse
import os
import sys
import struct
from enum import IntEnum
from xlate_camera_config import I2CBinGenerator


# This enum defines the ID ranges of the various modules that are currently
# supported.
# These values should be similar to the enum values defined in
# 'enum CAMERA_COMMAND_ID' which is defined in the file CameraConfig.h
#
# TBD-SRP - Explore possibility of using the enum values defined in
# CameraConfig.h file instead of defining them here. This will avoid
# incompatibility between the two files.

class CameraActionUID(IntEnum):
	EMPTY_ENTRY 					= 0

	BASIC_CAMERA_CONFIG				= 0x1001

	# Add more Camera configuration information UIDs here as needed. The maximum
	# value of the UID for Camera Config information is 0x1010.


	# The following UIDs are used for the Camera Configuration Actions.
	CAM_CFG_CMND_START_STREAM 		= 0x1011
	CAM_CFG_CMND_STOP_STREAM 		= 0x1012
	CAM_CFG_CMND_SET_FPS 			= 0x1013
	CAM_CFG_CMND_WHITE_BALANCE 		= 0x1014
	CAM_CFG_CMND_SET_EXPOSURE 		= 0x1015
	CAM_CFG_CMND_SET_AUTO_FOCUS 	= 0x1016
	CAM_CFG_CMND_SET_CONTRAST 		= 0x1017
	CAM_CFG_CMND_SET_BRIGHTNESS 	= 0x1018
	CAM_CFG_CMND_SET_SATURATION 	= 0x1019
	CAM_CFG_CMND_SET_SHARPNESS 		= 0x101A
	CAM_CFG_CMND_SET_HUE 			= 0x101B


	CAM_CFG_CMND_SET_GAIN 			= 0x1016
	CAM_CFG_CMND_SET_AUTO_EXPOSURE 	= 0x101C
	CAM_CFG_CMND_SET_AUTO_WHITE_BALANCE = 0x101D
	CAM_CFG_CMND_SET_AUTO_GAIN 		= 0x101F
	CAM_CFG_CMND_SET_AUTO_BRIGHTNESS = 0x1020
	CAM_CFG_CMND_SET_AUTO_CONTRAST 	= 0x1021
	CAM_CFG_CMND_SET_AUTO_SATURATION = 0x1022
	CAM_CFG_CMND_SET_AUTO_SHARPNESS = 0x1023
	CAM_CFG_CMND_SET_AUTO_HUE 		= 0x1024

	# Add more Camera Configuration Commands UIDs here as needed, the maximum
	# value of the UID for Camera Configuration Commands is 0xEFFF.

	CAM_CFG_CMND_LAST_VALID_CMND	= 0xEFFF


	# Camera Configuration Command UIDs in the range 0xF000 to 0xFFFF are
	# reserved for the Developers to try out these before assigning a valid UID
	# for that Camera Configuration Command.
	CAM_CFG_CMND_DEVELOPER_START 	= 0xF000
	CAM_CFG_CMND_DEVELOPER_END 		= 0xFFFF

	# get_enum_value() is a method that returns the enum value for a given
	# enum ID string. The string needs to be a space separated configuration
	# command. Any string that is not a valid enum ID will raise a ValueError.
	@classmethod
	def get_enum_value(cls, enum_id_str):
		enum_id_str = enum_id_str.strip().replace(' ', '_').upper()
		enum_id_str = "CAM_CFG_CMND_" + enum_id_str
		return cls[enum_id_str]


class CameraInterfaces(IntEnum):
	# This enum defines the interfaces which are supported by the camera.
	MIPI = 0x1  # MIPI interface
	I2C = 0x2  # I2C interface
	GPIO = 0x4  # GPIO interface

# parse_int() is a utility function that parses a string as an integer.
# It supports hexadecimal, decimal, and octal formats. If the string cannot be
# parsed as an integer, it raises an ArgumentTypeError with a message indicating
# the invalid value.
def parse_int(value):
	try:
		return int(value, 0)
	except ValueError:
		raise argparse.ArgumentTypeError(f"Invalid integer value: {value}")

# pad_string() is a utility function that pads a string to a specified length
# with null bytes. If the string is longer than the specified length, it is
# truncated to fit. The default length is 16 bytes.
# The function returns a bytes object with the null-padded string.
def pad_string(value, length=16):
		return value.encode('utf-8')[:length-1].ljust(length-1, b'\0')

# Setup command-line argument parser. The help is embedded within the argument.
parser = argparse.ArgumentParser(description='Camera Configuration Image Builder for GARD', formatter_class=argparse.RawTextHelpFormatter)

parser.add_argument('-o', '--outfile', metavar="FILE", type=str, required=True,
					help='Binary file which contains the built image')
parser.add_argument('-c', '--camconfcmnd', action="append", type=str, required=False,
					help="""CAM-CFG-CMND=File containing Camera Configuration Command. Thus if you need to provide
					Camera Configuration Command '0x1015' the file contents in cam_config_cmnd38.bin then the parameter would be:
					--camconf 0x1015=cam_config_cmnd38.bin. The other way to do the same is to provide a text name for the
					Camera Configuration Command and the file containing the Camera Configuration Command.
					For example, if you want to provide Camera Configuration Command 'White Balance' in the file
					cam_config_cmnd38.bin then the parameter would be: --camconf "White Balance"=cam_config_cmnd38.bin
					Note that the Camera Configuration Command UID is case-insensitive, so you can provide
					the Camera Configuration Command UID in any case.""")
parser.add_argument('-r', '--basicconf', action='append', type=str, required=False,
					help="""Provide a comma-separated list of basic configuration details.
					Following is one example: --basicconf="LayoutVersion=1,DriverVersion=3.23.04,VendorName=Sony,Interfaces=mipi,ModelNo=IMX-219,Interfaces=i2c
					Currently following configuration details are supported:
					- LayoutVersion: Version of the Basic Configuration structure layout and Camera Configuration Directory.
					- VendorName: Name of the camera vendor. This is a null-terminated string of max length up to 16 bytes including the null-terminator.
					- ModelNo: Name of the camera model. This is a null-terminated string of max length up to 16 bytes including the null-terminator.
					- DriverVersion: Version of the camera driver. This is a null-terminated string of max length up to 16 bytes including the null-terminator.
					- Interface: This is a bitmask which indicates the interfaces supported by the camera.
						- i2c: Indicates that the camera supports I2C interface.
						- gpio: Indicates that the camera supports GPIO interface.
						- mipi: Indicates that the camera supports MIPI interface.
					Note that the interface is a bitmask, so you can provide multiple interfaces by using
					pipe (|) as a separator. For example, if the camera supports I2C and SPI interfaces,
					then you can provide the interface as "i2c | spi".
					  """)
parser.add_argument('--i2cconfig', metavar="FILE", type=str, required=False,
					help='Text file containing I2C register configuration to be parsed and added to camera config image.')


class CameraConfigBuilder:
	# This class is used to build the Camera Configuration image. It takes the
	# configuration files provided on the command line and writes them to the
	# output file in a specific format. The format is defined in the
	# "GARD Flash Layout.xlsx" file sheet "Camera Configuration".

	# Camera Configuration image is a compelete monilithic image written in
	# flash. Since the complete Camera Configuration image is updated instead of
	# individual Configuration, the individual contents within this file need
	# not be aligned to any erase boundary.
	outfile = None  # Output file where the image will be written


	class BasicConfiguration:
		# This class defines the Basic configuration which is written to the
		# output file. The runtime configuration is written as the first
		# camera config command directory entry.

		# Basic configuration is of the following format:
		# Layout Version (4 bytes)
		# Vendor Name (16 bytes)
		# Model Name (16 bytes)
		# Driver Version (16 bytes)
		# Count Of Directory Entries (4 bytes)
		# Camera Comm Interfaces (4 bytes)
		BASIC_CONF_FORMAT = "<I16s16s16sII"
		LAYOUT_VERSION = 1  # Current layout version

		# Set defaults for the basic configuration. These defaults are likely to
		# be overridden by the command line arguments.
		def __init__(self):
			# Initialize the runtime configuration with default values.
			self.layout_version = self.LAYOUT_VERSION
			self.vendor_name = b"Lattice" + b'\x00' * (16 - len("DefaultVendor"))
			self.model_name = b"Model 1" + b'\x00' * (16 - len("Model 1"))
			self.driver_version = b"1.0.0" + b'\x00' * (16 - len("1.0.0"))
			self.count_of_dir_entries = 0
			self.camera_comm_intf = 0

		# get_configuration() returns the runtime configuration as a bytes object
		# which can be written to the output file. The format of the bytes object
		# is defined in the module structure format variable BASIC_CONF_FORMAT.
		def get_configuration(self):
			# Pack the runtime configuration into a bytes object.
			return struct.pack(
				self.BASIC_CONF_FORMAT,
				self.layout_version,
				self.vendor_name,
				self.model_name,
				self.driver_version,
				self.count_of_dir_entries,
				self.camera_comm_intf,
			)

	class CameraConfigDirectory:
		# This class defines the directory structure within the Camera
		# Configuration for a specific Camera.
		# This class just accumulates the directory entries in a list and returns
		# the list to the caller of get_dir_entries() method. It is the caller's
		# responsibility to write the directory entries to the output file.
		# This class provides the following help:
		# * Returns the directory entries list in a format which can be directly
		# 	written to the output file.
		# * Ensures that the UID of each entry is unique.

		# Each directory entry is of the following format:
		# Camera Configration Command UID (4 bytes)
		# Offset (4 bytes)
		# Size (4 bytes)
		# Below is the same format defined using struct module.
		DIR_ENTRY_FORMAT = "III"

		first_UID = None
		dir_entries = []  # List to hold directory entries

		# get_dir_entry_size() returns the size of the directory entry in bytes.
		# This size is based on the DIR_ENTRY_FORMAT defined above.
		def get_dir_entry_size(self):
			return struct.calcsize(self.DIR_ENTRY_FORMAT)

		# add_entry() adds a new directory entry to an internal list. It also
		# checks if the UID is unique in the directory. If the UID is not unique
		# it raises a ValueError.
		# The entry is added in the format DIR_ENTRY_FORMAT defined above.
		def add_entry(self, uid, offset, size):

			# Ensure this UID is unique in the directory.
			if any(struct.unpack("III", entry)[0] == uid for entry in self.dir_entries):
				raise ValueError(f"UID {uid} is passed multiple times on command line. Exiting")

			# Pack and write the directory entry to the list.
			entry = struct.pack(self.DIR_ENTRY_FORMAT, uid, offset, size)
			self.dir_entries.append(entry)

		# get_dir_entries() return the formatted directory entries as a bytes
		# object that can be written to the output file.
		def get_dir_entries(self):
			if self.first_UID:
				# Bring the first UID to the start of the directory entries.
				old_first_entry = self.dir_entries[0]
				entry = struct.unpack(self.DIR_ENTRY_FORMAT, old_first_entry)
				if entry[0] == self.first_UID:
					# Nothing to be done as everything is already in order.
					pass
				else:
					for item in self.dir_entries:
						if struct.unpack(self.DIR_ENTRY_FORMAT, item)[0] == self.first_UID:
							# This is the first entry, so we need to move it to the start.
							self.dir_entries.remove(item)
							self.dir_entries.insert(0, item)
							break
			return b''.join(self.dir_entries)

	# Unitialized variable to hold the next write offset in the output file.
	# This variable gets set during parsing of the command line arguments.
	confg_cmnd_pyld_offset = None

	# write_camera_config_to_file() writes the camera configuration command
	# payload to the output file and updates the directory.
	def write_camera_config_to_file(self, uid, infile):

		# Keep the file size handy.
		confg_cmnd_pyld_size = os.path.getsize(infile)

		# Get UID in integer format.
		if isinstance(uid, str):
			# If the UID is provided as a string, convert it to enum value.
			uid = CameraActionUID.get_enum_value(uid)

		# Add entry in the directory for the Camera Configuration Command
		# payload.
		ccb.config_dir.add_entry(uid, self.confg_cmnd_pyld_offset, confg_cmnd_pyld_size)

		# Write the Camera Configuration Command payload at precise offset in
		# the output file.
		self.outfile.seek(self.confg_cmnd_pyld_offset)

		with open(infile, "rb") as f:
			self.outfile.write(f.read())

		# Update the offset for the next Camera Configuration Command payload.
		self.confg_cmnd_pyld_offset += confg_cmnd_pyld_size

	# write_camera_info_to_file() writes the camera related information to the
	# output file and updates the directory. This information is the basic
	# configuration information that is written as the first entry in the
	# directory.
	def write_camera_info_to_file(self, uid, config_info):

		# Keep the payload size handy.
		confg_cmnd_pyld_size = len(config_info)

		# Get UID in integer format.
		if isinstance(uid, str):
			# If the UID is provided as a string, convert it to enum value.
			uid = CameraActionUID.get_enum_value(uid)

		# Add entry in the directory for the Camera Data.
		ccb.config_dir.add_entry(uid, self.confg_cmnd_pyld_offset, confg_cmnd_pyld_size)

		# Write the Camera information payload at precise offset in the output
		# file.
		self.outfile.seek(self.confg_cmnd_pyld_offset)

		self.outfile.write(config_info)

		# Update the offset for writing either:
		# - The next batch of Camera Information or,
		# - The Camera Configuration Command payload.
		self.confg_cmnd_pyld_offset += confg_cmnd_pyld_size

	# write_cc_dir_to_file() writes the directory to the start of the output
	# file.
	def write_cc_dir_to_file(self):
		# Directory is written at the start of the output file.
		self.outfile.seek(0)

		# Write the directory entries to the output file.
		self.outfile.write(self.config_dir.get_dir_entries())

	# CameraConfigBuilder constructor initializes the output file and the
	# configuration directory and basic configuration objects.
	def __init__(self, outfile):
		self.outfile = outfile
		self.config_dir = self.CameraConfigDirectory()
		self.basic_config = self.BasicConfiguration()

# write_camera_config_layout_from_args() is a function that parses the command
# line arguments.
# Since the Camera information is to be written before other Camera
# configuration commands are written, this function first scans for Camera
# information from the command-line arguments and build the Camera information
# structure before writing it out the output file.
# It then collects the Camera Configuration Commands from the command line
# arguments and writes them to the output file.
def write_camera_config_layout_from_args(args, ccb):

	# We parse the parameters twice. In the first round we find the count of
	# camera configuration commmands are provided so that we can calculate the
	# size of the directory.
	#
	# In the second iteration we start to fill the directory and also the camera
	# configuration file with the parameter values.

	# Start counting the number of directory entries that will be needed.
	ccb.basic_config.count_of_dir_entries = 0

	# First entry is the basic camera configuration entry.
	if args.basicconf:
		ccb.basic_config.count_of_dir_entries += 1  # +1 for the basic configuration entry

	# Now find all the camera configuration commands that are provided on the
	# command line.
	for arg, arg_value in vars(args).items():
		if arg == 'camconfcmnd' and arg_value:
			for ccc_entry in arg_value:
				ccb.basic_config.count_of_dir_entries += 1

	# Set command payload offset to the end of the last directory entry. This is
	# where we will start writing the Comfiguration data.
	ccb.confg_cmnd_pyld_offset = ccb.basic_config.count_of_dir_entries * ccb.config_dir.get_dir_entry_size()

	if args.basicconf:
		# Capture the parameters in the basic configuration.
		for item in args.basicconf[0].split(','):
			key, value = item.split('=')
			key.strip()  # Remove leading/trailing whitespace from key
			key = key.lower() # Make key lowercase to support case-insensitive keys
			value.strip()  # Remove leading/trailing whitespace from value
			if key == 'layoutversion':
				ccb.basic_config.layout_version = parse_int(value)
			elif key == 'vendorname':
				ccb.basic_config.vendor_name = pad_string(value, 16)
			elif key == 'modelno':
				ccb.basic_config.model_name = pad_string(value, 16)
			elif key == 'driverversion':
				ccb.basic_config.driver_version = pad_string(value, 16)
			elif key == 'interfaces':
				try:
					ccb.basic_config.camera_comm_intf = int(value, 0)
				except ValueError:
					# If the interfaces are provided as a string, map them to an integer.
					if value == 'i2c':
						ccb.basic_config.camera_comm_intf = ccb.basic_config.camera_comm_intf | CameraInterfaces.I2C
					elif value == 'gpio':
						ccb.basic_config.camera_comm_intf = ccb.basic_config.camera_comm_intf | CameraInterfaces.GPIO
					elif value == 'mipi':
						ccb.basic_config.camera_comm_intf = ccb.basic_config.camera_comm_intf | CameraInterfaces.MIPI
			else:
				raise ValueError(f"Unknown runtime configuration key: {key}")

	# Add first entry to the directory which is the basic configuration.
	ccb.write_camera_info_to_file(CameraActionUID.BASIC_CAMERA_CONFIG, ccb.basic_config.get_configuration())

	# If any other camera related information is to be added in future, then it
	# should be added here.

	# Now add the Camera Configuration Commands to the directory.
	for arg, arg_value in vars(args).items():
		if arg == 'camconfcmnd' and arg_value:
			for ccc_entry in arg_value:
				ccd_id, ccd_file = ccc_entry.split('=')
				try:
					ccd_id = parse_int(ccd_id)
				except argparse.ArgumentTypeError:
					# The Camera configuration command ID is provided as a
					# string, get its enum value.
					ccd_id = CameraActionUID.get_enum_value(ccd_id)

				# Generate bin file if it's a text file, else use as is
				if ccd_file.lower().endswith('.txt'):
					base_name = os.path.splitext(os.path.basename(ccd_file))[0]
					bin_file = f"{base_name}_i2c_config.bin"
					I2CBinGenerator().generate_bin(ccd_file, bin_file)
					ccb.write_camera_config_to_file(ccd_id, bin_file)
				else:
					ccb.write_camera_config_to_file(ccd_id, ccd_file)

	# At this point we have written the configuration commands and data to the
	# output file. In the process we also built the directory in-memory. We
	# write this directory to the start of the output file.
	ccb.write_cc_dir_to_file()

if __name__ == "__main__":

	# Parse the command line arguments using argparse.
	args = parser.parse_args()

	# If output file is provided then run the routines within this script.
	with open(args.outfile, "w+b") as of:
		ccb = CameraConfigBuilder(of)
		write_camera_config_layout_from_args(args, ccb)

	print (f"Done creating Camera Configuration in {args.outfile}.")

