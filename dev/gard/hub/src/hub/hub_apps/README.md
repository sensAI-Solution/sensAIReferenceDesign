# HUB APPS

HUB Applications Repository

## HUB C Applications

### Hub Minimal App - app.c

The application showcases hub's features — register and data read/write operations, and the display of temperature and energy sensors data.

#### Prerequisites

1.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    2.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.

#### Usage

##### Development mode

1.  Change directory to `HUB/build`.
2.  Run the make target `run_hub_minimal_app` with command `make run_hub_minimal_app`.

##### Production mode

1.  Change directory to `/opt/hub/`.
2.  Run "hub_app_minimal.elf".
    1.  .elf file is an executable file of streaming app.
    2.  It takes two command line arguments.
        1.  host_config.json file - contains configuration of host.
        2.  GARD config files path - contains supported GARD configs.
    3.  Refer to samples at /opt/hub/config.
    4.  Command - `./bin/hub_app_minimal.elf ./config/host_config.json ./config/`
3.  Use `Ctrl+C` to terminate the hub app.


### Streaming App - streaming_app.c

The application monitors live events and uses a user-given callback to populate the provided buffer with metadata.
Refer to [design document](../../../docs/GPIO%20C%20Event%20Handler.svg) for more details.

#### Prerequisites

1.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    2.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.

#### Usage

##### Development mode

1.  Change directory to `HUB/build`.
2.  Run the make target `run_hub_streaming_app` with command `make run_hub_streaming_app`.

##### Production mode

1.  Change directory to `/opt/hub/`.
2.  Run "hub_app_streaming.elf".
    1.  .elf file is an executable file of streaming app.
    2.  It takes two command line arguments.
        1.  host_config.json file - contains configuration of host.
        2.  GARD config files path - contains supported GARD configs.
    3.  Refer to samples at /opt/hub/config.
    4.  Command - `./bin/hub_app_streaming.elf ./config/host_config.json ./config/`
3.  Use `Ctrl+C` to terminate the hub app.

### Image Operations (rescaled image capture) App - img_ops_app.c

The application captures a rescaled image output from the GARD's ISP, and saves it locally as a BMP file.

#### Prerequisites

1.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    2.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.

#### Usage

##### Development mode

1.  Change directory to `HUB/build`.
2.  Run the make target `run_hub_img_ops_app` with command `make run_hub_img_ops_app`.

##### Production mode

1.  Change directory to `/opt/hub/`.
2.  Run "hub_app_img_ops.elf".
    1.  .elf file is an executable file of img_ops app.
    2.  It takes two command line arguments.
        1.  host_config.json file - contains configuration of host.
        2.  GARD config files path - contains supported GARD configs.
        3.  Directory to save the output BMP file.
    3.  Refer to samples at /opt/hub/config.
    4.  Command - `./bin/hub_app_img_ops.elf ./config/host_config.json ./config/ ~/.`
    5. If successful, the BMP file of the image captured will be saved in the user's home directory.

## HUB Python Applications

### Python Minimal App (app.py)

This is a demo application featuring usage of HUB interfaces.

#### Prerequisites
1.  Python 3.10 or above is installed.
2.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    3.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.

#### Usage

##### METHOD 1 - As a make target

1.  Create a python virtual environment using following commands:
    1.  Go to directory where you want to create a virtual environment. Preferably HUB/.
    2.  Run `python -m venv <env_name>` to create.
    3.  Run `source <env_name>/bin/activate` to use created environment.
2.  Install python wheel package in the activated virtual environment.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the wheel package from 'Assets' - it would be name lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl
        For example for version 1.5.0, the file name is lscc_hub-1.5.0-py3-none-manylinux_2_28_arm64.whl
    3.  Run `pip install <path_to_package>/lscc_hub-<version>-py3-none-any.whl` to install package.
