
################################################################################
# This file contains the implementation of a YAML parser to read configuration
# details for generating a temporary image file. The parser reads a YAML file
# and provides an interface to access the configuration data.
################################################################################

import yaml
import sys
import os

# Configuration Parser Class
class ConfigParser:
    def __init__(self, config_file: str = "config.yaml"):
        self.config_file = config_file
        self.config_data = {}

        self.outfile = ""
        self.force = False
        self.merge_with_existing_cfg = False
        self.ml_networks = {}
        self.app_profiles = {}
        self.gard_firmware = {}
        self.camera_configs = {}
        self.app_data = {}
        self.rfs_config = {}
        self.erase_block_alignment = "0x8000"
        self.rfs_dir_copies = 1

    # Load and parse the YAML configuration file
    def load_config(self) -> bool:
        try:
            if not os.path.exists(self.config_file):
                print(f"Error: Configuration file '{self.config_file}' not found.")
                return False

            with open(self.config_file, 'r') as f:
                self.config_data = yaml.safe_load(f)

            if not self.config_data:
                print(f"Error: Configuration file '{self.config_file}' is empty or invalid.")
                return False

            return self._parse_config()

        except yaml.YAMLError as e:
            print(f"Error parsing YAML file '{self.config_file}': {e}")
            return False
        except Exception as e:
            print(f"Error loading configuration from '{self.config_file}': {e}")
            return False

    # Parse the loaded configuration data
    def _parse_config(self) -> bool:
        try:
            # Parsing the basic configuration
            self.outfile = self.config_data.get('Output File', '')
            self.force = self.config_data.get('Overwrite Existing Config File', False)
            self.merge_with_existing_cfg = self.config_data.get('Merge With Existing Config', False)

            # Parsing the file mappings
            ml_raw = self.config_data.get('ML Networks', {})
            self.ml_networks = {str(k): v for k, v in ml_raw.items()}

            app_raw = self.config_data.get('App Profile', {})
            self.app_profiles = {str(k): v for k, v in app_raw.items()}

            gard_raw = self.config_data.get('Gard Firmwares', {})
            self.gard_firmware = {str(k): v for k, v in gard_raw.items()}

            camera_raw = self.config_data.get('Camera Configurations', {})
            self.camera_configs = {str(k): v for k, v in camera_raw.items()}

            app_data_raw = self.config_data.get('App Data', {})
            self.app_data = {str(k): v for k, v in app_data_raw.items()}

            # Parsing the RFS configuration
            rfs_config_raw = self.config_data.get('RFS Config', {})
            if rfs_config_raw:
                self.rfs_config = {}
                # Only include parameters that are explicitly provided in the YAML
                if 'Layout Version' in rfs_config_raw:
                    self.rfs_config['layout_version'] = rfs_config_raw['Layout Version']
                if 'Update Count' in rfs_config_raw:
                    self.rfs_config['update_count'] = rfs_config_raw['Update Count']
                if 'Control Field' in rfs_config_raw:
                    self.rfs_config['control_field'] = rfs_config_raw['Control Field']
                if 'Start Of Directory' in rfs_config_raw:
                    self.rfs_config['start_of_directory'] = rfs_config_raw['Start Of Directory']
                if 'Size Of Directory' in rfs_config_raw:
                    self.rfs_config['size_of_directory'] = rfs_config_raw['Size Of Directory']
                if 'Count Of Directory Entries' in rfs_config_raw:
                    self.rfs_config['count_of_directory_entries'] = rfs_config_raw['Count Of Directory Entries']
                if 'Size Of Directory Entry' in rfs_config_raw:
                    self.rfs_config['size_of_directory_entry'] = rfs_config_raw['Size Of Directory Entry']
                if 'UID Of Firmware To Boot' in rfs_config_raw:
                    self.rfs_config['uid_of_firmware_to_boot'] = rfs_config_raw['UID Of Firmware To Boot']

            # Parsing the advanced settings
            self.erase_block_alignment = str(self.config_data.get('Flash Erase Block Alignment', "0x8000"))
            self.rfs_dir_copies = self.config_data.get('RFS Dir Copies', 1)

            return True

        except Exception as e:
            print(f"Error parsing configuration data: {e}")
            return False

    # Generate command-line arguments based on the parsed configuration
    def get_cli_args(self):
        args = []

        if not self.merge_with_existing_cfg:
            args.append("-n")
        if self.force:
            args.append("-f")
        if self.outfile:
            args.append(f"-o {self.outfile}")

        for key, value in self.ml_networks.items():
            args.append(f"--mlfile {key}={value}")

        for key, value in self.camera_configs.items():
            args.append(f"--camconf {key}={value}")

        for key, value in self.gard_firmware.items():
            args.append(f"--gard {key}={value}")

        for key, value in self.app_profiles.items():
            args.append(f"--app {key}={value}")

        for key, value in self.app_data.items():
            args.append(f"--appdata {key}={value}")

        if self.rfs_config:
            rfs_args = []
            key_mapping = {
                'layout_version': 'LayoutVersion',
                'update_count': 'UpdateCount',
                'control_field': 'ControlField',
                'start_of_directory': 'StartOfDirectory',
                'size_of_directory': 'SizeOfDirectory',
                'count_of_directory_entries': 'CountOfDirectoryEntries',
                'size_of_directory_entry': 'SizeOfDirectoryEntry',
                'uid_of_firmware_to_boot': 'UIDOfFirmwareToBoot'
            }

            for key, value in self.rfs_config.items():
                mapped_key = key_mapping.get(key, key)
                if key == 'control_field':
                    rfs_args.append(f"{mapped_key}=Config-valid")
                    rfs_args.append(f"{mapped_key}=crc-not-valid")
                else:
                    rfs_args.append(f"{mapped_key}={value}")

            if rfs_args:  # Only add rfsconf if there are actual parameters
                args.append(f"--rfsconf {','.join(rfs_args)}")

        if self.erase_block_alignment:
            args.append(f"--erase-block-alignment {self.erase_block_alignment}")

        if self.rfs_dir_copies:
            args.append(f"--dir-copies {self.rfs_dir_copies}")

        return " ".join(args)

    # Print the current configuration and generated CLI arguments
    def print_config(self):
        # Printing the parsed configuration
        print("=== Parsed Configuration ===")
        print(f"Output File: {self.outfile}")
        print(f"Force: {self.force}")
        print(f"Generate New: {self.merge_with_existing_cfg}")

        print(f"\nML Networks: {self.ml_networks}")
        print(f"App Profiles: {self.app_profiles}")
        print(f"Gard Firmware: {self.gard_firmware}")
        print(f"Camera Configs: {self.camera_configs}")
        print(f"App Data: {self.app_data}")

        print(f"RFS Config: {self.rfs_config}")

        print(f"\nAdvanced Settings:")
        print(f"Erase Block Alignment: {self.erase_block_alignment}")
        print(f"RFS Dir Copies: {self.rfs_dir_copies}")
        print(self.get_cli_args())

def main():
    if len(sys.argv) > 1:
        config_file = sys.argv[1]
    else:
        config_file = "config.yaml"

    parser = ConfigParser(config_file)

    if parser.load_config():
        parser.print_config()
    else:
        print("Failed to load configuration.")
        sys.exit(1)


if __name__ == "__main__":
    main()
