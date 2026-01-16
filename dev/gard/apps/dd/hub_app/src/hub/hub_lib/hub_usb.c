/******************************************************************************
 * Copyright (c) 2025 Lattice Semiconductor Corporation
 *
 * SPDX-License-Identifier: UNLICENSED
 *
 ******************************************************************************/

#include "hub_usb.h"

/**
 * TBD-DPN: The current release does not have an OS abstraction layer (OSAL).
 * In subsequent releases, we will have a HUB OSAL to take care of all
 * operations performed by the underlying OS on behalf of / for HUB.
 */

/**
 * TBD-DPN: We support only 1 USB device for now
 * This variable maps the int bus_hdl to its libusb counterpart
 *
 * We initialize this structure to invalid / NULL entries
 * to indicate default condition on boot / bus close-disconnect.
 */
static struct usb_bus_hdl_map hdl_map = {
	.bus_hdl    = HUB_USB_INVALID_BUS_HDL,
	.libusb_hdl = NULL,
};

/**
 * TBD-DPN: We support only 1 USB device for now
 * This variable indicates whether another kernel driver
 * attached to our device of interest had to be detached, so
 * that we remember to re-attach it when we are done.
 */
static int kernelDriverDetached = 0;

/* Listing of static functions defined in this file */

/**
 * A bulk transfer (read / write) on USB  involves 2 steps:
 * 1. A register write to the AXI BASE REG indicating the operation
 * This is done by hub_usb_device_reg_write()
 * 2. A burst operation (read / write) doing the actual transfer
 * This is done by hub_usb_device_burst_write() for write
 * and hub_usb_device_burst_read() for read
 *
 * The code in these functions just follows lscVipUsb code, and
 * has some unknown magic numbers being written to registers.
 */
static int32_t
	hub_usb_device_reg_write(libusb_device_handle *hdl, int addr, int data);
static int32_t hub_usb_device_burst_write(libusb_device_handle *hdl,
										  unsigned char        *data,
										  int                   length);
static int32_t hub_usb_device_burst_read(libusb_device_handle *hdl,
										 unsigned char        *data,
										 int                  *length);

/**
 * TBD-DPN: Revisit the return value for this call.
 *
 * Open an USB bus given an opaque pointer representing the bus's properties.
 * Returns a bus handle on success.
 *
 * Any further operation on the said USB bus has to use this bus handle.
 *
 * @param: param an opaque pointer containing the bus's properties.
 *
 * @return: 1 is the bus handle for success in accordance with Python
 * 		 -1 for failure
 */
int32_t hub_usb_device_open(void *param)
{
	int32_t                        ret;

	struct hub_gard_bus_usb_props *p_usb_ctx =
		(struct hub_gard_bus_usb_props *)param;

	if (p_usb_ctx->is_open) {
		hub_pr_dbg("Bus already open with bus_hdl:%d\n", p_usb_ctx->bus_hdl);
		return p_usb_ctx->bus_hdl;
	}

	ret = libusb_init(0);
	if (0 != ret) {
		hub_pr_err("Error in libusb_init\n");
		goto err_usb_open_1;
	}

	/* Open device using VID and PID */
	hdl_map.libusb_hdl = libusb_open_device_with_vid_pid(
		0, p_usb_ctx->vendor_id, p_usb_ctx->product_id);
	if (NULL == hdl_map.libusb_hdl) {
		hub_pr_err("Error in libusb_open_device\n");
		goto err_usb_open_2;
	}

	/* Detach device from kernel driver */
	if (libusb_kernel_driver_active(hdl_map.libusb_hdl, 0)) {
		ret = libusb_detach_kernel_driver(hdl_map.libusb_hdl, 0);

		if (0 == ret) {
			kernelDriverDetached = 1;
		} else {
			hub_pr_err("Error detaching kernel driver\n");
			goto err_usb_open_3;
		}
	}

	/* Claim interface */
	ret = libusb_claim_interface(hdl_map.libusb_hdl, 0);
	if (0 != ret) {
		hub_pr_err("Error claiming interface\n");
		goto err_usb_open_4;
	}

	hub_pr_dbg("Opened USB device with VID:0x%x, PID:0x%x\n",
			   p_usb_ctx->vendor_id, p_usb_ctx->product_id);

	/* We return 1 to conform with the existing Python code */
	hdl_map.bus_hdl    = 1;
	p_usb_ctx->bus_hdl = hdl_map.bus_hdl;
	p_usb_ctx->is_open = true;
	return hdl_map.bus_hdl;

err_usb_open_4:
	libusb_attach_kernel_driver(hdl_map.libusb_hdl, 0);
err_usb_open_3:
	libusb_close(hdl_map.libusb_hdl);
err_usb_open_2:
	libusb_exit(0);
err_usb_open_1:
	hdl_map.bus_hdl    = -1;
	p_usb_ctx->bus_hdl = hdl_map.bus_hdl;
	p_usb_ctx->is_open = false;
	return hdl_map.bus_hdl;
}

