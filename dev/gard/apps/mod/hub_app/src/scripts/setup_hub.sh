#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

# Uncomment for verbose
#set -x

set -euo pipefail

#-----------------------------------------------------------------------------
# This script should be called automatically from HUB's make framework.
# The call would ideally be: make setup
#
# When done so, this script is called with the following arguments:
# $1 directory containing HUB external submodules (src code)
# $2 directory in build/output where HUB externals will be compiled and 
#    installed

#-----------------------------------------------------------------------------
# Where are HUB's external submodules?
#-----------------------------------------------------------------------------
HUB_EXTERNALS_DIR=${1:-../external}

#-----------------------------------------------------------------------------
# We install HUB's external dependencies (.so libs) here
#-----------------------------------------------------------------------------
HUB_EXTERNAL_INSTALLS_DIR=${2:-$(realpath ${OUTPUT_DIR}/hub_ext)}

# Compile and install libusb
function install_libusb()
{
    LIBUSB_DIR=${HUB_EXTERNALS_DIR}/libusb

    echo -n "Compiling and installing libusb-1.0..."
    cd ${LIBUSB_DIR} && \
    ./autogen.sh --silent 2>&1 >/dev/null && \
    ./configure --silent --prefix=${HUB_EXTERNAL_INSTALLS_DIR} 2>&1 >/dev/null \
    && make -j $(nproc) 2>&1 >/dev/null && make install 2>&1 >/dev/null && \
    echo "done."
}

# Compile and install libcjson
function install_cjson()
{
    CJSON_DIR=${HUB_EXTERNALS_DIR}/cJSON
    CMAKE_BLD_DIR=$(mktemp -d)
    CMAKE_BLD_OPTS="-DCMAKE_INSTALL_PREFIX=${HUB_EXTERNAL_INSTALLS_DIR} -DENABLE_CJSON_TEST=Off"

    echo -n "Compiling and installing cjson..."
    cd ${CMAKE_BLD_DIR} && \
    cmake ${CMAKE_BLD_OPTS} ${CJSON_DIR} 2>&1 >/dev/null && \
    make -j $(nproc) 2>&1 >/dev/null && make install 2>&1 >/dev/null && \
    rm -rf ${CMAKE_BLD_DIR} && \
    echo "done."
}

# Compile and install libgpiod
function install_libgpiod()
{
    LIBGPIOD_DIR=${HUB_EXTERNALS_DIR}/libgpiod

    echo -n "Compiling and installing libgpiod..."
    cd ${LIBGPIOD_DIR} && \
    ./autogen.sh --silent 2>&1 >/dev/null && \
    ./configure --silent --prefix=${HUB_EXTERNAL_INSTALLS_DIR} 2>&1 >/dev/null \
    && make -j $(nproc) 2>&1 >/dev/null && make install >/dev/null && \
    echo "done."
}

# Add a udev rule for VVML USB device with given Vendor Id and Product ID
# to make it writable by regular users
#
# The Vendor and Product IDs are read from the host_config.json
#
# TBD-DPN: Clean this up and make it more elegant!
#
# $1 is the host_config.json file from the config/ directory
function install_udev_usb_rule()
{
    HOST_CONFIG_JSON=$1
    USB_BUS_TYPE="HUB_GARD_BUS_USB"
    TMP_FILE=$(mktemp)
    VVML_USB_UDEV_FILE="/etc/udev/rules.d/99-vvml-usb.rules"

    USB_VENDOR_ID=$(cat ${HOST_CONFIG_JSON} | jq '.busses[] | select(.bus_type=="HUB_GARD_BUS_USB")'.usb_vendor_id)
    USB_PRODUCT_ID=$(cat ${HOST_CONFIG_JSON} | jq '.busses[] | select(.bus_type=="HUB_GARD_BUS_USB")'.usb_product_id)

    # check if we do get Vendor Id and Product ID
    if [ "$USB_VENDOR_ID" == "null" ] || [ "$USB_PRODUCT_ID" == "null" ]
    then
        echo "No USB entries found in ${HOST_CONFIG_JSON}."
    else
        echo "SUBSYSTEM==\"usb\",ATTR{idVendor}==${USB_VENDOR_ID},ATTR{idProduct}==${USB_PRODUCT_ID},MODE=\"0666\"" > ${TMP_FILE}
        sudo mv ${TMP_FILE} ${VVML_USB_UDEV_FILE}

        sudo udevadm control --reload
        sudo udevadm trigger
    fi
}

# clean up a previous installation
rm -rf ${HUB_EXTERNAL_INSTALLS_DIR}

# all packages needed for HUB should be installed by cloning the repo
# https://github.com/LSCC-Architecture/HUB_packages
# and running the scripts therein.

pushd .
install_libusb
install_cjson
install_libgpiod
popd
install_udev_usb_rule ../../config/host_config.json

