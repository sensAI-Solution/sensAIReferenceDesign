/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef FLASHMAP_H
#define FLASHMAP_H

/**
 * This file defines how the contents of the Flash memory are organized.
 * The Flash memory is divided into different sections, each serving a specific
 * purpose. The sections are defined as follows:
 * - Bitstream: Contains the bitstream data for the device. Two copies of this
 * are stored though the second copy is not currently used.
 * - Configuration: Contains configuration data related to the contents of the
 * Flash.
 * - Directory: Contains the directory structure for the modules stored in
 * Flash.
 * - Modules: Different modules that are a part of the GARD infrastructure.
 *
 * Two copies of Configuration, Directory and Modules are present for
 * redundancy when enough space is available for accomodating both.
 *
 * The Flash memory is organized as follows:
 * +---------------------+ Bottom of memory  @ 0x0
 * |                     |
 * |      Bitstream      |  Bitstream data for the device.
 * |                     |
 * +---------------------+  End of Bitstream ~ @2.5MB
 * |     Unused          |
 * +---------------------+  Start of RFS Configuration @ 3MB
 * |                     |
 * |  RFS Configuration  |  RFS Configuration data.
 * |                     |
 * +---------------------+  End of RFS Configuration @(3MB + 32KB)
 * +---------------------+  Start of RFS Directory @(3MB + 32KB)
 * |                     |
 * |   RFS Directory     |  RFS Directory structure for locating the modules
 * |                     |  stored in Flash.
 * |                     |
 * +---------------------+  End of RFS Directory @(3MB + 64KB)
 * +---------------------+  Start of RFS Data space
 * +---------------------+  Start of first module
 * |                     |
 * |      Module-1       |
 * |                     |
 * +---------------------+  End of Module-1 + align to 32KB
 * +---------------------+  Start of second module
 * |                     |
 * |      Module-2       |
 * |                     |
 * +---------------------+  End of Module-2 + align to 32KB
 * +---------------------+  Start of third module
 * |                     |
 * | Rest of the modules |
 * |                     |
 * +---------------------+  End of last module
 * +---------------------+  End of RFS Data space
 * +---------------------+
 * |                     |
 * |      Unused         |
 * |                     |
 * +---------------------+  Mid-point of Flash
 * |                     |
 * |      Bitstream      |  Second copy of Bitstream data for the device.
 * |                     |
 * +---------------------+  End of Bitstream @(Midpoint + ~ 2.5MB)
 * |     Unused          |
 * +---------------------+  Start of RFS Configuration @(Midpoint + 3MB)
 * |                     |
 * |  RFS Configuration  |  RFS Configuration data.
 * |                     |
 * +---------------------+  End of RFS Configuration @(Midpoint + 3MB + 32KB)
 * +---------------------+  Start of RFS Directory @(Midpoint + 3MB + 32KB)
 * |                     |
 * |   RFS Directory     |  RFS Directory structure for locating the modules
 * |                     |  stored in Flash.
 * |                     |
 * +---------------------+  End of RFS Directory @(Midpoint + 3MB + 64KB)
 * +---------------------+  Start of RFS Data space
 * +---------------------+  Start of first module
 * |                     |
 * |      Module-1       |
 * |                     |
 * +---------------------+  End of Module-1 + align to 32KB
 * +---------------------+  Start of second module
 * |                     |
 * |      Module-2       |
 * |                     |
 * +---------------------+  End of Module-2 + align to 32KB
 * +---------------------+  Start of third module
 * |                     |
 * | Rest of the modules |
 * |                     |
 * +---------------------+  End of last module
 * +---------------------+  End of RFS Data space
 * +---------------------+
 * |                     |
 * |      Unused         |
 * |                     |
 * +---------------------+  End of Flash
 */

#endif /* FLASHMAP_H */