/**
 * TBD-DPN: Revisit the return code for this call.
 *
 * Close the USB bus opened by the open call.
 *
 * @param: usb_bus_hdl Handle to the open USB bus.
 *
 * @return: 1 for success in accordance with Python
 * 		 -1 for failure
 */
int32_t hub_usb_device_close(int usb_bus_hdl)
{
	int32_t               ret;
	libusb_device_handle *hdl = NULL;

	if (HUB_USB_INVALID_BUS_HDL == usb_bus_hdl) {
		hub_pr_dbg("Bus already closed / not open!\n");
		return 1;
	}

	if (usb_bus_hdl != hdl_map.bus_hdl) {
		hub_pr_err("Invalid bus_hdl");
		return -1;
	}

	hdl = hdl_map.libusb_hdl;
	if (NULL == hdl) {
		hub_pr_err("Invalid libusb_hdl\n");
		return -1;
	}

	/* Release interface */
	ret = libusb_release_interface(hdl, 0);

	if (0 != ret) {
		hub_pr_err("Error releasing interface\n");
		return -1;
	}

	/* Attach interface to kernel driver back */
	if (kernelDriverDetached) {
		libusb_attach_kernel_driver(hdl, 0);
	}

	libusb_close(hdl);

	/* Shutdown libusb */
	libusb_exit(0);

	/* Reset our hdl_map variables */
	hdl_map.bus_hdl    = HUB_USB_INVALID_BUS_HDL;
	hdl_map.libusb_hdl = NULL;

	hub_pr_dbg("Closed bus_hdl %d\n", usb_bus_hdl);

	/* We return 1 here since that's expected by Python */
	return 1;
}

/* TBD-DPN: Revisit optimizations and changes in subsequent releases */
/* Not commenting - This function is directly copied from lscVipUSB */
static int32_t
	hub_usb_device_reg_write(libusb_device_handle *hdl, int addr, int data)
{
	int32_t ret;

	uint8_t buf[4];

	buf[0] = (unsigned char)(data & 0xff);
	buf[1] = (unsigned char)((data >> 8) & 0xff);
	buf[2] = (unsigned char)((data >> 16) & 0xff);
	buf[3] = (unsigned char)((data >> 24) & 0xff);

	ret    = libusb_control_transfer(
        hdl,
        LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR |
            LIBUSB_RECIPIENT_DEVICE,
        LSCVIP_VX_WREG_WRITE, 0x00, (uint16_t)addr, buf, 4, ENDPOINT_TIMEOUT);

	/* Register write needs 1 usec sleep in case of burst write */
	usleep(1);

	return ret;
}

