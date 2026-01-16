#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# Copyright (c) 2025 Lattice Semiconductor Corporation
#
# SPDX-License-Identifier: UNLICENSED
#
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# TBD-DPN: Refactor this and make it modular
#
# Basic bash script for creating a HUB .deb and .whl packages
#-----------------------------------------------------------------------------

# Exit immediately if a command exits with a non-zero status.
set -euo pipefail

THIS_SCRIPTS_DIR=$(dirname "$(realpath "$0")")

#-----------------------------------------------------------------------------
# This script should be called automatically from HUB's make framework.
# The call would ideally be: make package
#
# When done so, this script is called with the following arguments:
# $1 HUB package name
# $2 HUB version header file
# $3 HUB CPU architecture
# $4 HUB artifacts dir
#-----------------------------------------------------------------------------

#
# Input arguments to this script
#
# The name of the package as per Debian dpkg / apt
HUB_PKG_NAME=${1:-ILLEGAL_PACKAGE}
# HUB version header file
HUB_VERSION_HDR=${2:-ILLEGAL_HEADER}
# HUB CPU architecture
HUB_CPU_ARCH=${3:-INVALID_CPU}
# HUB build arfticats dir
HUB_ARTIFACTS_DIR=${4:-ILLEGAL_PACKAGE}

# Debian control file template
HUB_DEBIAN_CONTROL_FILE_TEMPL=${THIS_SCRIPTS_DIR}/hub_pkg_control.in

# The final location of HUB when installed on the system
HUB_ON_SYSTEM=/opt/hub

HUB_LIB_DIR=${HUB_ON_SYSTEM}/lib
HUB_BIN_DIR=${HUB_ON_SYSTEM}/bin
HUB_DRIVERS_DIR=${HUB_ON_SYSTEM}/drivers
HUB_INC_DIR=${HUB_ON_SYSTEM}/include

# Get HUB version string from HUB version header file
#
# Remember that hub_version.h contains lines like
# #define HUB_VERSION_XXX <number>
# so we grep for such lines for MAJOR, MINOR and BUGFIX
#
# $1 HUB version header file
function get_hub_version() {
    local MAJOR=$(cat "$1" \
                | grep "#define HUB_VERSION_MAJOR" | awk '{print $3}')
    local MINOR=$(cat "$1" \
                | grep "#define HUB_VERSION_MINOR" | awk '{print $3}')
    local BUGFIX=$(cat "$1" \
                | grep "#define HUB_VERSION_BUGFIX" | awk '{print $3}')

    HUB_VERSION_STRING=${MAJOR}.${MINOR}.${BUGFIX}
}

# TBD-DPN: Put checks on each step?
# Create a .deb package for HUB
function create_deb_package() {

    rm -rf ${PKG_DIR} &&                            \
    cd ${HUB_ARTIFACTS_DIR} &&                      \
    mkdir -p ${PKG_DIR}/DEBIAN &&                   \
    mkdir -p ${PKG_DIR}/${HUB_LIB_DIR} &&           \
    mkdir -p ${PKG_DIR}/${HUB_BIN_DIR} &&           \
    mkdir -p ${PKG_DIR}/${HUB_DRIVERS_DIR} &&       \
    mkdir -p ${PKG_DIR}/${HUB_INC_DIR} &&           \
    mv *.so ${PKG_DIR}/${HUB_LIB_DIR} &&            \
    mv *.so.* ${PKG_DIR}/${HUB_LIB_DIR} &&          \
    mv *.h ${PKG_DIR}/${HUB_INC_DIR} &&                             \
    chrpath -r ${HUB_LIB_DIR} *.elf 2>&1 > /dev/null &&             \
    mv *.elf ${PKG_DIR}/${HUB_BIN_DIR} &&                           \
    mv *.sh  ${PKG_DIR}/${HUB_BIN_DIR} &&                           \
    mv *.ko ${PKG_DIR}/${HUB_DRIVERS_DIR} &&                        \
    mv *.dtbo ${PKG_DIR}/${HUB_DRIVERS_DIR} &&                      \
    mv config ${PKG_DIR}/${HUB_ON_SYSTEM} &&                        \
    mv examples ${PKG_DIR}/${HUB_ON_SYSTEM} &&                      \
    cp ${HUB_DEBIAN_CONTROL_FILE_TEMPL} ${HUB_PKG_CONTROL_FILE} &&                  \
    sed -i "s,HUB_PKG_NAME,$HUB_PKG_NAME,g" ${HUB_PKG_CONTROL_FILE} &&                  \
    sed -i "s,HUB_VERSION_STRING,$HUB_VERSION_STRING,g" ${HUB_PKG_CONTROL_FILE} &&      \
    sed -i "s,HUB_CPU_ARCH,$HUB_CPU_ARCH,g" ${HUB_PKG_CONTROL_FILE} &&                  \
    dpkg-deb --build --root-owner-group ${PKG_DIR} &&                                   \
    rm -rf ${PKG_DIR} &&                                                                \
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-" &&      \
    echo "" &&                                                                          \
    echo "Package ${HUB_DEB_NAME}.deb created in ${HUB_ARTIFACTS_DIR}" &&               \
    echo "" &&                                                                          \
    echo "-=-=-=-=-=-=-=--=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-" &&      \
    echo ""
}


