################################################################################
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
################################################################################


################################################################################
# This file contains code which is used to build the image that is then written
# onto the flash present on the GARD SoM.
# The output file is not the complete flash image, but the part in flash which
# contains the various firmwares and Configurations. The format of this block
# is described in the file GARD Flash Layout.xlsx which is present in folder
# docs.
# This script will be run on the Host PC where the User has the firmwares
# and configuration files present. If these files need to be encrypted and/or if
# signature is to be generated then it needs to have been done before and the
# details of which are not covered in this script.
# This script can identify the following types of files:
# - Firmware file
# - ML firmware files
# - Camera Configuration files
# - Application Profiles.
# This script also takes in an existing image file (if present) and tries to
# embed the new entities within the same image file. If the image file is not
# found then it will create a new image file. If the firmware/configuration is
# already present in the image file then the script will either overwrite or
# skip based on the User's input. If the flag -f --force is used then the script
# will overwrite the existing firmware/configuration without prompting the User.
# If the flag -n --generate-new is used then the script will generate a new
# image file instead of modifying the existing one.
# The script can take in the configuration details from a YAML file. The YAML
# file should contain all the necessary information about the firmware, ML
# models, camera configurations, and application profiles that need to be
# included in the generated image.
# If the YAML file is not provided, the script will rely on command-line
# arguments to obtain the required configuration details.
# With YAML file provided as well, the user can override/append some of the
# parameters using command-line arguments.
################################################################################

import argparse
import os
import sys
import struct
from enum import IntEnum
import shlex


# This enum defines the ID ranges of the various modules that are currently
# supported. Make sure this one matches with the enum MODULE_ID present in file
# flashmap.h
class ModuleType(IntEnum):
	EMPTY_ENTRY 			= 0

	BAD_BLOCK 				= 0x1

	MODULE_FW_START_ID 		= 0x1001
	MODULE_FW_END_ID 		= 0x1FFE
	MODULE_FW_TEMPORARY 	= 0x1FFF

	MODULE_ML_START_ID 		= 0x2001
	MODULE_ML_END_ID 		= 0x2FFF

	MODULE_CAM_CFG_START_ID 	= 0x3001
	MODULE_CAM_CFG_END_ID 	= 0x3FFF

	MODULE_APP_PROF_START_ID 	= 0x4001
	MODULE_APP_PROF_END_ID 		= 0x4FFF

	MODULE_APP_DATA_START_ID = 0x5001
	MODULE_APP_DATA_END_ID   = 0x5FFF

	MODULE_UNUSED_START_ID 	= 0x6001
	MODULE_UNUSED_END_ID 		= 0xFFFF

class ControlField(IntEnum):
	# This enum defines the control field values which are used in the rfs
	# configuration. The control field is a bitmask which indicates the
	# validity of the configuration.
	VALID_CONFIG = 0xFFFFFFFE  # Indicates that the configuration is valid.
	VALID_CRC = 0xFFFFFFFFFD   # Indicates that the CRC of the configuration is valid.

def parse_int(value):
	try:
		return int(value, 0)
	except ValueError:
		raise argparse.ArgumentTypeError(f"Invalid integer value: {value}")

# Setup command-line argument parser.
parser = argparse.ArgumentParser(description='Image Builder for GARD SoM', formatter_class=argparse.RawTextHelpFormatter)

parser.add_argument('-y', '--yaml', metavar="FILE", type=str, required=False,
					help='YAML file which contains all the configuration details. If this is not provided, other command-line arguments must be provided. You can use this to provide a base configuration and then override/append some of the parameters using other command-line arguments.')
parser.add_argument('-o', '--outfile', metavar="FILE", type=str, required=False,
					help='Binary file which contains the built image')
parser.add_argument('-f', '--force', action='store_true', default=False,
					help='Overwrite existing firmware/configuration in the image file')
parser.add_argument('-n', '--new', action='store_true', default=False,
					help=""""Create a new image file instead of updating an existing one. If an existing image file is present then its previous contents will be lost.""")
parser.add_argument('-m', '--mlfile', action="append", type=str, required=False,
					help="""ML-ID=File containing the ML firmware. Thus if you need to provide ML3 firmware file named ml3_fw.bin then the parameter would be:
					--mlfile 0x2003=ml3_firmware.bin""")
