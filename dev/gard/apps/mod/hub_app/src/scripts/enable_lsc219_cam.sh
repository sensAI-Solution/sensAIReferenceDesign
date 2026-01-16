#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Bash script for enabling the LSC219 camera on CPNX SOM
#-----------------------------------------------------------------------------

# Exit immediately if a command exits with a non-zero status.
set -euo pipefail

#set -x # Enable debugging

THIS_SCRIPTS_DIR=$(dirname "$(realpath "$0")")

#-----------------------------------------------------------------------------
# This script should be called automatically from HUB's make framework.
# The call would ideally be: make package
#
# When done so, this script is called with the following arguments:
# $1 TGT_OUTPUT_DIR from HUB Make
#-----------------------------------------------------------------------------

#
# Input arguments to this script
#
TGT_OUTPUT_DIR=${1:-/opt/hub/drivers}

LSC_OVERLAY_2LANE="dtoverlay=lsc219,cam0"
LSC_OVERLAY_4LANE="dtoverlay=lsc219,cam0,4lane"

CAM_HEIGHT_2LANE="1080"
CAM_WIDTH_2LANE="1920"
CAM_HEIGHT_4LANE="2464"
CAM_WIDTH_4LANE="3280"

LANE_CONFIG=2

NEEDS_REBOOT=false

# Disable inbox imx219 driver from loading
function disable_inbox_imx219_driver() {
    local kernel_version=$(uname -r)
    local imx219_module="/lib/modules/${kernel_version}/kernel/drivers/media/i2c/imx219.ko.xz"

    echo -n "Disabling inbox imx219 driver..."
    if [ -f "${imx219_module}" ]; then
        
        sudo mv ${imx219_module} ${imx219_module}.disabled
        sudo depmod -a
        if [ $? -ne 0 ]; then
            echo "Failed to disable inbox imx219 driver"
            exit 1
        fi
        echo "driver disabled."
    else
        echo "driver already disabled."
    fi
}

# Update /boot/firmware/config.txt to add the dtoverlay for lsc219 camera
function update_config_file () {
    local config_file="/boot/firmware/config.txt"

    echo -e "\nLane config for LSC219 camera:"
    echo "2 for 2-lane"
    echo "4 for 4-lane"
    echo "Any other key to exit without changes"
    echo -n "Enter lane config (2 or 4): "
    read -s -n 1 lane_config

    if [ "$lane_config" == "2" ]; then
        LSC_OVERLAY_THIS=${LSC_OVERLAY_2LANE}
        LSC_OVERLAY_THAT=${LSC_OVERLAY_4LANE}
        CAM_HEIGHT=${CAM_HEIGHT_2LANE}
        CAM_WIDTH=${CAM_WIDTH_2LANE}

    elif [ "$lane_config" == "4" ]; then
        LSC_OVERLAY_THIS=${LSC_OVERLAY_4LANE}
        LSC_OVERLAY_THAT=${LSC_OVERLAY_2LANE}
        CAM_HEIGHT=${CAM_HEIGHT_4LANE}
        CAM_WIDTH=${CAM_WIDTH_4LANE}
    else
        echo -e "\nInvalid input. Exiting without changes."
        exit 0
    fi

    # Check if the overlay line already exists in the config file
    if grep -q "^${LSC_OVERLAY_THIS}$" "$config_file"; then
        echo "Overlay already exists in $config_file"
    else
        echo "Adding overlay to $config_file"
        echo "$LSC_OVERLAY_THIS" | sudo tee -a "$config_file" > /dev/null
        NEEDS_REBOOT=true
    fi

    # Remove the other overlay line if it exists
    if grep -q "^${LSC_OVERLAY_THAT}$" "$config_file"; then
        echo "Removing conflicting overlay from $config_file"
        sudo sed -i.bak "/^${LSC_OVERLAY_THAT}$/d" "$config_file"
        NEEDS_REBOOT=true
    fi

    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
    echo "Configured LSC219 for ${lane_config}-lane operation."
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
}

# Load the lsc219 kernel module
# $1: Kernel module file path
function load_lsc219_kernel_module() {
    local module=$(basename $1)

    echo -n "Loading lsc219 kernel module..."

    # Check if the module is already loaded
    if lsmod | grep -q ${module%.*}; then
        echo "module is already loaded."
        return
    fi

    sudo insmod $1
    if [ $? -ne 0 ]; then
        echo "failed to load lsc219 kernel module."
        exit 1
    fi
    echo "module loaded successfully."
}

# Copy the DTBO file to /boot/overlays/
# $1: DTBO file
function copy_dtbo() {
    sudo cp -rv "$1" /boot/overlays/
}

# Main script execution starts here
disable_inbox_imx219_driver
copy_dtbo "${TGT_OUTPUT_DIR}/lsc219.dtbo"
update_config_file
if [ "$NEEDS_REBOOT" = true ]; then
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
    echo "Please POWER-CYCLE the SOM Assembly for changes to take effect."
    echo "Run this script AGAIN after the power-cycle to enable the LSC219 camera."
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
else
    load_lsc219_kernel_module ${TGT_OUTPUT_DIR}/imx219.ko
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
    echo "Please run a camera application (preferably on VNC) to test..."
    echo "Example: rpicam-vid -t 0 --height $CAM_HEIGHT --width $CAM_WIDTH"
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-"
fi
