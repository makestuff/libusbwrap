/*
 * Copyright (C) 2009-2012 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef WIN32
#include <Windows.h>
#else
#define _BSD_SOURCE
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <makestuff.h>
#include <liberror.h>
#include "private.h"

static struct libusb_context *m_ctx = NULL;

// Modified from libusb_open_device_with_vid_pid in core.c of libusbx
//
static libusb_device_handle *libusbOpenWithVidPid(
	libusb_context *ctx, uint16 vid, uint16 pid, uint16 did, const char **error)
{
	static libusb_device_handle *retVal = NULL;
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	size_t i = 0;
	int status = (int)libusb_get_device_list(ctx, &devs);
	CHECK_STATUS(status < 0, NULL, cleanup, libusb_error_name(status));
	dev = devs[i++];
	while ( dev ) {
		struct libusb_device_descriptor desc;
		status = libusb_get_device_descriptor(dev, &desc);
		CHECK_STATUS(status < 0, NULL, cleanup, libusb_error_name(status));
		if (
			desc.idVendor == vid &&
			desc.idProduct == pid &&
			(did == 0x0000 || desc.bcdDevice == did)
		) {
			found = dev;
			break;
		}
		dev = devs[i++];
	}

	if ( found ) {
		status = libusb_open(found, &retVal);
		CHECK_STATUS(status < 0, NULL, cleanup, libusb_error_name(status));
	} else {
		errRender(error, "device not found");
	}

cleanup:
	libusb_free_device_list(devs, 1);
	return retVal;
}

// Return true if vp is VVVV:PPPP where V and P are hex digits:
//
DLLEXPORT(bool) usbValidateVidPid(const char *vp) {
	int i;
	char ch;
	const size_t len = strlen(vp);
	bool hasDID;
	if ( !vp ) {
		return false;
	}
	if ( len == 9 ) {
		hasDID = false;
	} else if ( len == 14 ) {
		hasDID = true;
	} else {
		return false;
	}
	if ( vp[4] != ':' || (hasDID && vp[9] != ':') ) {
		return false;
	}
	for ( i = 0; i < 4; i++ ) {
		ch = vp[i];
		if (
			ch < '0' ||
			(ch > '9' && ch < 'A') ||
			(ch > 'F' && ch < 'a') ||
			ch > 'f')
		{
			return false;
		}
	}
	for ( i = 5; i < 9; i++ ) {
		ch = vp[i];
		if (
			ch < '0' ||
			(ch > '9' && ch < 'A') ||
			(ch > 'F' && ch < 'a') ||
			ch > 'f')
		{
			return false;
		}
	}
	if ( hasDID ) {
		for ( i = 10; i < 14; i++ ) {
			ch = vp[i];
			if (
				ch < '0' ||
				(ch > '9' && ch < 'A') ||
				(ch > 'F' && ch < 'a') ||
				ch > 'f')
			{
				return false;
			}
		}
	}
	return true;
}

// Initialise LibUSB with the given log level.
//
DLLEXPORT(USBStatus) usbInitialise(int debugLevel, const char **error) {
	USBStatus retVal = USB_SUCCESS;
	int status = libusb_init(&m_ctx);
	CHECK_STATUS(status, USB_INIT, cleanup, "usbInitialise(): %s", libusb_error_name(status));
	libusb_set_debug(m_ctx, debugLevel);
cleanup:
	return retVal;
}

#define isMatching (thisDevice->descriptor.idVendor == vid && thisDevice->descriptor.idProduct == pid)

#define FORMAT_ERR "The supplied VID:PID:DID \"%s\" is invalid; it should look like 1D50:602B or 1D50:602B:0001"

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(USBStatus) usbIsDeviceAvailable(const char *vp, bool *isAvailable, const char **error) {
	USBStatus retVal = USB_SUCCESS;
	struct libusb_device **devList = NULL;
	struct libusb_device *thisDev;
	struct libusb_device_descriptor desc;
	uint16 vid, pid, did;
	int status, count = (int)libusb_get_device_list(m_ctx, &devList);
	CHECK_STATUS(
		count < 0, USB_CANNOT_OPEN_DEVICE, cleanup,
		"usbIsDeviceAvailable(): %s", libusb_error_name(count));
	CHECK_STATUS(
		!usbValidateVidPid(vp), USB_INVALID_VIDPID, cleanup,
		"usbIsDeviceAvailable(): "FORMAT_ERR, vp);
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	did = (uint16)((strlen(vp) == 14) ? strtoul(vp+10, NULL, 16) : 0x0000);
	*isAvailable = false;
	while ( count-- ) {
		thisDev = devList[count];
		status = libusb_get_device_descriptor(thisDev, &desc);
		CHECK_STATUS(
			status, USB_CANNOT_GET_DESCRIPTOR, cleanup,
			"usbIsDeviceAvailable(): %s", libusb_error_name(status));
		if (
			desc.idVendor == vid &&
			desc.idProduct == pid &&
			(did == 0x0000 || desc.bcdDevice == did)
		) {
			*isAvailable = true;
			break;
		}
	}
cleanup:
	libusb_free_device_list(devList, 1);
	return retVal;
}

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(USBStatus) usbOpenDevice(
	const char *vp, int configuration, int iface, int altSetting,
	struct USBDevice **devHandlePtr, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	struct libusb_device_handle *deviceHandle;
	uint16 vid, pid, did;
	int status;
	CHECK_STATUS(
		!usbValidateVidPid(vp), USB_INVALID_VIDPID, cleanup,
		"usbOpenDevice(): "FORMAT_ERR, vp);
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	did = (uint16)((strlen(vp) == 14) ? strtoul(vp+10, NULL, 16) : 0x0000);
	*devHandlePtr = NULL;
	deviceHandle = libusbOpenWithVidPid(m_ctx, vid, pid, did, error);
	CHECK_STATUS(!deviceHandle, USB_CANNOT_OPEN_DEVICE, cleanup, "usbOpenDevice()");
	*devHandlePtr = wrap(deviceHandle);  // Return the valid device handle anyway, even if subsequent operations fail
	status = libusb_set_configuration(deviceHandle, configuration);
	CHECK_STATUS(
		status < 0, USB_CANNOT_SET_CONFIGURATION, cleanup,
		"usbOpenDevice(): %s", libusb_error_name(status));
	status = libusb_claim_interface(deviceHandle, iface);
	CHECK_STATUS(
		status < 0, USB_CANNOT_CLAIM_INTERFACE, cleanup,
		"usbOpenDevice(): %s", libusb_error_name(status));
	status = libusb_set_interface_alt_setting(deviceHandle, iface, altSetting);
	CHECK_STATUS(
		status < 0, USB_CANNOT_SET_ALTINT, cleanup,
		"usbOpenDevice(): %s", libusb_error_name(status));
cleanup:
	return retVal;
}

DLLEXPORT(void) usbCloseDevice(struct USBDevice *dev, int iface) {
	libusb_release_interface(unwrap(dev), iface);
	libusb_close(unwrap(dev));
}

DLLEXPORT(USBStatus) usbControlRead(
	struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	int status = libusb_control_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	CHECK_STATUS(
		status < 0, USB_CONTROL, cleanup,
		"usbControlRead(): %s", libusb_error_name(status));
	CHECK_STATUS(
		status != wLength, USB_CONTROL, cleanup,
		"usbControlRead(): expected to read %d bytes but actually read %d", wLength, status);
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbControlWrite(
	struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	const uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	int status = libusb_control_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	CHECK_STATUS(
		status < 0, USB_CONTROL, cleanup,
		"usbControlWrite(): %s", libusb_error_name(status));
	CHECK_STATUS(
		status != wLength, USB_CONTROL, cleanup,
		"usbControlWrite(): expected to write %d bytes but actually wrote %d", wLength, status);
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkRead(
	struct USBDevice *dev, uint8 endpoint, uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	int numRead;
	int status = libusb_bulk_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_IN | endpoint,
		(uint8 *)data,
		(int)count,
		&numRead,
		timeout
	);
	CHECK_STATUS(
		status < 0, USB_BULK, cleanup,
		"usbBulkRead(): %s", libusb_error_name(status));
	CHECK_STATUS(
		(uint32)numRead != count, USB_BULK, cleanup,
		"usbBulkRead(): expected to read %d bytes but actually read %d (status = %d): %s",
		count, numRead, status, libusb_error_name(status));
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkWrite(
	struct USBDevice *dev, uint8 endpoint, const uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	int numWritten;
	int status = libusb_bulk_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_OUT | endpoint,
		(uint8 *)data,
		(int)count,
		&numWritten,
		timeout
	);
	CHECK_STATUS(
		status < 0, USB_BULK, cleanup,
		"usbBulkWrite(): %s", libusb_error_name(status));
	CHECK_STATUS(
		(uint32)numWritten != count, USB_BULK, cleanup,
		"usbBulkWrite(): expected to write %d bytes but actually wrote %d (status = %d): %s",
		count, numWritten, status, libusb_error_name(status));
cleanup:
	return retVal;
}

DLLEXPORT(struct libusb_context *) usbGetContext(void) {
	return m_ctx;
}