parser.add_argument('-a', '--app', action="append", type=str, required=False,
					help="""APP-ID=File containing Application Profile. Thus if you need to provide App Profile 6 file named app6.bin then the parameter would be:
					--app 0x4006=app6.bin""")
parser.add_argument('-ad', '--appdata', action="append", type=str, required=False,
					help="""App-Module-ID=file containing the Application Data. Thus, if you need to provide the App Data contained in file app_data5.bin then the parameter would be:
					--appdata 0x5005=app_data5.bin""")
parser.add_argument('-g', '--gard', action="append", type=str, required=False,
					help="""GARD-ID=File containing GARD firmware.' Thus if you need to provide GARD firmware file named gard_fw.bin then the parameter would be:
					--gard 0x1004=gard_fw4.bin""")
parser.add_argument('-c', '--camconf', action="append", type=str, required=False,
					help="""CAM-CFG=File containing Camera Configuration. Thus if you need to provide Camera Configuration 8 file named cam_config8.bin then the parameter would be:
					--camconf 0x3008=cam_config8.bin""")
parser.add_argument('-r', '--rfsconf', action='append', type=str, required=False,
					help="""Provide a comma-separated list of configuration details.
					Following is one example: --rfsconf="LayoutVersion=1,UpdateCount=4,ControlField=(config-valid | config-not-valid),
					ControlField=(crc-valid | crc-not-valid),StartOfDirectory=0x10000,SizeOfDirectory=0x2000,
					CountOfDirectoryEntries=10,SizeOfDirectoryEntry=12, UIDOfFirmwareToBoot=0x1001"
					Currently following configuration details are supported:
					- LayoutVersion: Version of the Configuration and Directory layout.
					- UpdateCount: Number of updates that have been applied to either the Configuration or the Directory or both.
					- ControlField: This is a bitmask which indicates the validity of the configuration. It can be one of the following:
						- Config-Valid (Default): Indicates that the configuration is valid.
						- Config-Not-Valid: Indicates that the configuration is not valid.
						- CRC-Valid: Indicates that the CRC of the configuration is valid.
						- CRC-Not-Valid (Default): Indicates that the CRC of the configuration is not valid.
					- StartOfDirectory: Address in the Flash where the Directory starts. This address must
					be aligned to the erase block size. This is an optional argument as the script will place the
					directory in the next alignment address after the configuration space.
					- UIDOfFirmwareToBoot: UID of the firmware that should be booted by the GARD SoM.
					Note that not all values are not checked for correctness, so make sure you provide the correct
					values." """)
parser.add_argument('--erase-block-alignment', type=parse_int, required=False, default=0x8000,
					help="""Alignment to use for placing the Directory and Modules. This alignment is based on the Minimum granularity supported by the Flash for an Erase operation. The default value is 0x8000 (32KB)""")
parser.add_argument('--dir-copies', type=parse_int, required=False, default=1,
					help="""Number of Directory copies that will be created in the Image file..""")