3.  Git clone [HUB repository](https://github.com/LSCC-Architecture/HUB/)
4.  Go to to HUB/build.
5.  Run the make target `make run_hub_minimal_py`

##### METHOD 2 - As an independent app

1.  Create a python virtual environment using following commands:
    1.  Go to directory where you want to create a virtual environment. Preferably HUB/.
    2.  Run `python -m venv <env_name>` to create.
    3.  Run `source <env_name>/bin/activate` to use created environment.
2.  Install python wheel package in the activated virtual environment.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the wheel package from 'Assets' - it would be name lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl
        For example for version 1.5.0, the file name is lscc_hub-1.5.0-py3-none-manylinux_2_28_arm64.whl
    3.  Run `pip install <path_to_package>/lscc_hub-<version>-py3-none-any.whl` to install package.
3.  Go to src/hub/hub_apps directory.
    1.  Git clone [HUB repository](https://github.com/LSCC-Architecture/HUB/)
    2.  Go to to HUB/src/hub/hub_apps.
4.  Run `python app.py`
5.  Run `deactivate` to come out of environment after usage.


### Python Live Data Streaming App (streaming_app.py)

This is a demo application for live data streaming. When an event occurs, the user given callback function
is triggered with data in buffer. Refer to [design document](../../../docs/GPIO%20PY%20Event%20Handler.svg) for more details.

#### Prerequisites
1.  Python 3.10 or above is installed.
2.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    3.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.

#### Usage

##### METHOD 1 - As a make target

1.  Create a python virtual environment using following commands:
    1.  Go to directory where you want to create a virtual environment. Preferably HUB/.
    2.  Run `python -m venv <env_name>` to create.
    3.  Run `source <env_name>/bin/activate` to use created environment.
2.  Install python wheel package in the activated virtual environment.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the wheel package from 'Assets' - it would be name lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl
        For example for version 1.5.0, the file name is lscc_hub-1.5.0-py3-none-manylinux_2_28_arm64.whl
    3.  Run `pip install <path_to_package>/lscc_hub-<version>-py3-none-any.whl` to install package.
3.  Git clone [HUB repository](https://github.com/LSCC-Architecture/HUB/)
4.  Go to to HUB/build.
5.  Run the make target `make run_hub_streaming_py`

##### METHOD 2 - As an independent app

1.  Create a python virtual environment using following commands:
    1.  Go to directory where you want to create a virtual environment. Preferably HUB/.
    2.  Run `python -m venv <env_name>` to create.
    3.  Run `source <env_name>/bin/activate` to use created environment.
2.  Install python wheel package in the activated virtual environment.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the wheel package from 'Assets' - it would be name lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl
        For example for version 1.5.0, the file name is lscc_hub-1.5.0-py3-none-manylinux_2_28_arm64.whl
    3.  Run `pip install <path_to_package>/lscc_hub-<version>-py3-none-any.whl` to install package.
3.  Go to src/hub/hub_apps directory.
    1.  Git clone [HUB repository](https://github.com/LSCC-Architecture/HUB/)
    2.  Go to to HUB/src/hub/hub_apps.
4.  Run `python streaming_app.py`
5.  Run `deactivate` to come out of environment after usage.


### Python App - EdgeHUB

A web application for real-time graphical visualization of sensor data.

#### Prerequisites

1.  Python 3.10 or above is installed.
2.  Install libhub Debian package.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the debian package from 'Assets' - it would be named: lscc-hub_<hub_version>_arm64.deb
        For example for version 1.5.0, the file name is lscc-hub_1.5.0_arm64.deb
    3.  Run `sudo apt install <path_to_package>/lscc-hub_<version>_arm64.deb -y` to install the package.
3.  Create a python virtual environment using following commands:
    1.  Go to directory where you want to create a virtual environment. Preferably HUB/.
    2.  Run `python -m venv <env_name>` to create.
    3.  Run `source <env_name>/bin/activate` to use created environment.
4.  Install python wheel package in the activated virtual environment.
    1.  Go to GITHUB [releases of HUB](https://github.com/LSCC-Architecture/HUB/releases)
    2.  Download the wheel package from 'Assets' - it would be name lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl
        For example for version 1.5.0, the file name is lscc_hub-1.5.0-py3-none-manylinux_2_28_arm64.whl
    3.  Run `pip install <path_to_package>/lscc_hub-<hub_version>-py3-none-manylinux_2_28_arm64.whl` to install package.
5.  Install python package requirements of edgeHUB.
    1.  Go to `HUB/src/hub/hub_apps/edgeHUB`
    2.  Install requirements using `pip install -r requirements.txt`
6.  Setup SSL certificates for secure HTTPS access to the EdgeHUB application.
    1.  Equip with SSL key and certificate. If not available, refer to [Guide 1 "SSL Certificate Generation"](edgeHUB/README.md) to generate the SSL key and certificate.
    2.  Install the certificate by referring to [Guide 2 "Browser Certificate Installation"](edgeHUB/README.md).
    3.  Restart the browser for seamless access.
7.  Run `deactivate` to come out of environment after usage.

#### Usage

1.  Change directory to `HUB/src/hub/hub_apps/edgeHUB`.
2.  To suppress SSL warnings refer to [Guide 1 & 2 of EdgeHUB README](edgeHUB/README.md).
2.  Run `python app.py` to get application up and running.
3.  It will run the web application on 443 port. To change port refer to [Guide 3 of EdgeHUB README](edgeHUB/README.md).
4.  Open the link mentioned in the terminal (say, Running on ...") in a web browser to see the web app in action.
5.  Use on-page buttons to navigate through features. Find details at [Features section of EdgeHUB README](edgeHUB/README.md).
