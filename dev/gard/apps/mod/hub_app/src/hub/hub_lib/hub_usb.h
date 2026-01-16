/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#ifndef __HUB_USB_H__
#define __HUB_USB_H__

#include <stdio.h>
#include <unistd.h>
#include <libusb.h>

#include "gard_info.h"

/**
 * Define the vendor commands supported by our device.
 *
 * This assumes that we are connected to an FX-3 chip
 * flashed with VVML firmware and interacting with HUB
 * on a VVML board.
 *
 *  These commands are probably conveyed to the FX-3 firmware
 * flashed for VVML USB debug on the VVML board.
 */
#define LSCVIP_VX_SetIntMode    0xB1
#define LSCVIP_VX_REG_READ      0xB3
#define LSCVIP_VX_REG_WRITE     0xB4
#define LSCVIP_VX_WREG_READ     0xB5
#define LSCVIP_VX_WREG_WRITE    0xB6
#define LSCVIP_VX_Get_Status    0xB7

/**
 * These addresses are copied from the lscVipUsb code
 * and pertain to a VVML board.
 *
 * AXI read/write base addresses
 */
#define AXIWBASE                0x100
#define AXIRBASE                0x110

/**
 * TBD-DPN: Code assumes a single USB bus.
 *
 * Uninitialized value of a USB bus handle
 * This handle is used by the public calls
 * for the USB bus.
 *
 * Internally, the USB bus uses a libusb_device_handle.
 */
#define HUB_USB_INVALID_BUS_HDL -1

/**
 * Operation timeout for a USB operation.
 *
 * Inherited from lscVipUsb code.
 */
#define ENDPOINT_TIMEOUT        1000  // millisecs

/**
 * Structure holding the mapping of USB bus handle
 * (for USB operations) and the internal bus handle
 * used by libusb stack.
 */
struct usb_bus_hdl_map {
	int                   bus_hdl;
	libusb_device_handle *libusb_hdl;
};

/**
 * TBD-DPN: Revisit the return value for this call.
 *
 * Open an USB bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 *
 * Any further operation on the said USB bus has to use this bus handle.
 */
int32_t hub_usb_device_open(void *param);

/**
 * TBD-DPN: Revisit the return code for this call.
 *
 * Close the USB bus opened by the open call.
 */
int32_t hub_usb_device_close(int usb_bus_hdl);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 *
 * Perform a write operation on the USB bus represented by the given bus
 * handle.
 */
int32_t
	hub_usb_device_write(int usb_bus_hdl, const void *p_buffer, uint32_t count);

/**
 * TBD-DPN: Revisit the error behaviour of this call
 *
 * Perform a read operation on the USB bus represented by the given bus handle.
 */
int32_t hub_usb_device_read(int usb_bus_hdl, void *p_buffer, uint32_t count);

#endif /* __HUB_USB_H__ */