/* TBD-DPN: Revisit optimizations and changes in subsequent releases */
/* Not commenting - This function is directly copied from lscVipUSB */
static int32_t hub_usb_device_burst_write(libusb_device_handle *hdl,
										  unsigned char        *data,
										  int                   length)
{
	int32_t ret;
	int     nwrite;
	/* TBD-DPN: Can this be auto-determined ? */
	unsigned char ep = 0x04;

	ret =
		libusb_bulk_transfer(hdl, ep, data, length, &nwrite, ENDPOINT_TIMEOUT);

	return ret;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 *
 * Perform a write operation on the USB bus represented by the given bus
 * handle.
 *
 * @param: usb_bus_hdl Handle to the USB bus
 * @param: p_buffer Pointer to a buffer with data for writing
 * @param: count Number of bytes to write - SHOULD BE 4K ONLY
 *
 * TBD-DPN: Follows Python code assumptions; needs to change.
 * @return: 1 for success, -1 for error
 */
int32_t
	hub_usb_device_write(int usb_bus_hdl, const void *p_buffer, uint32_t count)
{
	int32_t                 ret, i;
	int                     wconfig   = 255;
	libusb_device_handle   *hdl       = NULL;
	struct hub_usb_ops_map *p_usb_ops = NULL;

	p_usb_ops                         = (struct hub_usb_ops_map *)p_buffer;

	/* This variable is typecast to int* to adhere to libusb call */
	int     *data                     = (int *)(p_usb_ops->p_buffer);
	uint32_t addr                     = p_usb_ops->addr;

	if (usb_bus_hdl != hdl_map.bus_hdl) {
		hub_pr_err("Invalid bus_hdl\n");
		return -1;
	}

	hdl = hdl_map.libusb_hdl;
	if (NULL == hdl) {
		hub_pr_err("Invalid libusb_hdl\n");
		return -1;
	}

	ret = hub_usb_device_reg_write(hdl, AXIWBASE + 1, wconfig);
	if (4 != ret) {
		return -1;
	}

	for (i = 0; i < 2; i++) {
		/* TBD-DPN: clean up the ret value checks*/
		ret = hub_usb_device_reg_write(hdl, AXIWBASE, addr + 2048 * i);
		if (4 != ret) {
			return -1;
		}
		ret = hub_usb_device_burst_write(hdl, (unsigned char *)(data), 2048);
		if (0 != ret) {
			return -1;
		}

		data += 512;
	}

	(void)ret;

	/* TBD-DPN: we return 1 to keep Python code happy */
	return 1;
}

/* TBD-DPN: Revisit optimizations and changes in subsequent releases */
/* Not commenting - This function is directly copied from lscVipUSB */
static int32_t hub_usb_device_burst_read(libusb_device_handle *hdl,
										 unsigned char        *data,
										 int                  *length)
{
	int32_t ret;
	int     nread;
	/* TBD-DPN: Can this be auto-determined ? */
	unsigned char ep = 0x81;

	nread            = *length;
	*length          = 0;

	ret = libusb_bulk_transfer(hdl, ep, data, nread, length, ENDPOINT_TIMEOUT);

	return ret;
}

/**
 * TBD-DPN: Revisit the error behaviour of this call
 *
 * Perform a read operation on the USB bus represented by the given bus handle.
 *
 * @param: usb_bus_hdl Handle to the USB bus
 * @param: p_buffer Pointer to a buffer to be filled by the read
 * @param: count Number of bytes to read - SHOULD BE 4K ONLY
 *
 * TBD-DPN: Follows Python code assumptions; needs to change.
 * @return: 1 for success, -1 for error
 */
int32_t hub_usb_device_read(int usb_bus_hdl, void *p_buffer, uint32_t count)
{
	int32_t                 ret, i, nread;
	int                     rconfig   = 255;
	libusb_device_handle   *hdl       = NULL;
	struct hub_usb_ops_map *p_usb_ops = NULL;

	p_usb_ops                         = (struct hub_usb_ops_map *)p_buffer;

	/* This variable is typecast to int* to adhere to libusb call */
	int     *data                     = (int *)(p_usb_ops->p_buffer);
	uint32_t addr                     = p_usb_ops->addr;

	if (usb_bus_hdl != hdl_map.bus_hdl) {
		hub_pr_err("Invalid bus_hdl\n");
		return -1;
	}

	hdl = hdl_map.libusb_hdl;
	if (NULL == hdl) {
		hub_pr_err("Invalid libusb_hdl\n");
		return -1;
	}

	ret = hub_usb_device_reg_write(hdl, AXIRBASE + 1, rconfig);
	if (4 != ret) {
		return -1;
	}

	for (i = 0; i < 2; i++) {
		/* TBD-DPN: clean up the ret value checks*/
		ret = hub_usb_device_reg_write(hdl, AXIRBASE, addr + 2048 * i);
		if (4 != ret) {
			return -1;
		}

		nread = 2048;
		ret   = hub_usb_device_burst_read(hdl, (unsigned char *)(data), &nread);
		if (2048 != nread) {
			return -1;
		}

		data += 512;
		ret   = (nread == 2048);
	}

	(void)ret;

	/* TBD-DPN: does Python code assume 1 as success? */
	return ret;
}