class FlashLayout:
	# This class defines the layout of the flash memory for the GARD SoM.
	# Refer to file "GARD Flash Layout.xlsx" for details.

	# Every module (including directory) in the flash memory is aligned to 64 KiB.
	# Make sure this is a power-of-two for the function align_value() to work.
	# This default can be overridden by the User using the command-line
	# argument --erase-block-alignment.
	ALIGNMENT_VALUE = 64 * 1024  # 64 KiB alignment for module data

	outfile = None  # Output file where the image will be written
	outfile_offset = 0  # Offset in the output file where the next write will happen.

	configuration_address = 0 # Default address in flash where the configuration is written.
	dir1_offset = 0
	dir_copies = 1
	module_offset = dir1_offset + ALIGNMENT_VALUE  # 64 KiB offset for the directory

	# Routine to align a value to the nearest multiple of ALIGNMENT_VALUE.
	def align_value(self, value):
		return (value + self.ALIGNMENT_VALUE - 1) & ~(self.ALIGNMENT_VALUE - 1)

	class RfsConfiguration:
		# This class defines the rfs configuration which is written to the
		# output file. The rfs configuration is written at the start of the
		# output file and it contains information about the complete image file

		# Rfs configuration is of the following format:
		# Signature (4 bytes)
		# Layout Version (4 bytes)
		# Update Version (4 bytes)
		# CRC (4 bytes)
		# Control Field (4 bytes)
		# Start Of Directory (4 bytes)
		# Size Of Directory (4 bytes)
		# Count Of Directory Entries (4 bytes)
		# Size of Directory Entry (4 bytes)
		# UID Of Firmware to Boot (4 bytes)
		RUNTIME_CONF_FORMAT = "IIIIIIIIII"
		def __init__(self):
			# Initialize the rfs configuration with default values.
			self.signature = 0x4343534C
			self.layout_version = 1
			self.update_count = 1
			self.control_field = 0xFFFFFFFF  # Default control field value
			self.control_field = ControlField.VALID_CONFIG
			self.start_of_directory = FlashLayout.dir1_offset
			self.size_of_directory = FlashLayout.ALIGNMENT_VALUE
			self.count_of_directories = 1
			self.size_of_directory_entry = 12
			self.uid_of_firmware_to_boot = ModuleType.MODULE_FW_START_ID

		def get_configuration(self):
			# Pack the rfs configuration into a bytes object.
			return struct.pack(
				self.RUNTIME_CONF_FORMAT,
				self.signature,
				self.layout_version,
				self.update_count,
				0,  # CRC is not calculated here, it will be calculated later
				self.control_field,
				self.start_of_directory,
				self.size_of_directory,
				self.count_of_directories,
				self.size_of_directory_entry,
				self.uid_of_firmware_to_boot
			)

	class Directory:
		# This class defines the directory structure within the flash memory.
		# This class just accumulates the directory entries in a list and returns
		# the list to the caller of get_dir_entries() method. It is the caller's
		# responsibility to write the directory entries to the output file.
		# This class provides the following help:
		# * Returns the directory entries list in a format which can be directly
		# 	written to the output file.
		# * Ensures that the UID of each entry is unique.


		# Each directory entry is of the following format:
		# UID (4 bytes)
		# Offset (4 bytes)
		# Size (4 bytes)
		# Below is the same format defined using struct module.
		DIR_ENTRY_FORMAT = "III"

		first_UID = None
		dir_entries = []  # List to hold directory entries

		# add_entry() adds a new entry to an internal list.
		def add_entry(self, uid, offset, size):

			# Ensure this UID is unique in the directory.
			if any(struct.unpack("III", entry)[0] == uid for entry in self.dir_entries):
				raise ValueError(f"UID {uid} is passed multiple times on command line. Exiting")

			# Pack and write the directory entry to the list.
			entry = struct.pack(self.DIR_ENTRY_FORMAT, uid, offset, size)
			self.dir_entries.append(entry)

		# get_dir_entries() return the directory entries as a bytes object which
		# can be written to the output file.
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


	# WriteModuleToFile() writes a module to the output file and updates the
	# directory.
	def WriteModuleToFile(self, uid, infile):
		module_size = os.path.getsize(infile)
		module_start_offset = self.module_offset

		self.outfile.seek(module_start_offset)

		with open(infile, "rb") as f:
			self.outfile.write(f.read())

		# Move module offset to the next position which is a multiple of
		# ALIGNMENT_VALUE
		self.module_offset = self.align_value(self.outfile.tell())

		# Once the module content is written, add the entry to the directory.
		self.dir_instance.add_entry(uid, module_start_offset - self.dir1_offset, module_size)

	# AddMLFirmware() write the ML firmware file to the output file and it also
	# updates the directory with the ML firmware entry.
	def AddMLFirmware(self, ml_id, ml_file):
		# Ensure the ML ID is within the valid range.
		if not (ModuleType.MODULE_ML_START_ID <= ml_id <= ModuleType.MODULE_ML_END_ID):
			raise ValueError(f"""Invalid ML ID {ml_id}. Valid range is
							 {ModuleType.MODULE_ML_START_ID} to
							 {ModuleType.MODULE_ML_END_ID}. Exiting.""")

		if not os.path.exists(ml_file):
			raise ValueError(f"ML firmware file '{ml_file}' does not exist. Exiting.")

		self.WriteModuleToFile(ml_id, ml_file)

	# AddCameraConfiguration() writes the Camera Configuration file to the output
	# file and it also updates the directory with the Camera Configuration entry.
	def AddCameraConfiguration(self, camconf_id, camconf_file):
		# Ensure the Camera Configuration ID is within the valid range.
		if not (ModuleType.MODULE_CAM_CFG_START_ID <= camconf_id <= ModuleType.MODULE_CAM_CFG_END_ID):
			raise ValueError(f"""Invalid Camera Configuration ID {camconf_id}. Valid range is
							 {ModuleType.MODULE_CAM_CFG_START_ID} to
							 {ModuleType.MODULE_CAM_CFG_END_ID}. Exiting.""")

		if not os.path.exists(camconf_file):
			raise ValueError(f"Camera Configuration file '{camconf_file}' does not exist. Exiting.")

		self.WriteModuleToFile(camconf_id, camconf_file)

	# AddAppProfile() writes the Application Profile file to the output file
	# and it also updates the directory with the Application Profile entry.
	def AddAppProfile(self, app_id, app_file):
		# Ensure the App Profile ID is within the valid range.
		if not (ModuleType.MODULE_APP_PROF_START_ID <= app_id <= ModuleType.MODULE_APP_PROF_END_ID):
			raise ValueError(f"""Invalid App Profile ID {app_id}. Valid range is
							 {ModuleType.MODULE_APP_PROF_START_ID} to
							 {ModuleType.MODULE_APP_PROF_END_ID}. Exiting.""")

		if not os.path.exists(app_file):
			raise ValueError(f"App Profile file '{app_file}' does not exist. Exiting.")

		self.WriteModuleToFile(app_id, app_file)

	# AddGARDFirmware() writes the GARD firmware file to the output file
	# and it also updates the directory with the GARD firmware entry.
	def AddGARDFirmware(self, gard_id, gard_file):
		# Ensure the GARD firmware ID is within the valid range.
		if not (ModuleType.MODULE_FW_START_ID <= gard_id <= ModuleType.MODULE_FW_END_ID):
			raise ValueError(f"""Invalid GARD FW ID {gard_id}. Valid range is
							 {ModuleType.MODULE_FW_START_ID} to
							 {ModuleType.MODULE_FW_END_ID}. Exiting.""")

		if not os.path.exists(gard_file):
			raise ValueError(f"GARD Firmware file '{gard_file}' does not exist. Exiting.")

		self.WriteModuleToFile(gard_id, gard_file)

	# AddAppData() writes the Application Data file to the output file
	# and it also updates the directory with the Application Data entry.
	def AddAppData(self, app_data_id, app_data_file):
		# Ensure the App Data ID is within the valid range.
		if not (ModuleType.MODULE_APP_DATA_START_ID <= app_data_id <= ModuleType.MODULE_APP_DATA_END_ID):
			raise ValueError(f"""Invalid APP_DATA ID {app_data_id}.
					Valid range is {ModuleType.MODULE_APP_DATA_START_ID}
					to {ModuleType.MODULE_APP_DATA_END_ID}. Exiting.""")

		if not os.path.exists(app_data_file):
			raise ValueError(f"App Data file '{app_data_file}' does not exist. Exiting.")

		if (os.path.getsize(app_data_file) % 4) != 0:
			raise ValueError(f"App Data file '{app_data_file}' size is not aligned to 4 bytes. Exiting.")
		self.WriteModuleToFile(app_data_id, app_data_file)

	# WriteConfigurationToFile() writes the rfs configuration to the output
	# file.
	def WriteConfigurationToFile(self):
		# Write the rfs configuration to the output file.
		self.outfile.seek(self.configuration_address)
		config_bytes = self.config_instance.get_configuration()
		self.outfile.write(config_bytes)

		# Move the module offset to the next position which is a multiple of
		# ALIGNMENT_VALUE
		self.module_offset = self.align_value(self.outfile.tell())

	# WriteDirectoryToFile() writes 1 or multiple copies of the directory to
	# the output file.
	def WriteDirectoryToFile(self):
		# Write the first instance of directory to the output file.
		self.outfile.seek(self.dir1_offset)
		self.outfile.write(self.dir_instance.get_dir_entries())
		self.dir_copies = self.dir_copies - 1

		while self.dir_copies > 0:
			# Align the next directory offset to the erase block size.
			self.dir1_offset = self.align_value(self.outfile.tell())

			# Write the second instance of directory to the output file.
			self.dir_copies = self.dir_copies - 1

			self.outfile.seek(self.dir1_offset)
			self.outfile.write(self.dir_instance.get_dir_entries())

		# The modules are already written to the output file, when they were
		# added.

	def __init__(self, outfile):
		self.outfile = outfile
		self.dir_instance = self.Directory()
		self.config_instance = self.RfsConfiguration()