function create_wheel_package() {
    # 'SECTION 0: CHECKS'
    # Check Python Build Module
    if ! python -c "import build" &> /dev/null; then
        echo "Python 'build' module not found. Install 'build' module to proceed."
        return 1
        # pip install build
    else
        echo "Python 'build' module found. Ready."
    fi

    # 'SECTION 1: PREPARATION PHASE'
    cd "$HUB_PY_PKG_DIR"
    mkdir -p "$WHL_DIR"
    cp -r "$HUB_PY_PKG_DIR"/* "$WHL_DIR"/
    cd "$WHL_DIR"

    if [ -n "${HUB_VERSION_STRING}" ]; then
        echo "Updating version in pyproject.toml to ${HUB_VERSION_STRING}"
        sed -i "s/^version = \".*\"/version = \"${HUB_VERSION_STRING}\"/" "pyproject.toml"
    fi

    if [ -n "${HUB_PKG_NAME}" ]; then
        echo "Updating package name in pyproject.toml to ${HUB_PKG_NAME}"
        sed -i "s/^name = \".*\"/name = \"${HUB_PKG_NAME}\"/" "pyproject.toml"
    fi

    if [ -n "${HUB_VERSION_STRING}" ]; then
        echo "Updating version in init to ${HUB_VERSION_STRING}"
        sed -i "s/^HUB_VERSION = \".*\"/HUB_VERSION = \"${HUB_VERSION_STRING}\"/" "hub/__init__.py"
    fi


    # 'SECTION 2: BUILD PHASE'
    # Build the Wheel Package
    echo "--- Building the wheel package ---"
    if [ ! -f "pyproject.toml" ]; then
        echo "Error: 'pyproject.toml' not found in $BUILD_PY_DIR."
        return 1
    fi
    # glibc is 2.28 is the widely used stable version for compatibility
    (python -m build --no-isolation --wheel --config-setting="--build-option=--plat-name=manylinux_2_28_aarch64")
    echo "Wheel package successfully built."


    # 'SECTION 3: DISPATCH PHASE'
    # Copy the .whl file to the final output directory
    WHEEL_FILE=$(ls "./dist/"*.whl)
    mv "$WHEEL_FILE" "$HUB_ARTIFACTS_DIR"
    echo "Final package copied to $HUB_ARTIFACTS_DIR"

    rm -rf $WHL_DIR

    echo -e "\n=========================================================================================\n"
    echo " Wheel package is created at build/output/package/${WHEEL_FILE##*/}"
    echo -e "\n=========================================================================================\n"

    return 0

}
# --- Main Script Execution ---

get_hub_version "${HUB_VERSION_HDR}"

# Name of the .deb file to create
HUB_DEB_NAME=${HUB_PKG_NAME}_${HUB_VERSION_STRING}_${HUB_CPU_ARCH}
# Temporary directory for .deb creation
PKG_DIR=${HUB_ARTIFACTS_DIR}/${HUB_DEB_NAME}
# Final control file
HUB_PKG_CONTROL_FILE=${PKG_DIR}/DEBIAN/control

HUB_PY_PKG_DIR=${OUTPUT_DIR}/hub_py
WHL_DIR=${HUB_ARTIFACTS_DIR}/temp_whl_build


# Call the functions to create both packages
create_deb_package
pushd .
create_wheel_package
if [ $? -eq 0 ]; then
  echo "Wheel package is created successfully!"
else
  echo "Failed to create wheel package!"
  exit 1
fi
popd
