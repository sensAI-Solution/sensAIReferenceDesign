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
# This package provides:
#    - HUB: Main interface for HUB operations and GARD management
#    - GARD: Interface for GARD device operations (register access, data transfer, sensors)
# 
# The HUB.py is designed to provide python bindings to the
# HUB C library functions, so that python applications can use HUB natively.
#
# -----------------------------------------------------------------------------

HUB_VERSION = ""
__version__ = HUB_VERSION
__author__ = "Lattice Semiconductor Corporation"
__copyright__ = "Copyright (c) 2025 Lattice Semiconductor Corporation"
__license__ = "UNLICENSED"

from .hub import HUB, GARD
from .camInterface import CamInterface

# Define public API - only HUB and GARD are exported
__all__ = [
    "HUB",
    "GARD",
    "__version__",
]