def write_flash_layout_from_args(args, fl):
	# All YAML processing is now done in main(), so we just process the final args

	if args.erase_block_alignment:
		# Ensure that the alignment value is a power of two.
		if (args.erase_block_alignment & (args.erase_block_alignment - 1)) != 0:
			raise ValueError(f"Alignment value {args.erase_block_alignment} is not a power of two. Exiting.")
		fl.ALIGNMENT_VALUE = args.erase_block_alignment
		fl.dir1_offset = fl.ALIGNMENT_VALUE
	if args.dir_copies:
		# Ensure that the number of directory copies is at least 1.
		if args.dir_copies < 1:
			raise ValueError(f"Number of directory copies {args.dir_copies} is less than 1. Exiting.")
		fl.dir_copies = args.dir_copies
		fl.config_instance.count_of_directories = fl.dir_copies
		fl.module_offset = fl.dir1_offset + (fl.ALIGNMENT_VALUE * fl.dir_copies)

	for arg, arg_value in vars(args).items():
		if arg == 'mlfile' and arg_value:
			for ml_entry in arg_value:
				ml_id, ml_file = ml_entry.split('=')
				ml_id = int(ml_id, 0)
				fl.AddMLFirmware(ml_id, ml_file)
		elif arg == 'app' and arg_value:
			for app_entry in arg_value:
				app_id, app_file = app_entry.split('=')
				app_id = int(app_id, 0)
				fl.AddAppProfile(app_id, app_file)
		elif arg == 'camconf' and arg_value:
			for camconf_entry in arg_value:
				camconf_id, camconf_file = camconf_entry.split('=')
				camconf_id = int(camconf_id, 0)
				fl.AddCameraConfiguration(camconf_id, camconf_file)
		elif arg == 'gard' and arg_value:
			for gard_entry in arg_value:
				gard_id, gard_file = gard_entry.split('=')
				gard_id = int(gard_id, 0)
				fl.AddGARDFirmware(gard_id, gard_file)
		elif arg == 'config_addr_in_flash' and arg_value:
			fl.configuration_address = arg_value
		elif arg == 'rfsconf' and arg_value:
			# Parse the rfs configuration details from the provided string.
			control_field = 0xFFFFFFFF  # Default control field value
			for item in arg_value[0].split(','):
				key, value = item.split('=')
				key.strip()  # Remove leading/trailing whitespace from key
				key = key.lower() # Make key lowercase to support case-insensitive keys
				value.strip()  # Remove leading/trailing whitespace from value
				value = value.lower()  # Make value lowercase to support case-insensitive values

				if key == 'layoutversion':
					fl.config_instance.layout_version = parse_int(value)
				elif key == 'updatecount':
					fl.config_instance.update_count = parse_int(value)
				elif key == 'controlfield':
					try:
						control_field = int(value, 0)
						fl.config_instance.control_field = control_field
					except ValueError:
						# If the control field is provided as a string, map it to an integer.
						if (value == 'Config-VALID'.lower()):
							fl.config_instance.control_field = fl.config_instance.control_field & ControlField.VALID_CONFIG
						elif (value == 'CRC-VALID'.lower()):
							fl.config_instance.control_field = fl.config_instance.control_field & ControlField.VALID_CRC
				elif key == 'startofdirectory':
					fl.config_instance.start_of_directory = parse_int(value)
					if (fl.config_instance.start_of_directory % fl.ALIGNMENT_VALUE) != 0:
						raise ValueError(f"Start of directory {fl.config_instance.start_of_directory} is not aligned to {fl.ALIGNMENT_VALUE} bytes.")
					elif (fl.config_instance.start_of_directory < fl.dir1_offset):
						raise ValueError(f"Start of directory {fl.config_instance.start_of_directory} is less than the first directory offset {fl.dir1_offset}.")
				elif key == 'uidoffirmwaretoboot':
					fl.config_instance.uid_of_firmware_to_boot = parse_int(value)
					fl.dir_instance.first_UID = fl.config_instance.uid_of_firmware_to_boot
				else:
					raise ValueError(f"Unknown rfs configuration key: {key}")
		elif arg == 'appdata' and arg_value:
			for app_data_entry in arg_value:
				app_data_id, app_data_file = app_data_entry.split('=')
				app_data_id = int(app_data_id, 0)
				fl.AddAppData(app_data_id, app_data_file)

	fl.config_instance.start_of_directory = fl.dir1_offset
	fl.config_instance.size_of_directory = len(fl.dir_instance.dir_entries) * fl.config_instance.size_of_directory_entry

	# Once all the modules have been written to the output file, write the
	# configuration followed by the directory entries needed to access them by
	# the firmware.
	fl.WriteConfigurationToFile()
	fl.WriteDirectoryToFile()

if __name__ == "__main__":
	args = parser.parse_args()

	# See if YAML File is provided or not
	if args.yaml:
		# Import the ConfigParser class from RootImageBuilderConfigParser module only if yaml file is provided.
		from RootImageBuilderConfigParser import ConfigParser
		config_parser = ConfigParser(args.yaml)
		# Load the configuration from the YAML file and check for errors. If not then parse it, the output would be a dictionary of YAML contents.
		if not config_parser.load_config():
			print(f"Failed to load configuration from {args.yaml}.")
			sys.exit(1)

		# Now Update the YAML configuration with CLI arguments if present.
		# The CLI arguments will override or append if not present in the YAML configuration.
		if args.outfile:
			config_parser.outfile = args.outfile
		if args.force:
			config_parser.force = args.force
		if args.new:
			config_parser.merge_with_existing_cfg = not args.new
		if args.erase_block_alignment != 0x8000:
			config_parser.erase_block_alignment = str(args.erase_block_alignment)
		if args.dir_copies != 1:
			config_parser.rfs_dir_copies = args.dir_copies

		if args.mlfile:
			for ml_entry in args.mlfile:
				ml_id, ml_file = ml_entry.split('=')
				config_parser.ml_networks[ml_id] = ml_file
		if args.app:
			for app_entry in args.app:
				app_id, app_file = app_entry.split('=')
				config_parser.app_profiles[app_id] = app_file
		if args.gard:
			for gard_entry in args.gard:
				gard_id, gard_file = gard_entry.split('=')
				config_parser.gard_firmware[gard_id] = gard_file
		if args.camconf:
			for camconf_entry in args.camconf:
				camconf_id, camconf_file = camconf_entry.split('=')
				config_parser.camera_configs[camconf_id] = camconf_file

		if args.rfsconf:
			for rfs_entry in args.rfsconf:
				for item in rfs_entry.split(','):
					key, value = item.split('=')
					key = key.strip().lower()
					value = value.strip()

					if key == 'layoutversion':
						config_parser.rfs_config['layout_version'] = int(value, 0)
					elif key == 'updatecount':
						config_parser.rfs_config['update_count'] = int(value, 0)
					elif key == 'controlfield':
						config_parser.rfs_config['control_field'] = value
					elif key == 'startofdirectory':
						config_parser.rfs_config['start_of_directory'] = int(value, 0)
					elif key == 'sizeofdirectory':
						config_parser.rfs_config['size_of_directory'] = int(value, 0)
					elif key == 'countofdirectoryentries':
						config_parser.rfs_config['count_of_directory_entries'] = int(value, 0)
					elif key == 'sizeofdirectoryentry':
						config_parser.rfs_config['size_of_directory_entry'] = int(value, 0)
					elif key == 'uidoffirmwaretoboot':
						config_parser.rfs_config['uid_of_firmware_to_boot'] = int(value, 0)

		# Now get the final command string from updated config
		#So we first made the changes to config_parser and then we get the final args string from it.
		# This final args string is then parsed by argparse to get the final args object.
		final_args_str = config_parser.get_cli_args()
		args = parser.parse_args(shlex.split(final_args_str))

	if not args.outfile:
		parser.error("the following arguments are required: -o/--outfile (must be in YAML or command line)")

	if not args.new:
		exit ("Currently only new image file creation is supported. Use -n/--new flag or set GenerateNew: true in YAML file.")

	with open(args.outfile, "w+b") as of:
		fl = FlashLayout(of)
		write_flash_layout_from_args(args, fl)

	print (f"Done filling entries in {args.outfile}.")
