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
	struct libusb_device **devs;
	struct libusb_device *found = NULL;
	struct libusb_device *dev;
	struct libusb_device_handle *handle = NULL;
	size_t i = 0;
	int uStatus = (int)libusb_get_device_list(ctx, &devs);
	if ( uStatus < 0 ) {
		errRender(error, "%s", libusb_error_name(uStatus));
		return NULL;
	}
	dev = devs[i++];
	while ( dev ) {
		struct libusb_device_descriptor desc;
		uStatus = libusb_get_device_descriptor(dev, &desc);
		if ( uStatus < 0 ) {
			errRender(error, "%s", libusb_error_name(uStatus));
			goto cleanup;
		}
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
		uStatus = libusb_open(found, &handle);
		if ( uStatus < 0 ) {
			errRender(error, "%s", libusb_error_name(uStatus));
			handle = NULL;
		}
	} else {
		errRender(error, "device not found");
	}

cleanup:
	libusb_free_device_list(devs, 1);
	return handle;
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
DLLEXPORT(int) usbInitialise(int debugLevel, const char **error) {
	int returnCode = libusb_init(&m_ctx);
	if ( returnCode ) {
		errRender(error, "usbInitialise(): %s", libusb_error_name(returnCode));
		FAIL(USB_INIT);
	}
	libusb_set_debug(m_ctx, debugLevel);
cleanup:
	return returnCode;
}

#define isMatching (thisDevice->descriptor.idVendor == vid && thisDevice->descriptor.idProduct == pid)

#define FORMAT_ERR "The supplied VID:PID:DID \"%s\" is invalid; it should look like 1D50:602B or 1D50:602B:0001"

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(int) usbIsDeviceAvailable(const char *vp, bool *isAvailable, const char **error) {
	int returnCode = USB_SUCCESS, uStatus;
	struct libusb_device **devList = NULL;
	struct libusb_device *thisDev;
	struct libusb_device_descriptor desc;
	uint16 vid, pid, did;
	int count = (int)libusb_get_device_list(m_ctx, &devList);
	if ( count < 0 ) {
		errRender(error, "usbIsDeviceAvailable(): %s", libusb_error_name(count));
		FAIL(USB_CANNOT_OPEN_DEVICE);
	}
	if ( !usbValidateVidPid(vp) ) {
		errRender(error, FORMAT_ERR, vp);
		FAIL(USB_INVALID_VIDPID);
	}
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	did = (strlen(vp) == 14) ? (uint16)strtoul(vp+10, NULL, 16) : 0x0000;
	*isAvailable = false;
	while ( count-- ) {
		thisDev = devList[count];
		uStatus = libusb_get_device_descriptor(thisDev, &desc);
		if ( uStatus < 0 ) {
			errRender(error, "usbIsDeviceAvailable(): %s", libusb_error_name(uStatus));
			FAIL(USB_CANNOT_GET_DESCRIPTOR);
		}
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
	return returnCode;
}

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(int) usbOpenDevice(
	const char *vp, int configuration, int iface, int altSetting,
	struct USBDevice **devHandlePtr, const char **error)
{
	USBStatus returnCode = USB_SUCCESS;
	struct libusb_device_handle *deviceHandle;
	int uStatus;
	uint16 vid, pid, did;
	if ( !usbValidateVidPid(vp) ) {
		errRender(error, FORMAT_ERR, vp);
		return USB_INVALID_VIDPID;
	}
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	did = (strlen(vp) == 14) ? (uint16)strtoul(vp+10, NULL, 16) : 0x0000;
	*devHandlePtr = NULL;
	deviceHandle = libusbOpenWithVidPid(m_ctx, vid, pid, did, error);
	if ( !deviceHandle ) {
		errPrefix(error, "usbOpenDevice()");
		FAIL(USB_CANNOT_OPEN_DEVICE);
	}
	//CHECK_STATUS((!deviceHandle), "usbOpenDevice()", USB_CANNOT_OPEN_DEVICE);
	*devHandlePtr = wrap(deviceHandle);  // Return the valid device handle anyway, even if subsequent operations fail
	uStatus = libusb_set_configuration(deviceHandle, configuration);
	if ( uStatus < 0 ) {
		errRender(error, "usbOpenDevice(): %s", libusb_error_name(uStatus));
		FAIL(USB_CANNOT_SET_CONFIGURATION);
	}
	uStatus = libusb_claim_interface(deviceHandle, iface);
	if ( uStatus < 0 ) {
		errRender(error, "usbOpenDevice(): %s", libusb_error_name(uStatus));
		FAIL(USB_CANNOT_CLAIM_INTERFACE);
	}
	//if ( altSetting ) {
		uStatus = libusb_set_interface_alt_setting(deviceHandle, iface, altSetting);
		if ( uStatus < 0 ) {
			errRender(error, "usbOpenDevice(): %s", libusb_error_name(uStatus));
			FAIL(USB_CANNOT_SET_ALTINT);
		}
	//}
	/*uStatus = libusb_control_transfer(
		deviceHandle,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_INTERFACE,
		0x0b,
		(uint16)altSetting,
		(uint16)iface,
		NULL,
		0x0000,
		5000
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbOpenDevice(): %s", libusb_error_name(uStatus));
		FAIL(uStatus);
	}*/
cleanup:
	return returnCode;
}

DLLEXPORT(void) usbCloseDevice(struct USBDevice *dev, int iface) {
	libusb_release_interface(unwrap(dev), iface);
	libusb_close(unwrap(dev));
}

DLLEXPORT(int) usbControlRead(
	struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = libusb_control_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbControlRead(): %s", libusb_error_name(uStatus));
		FAIL(uStatus);
	} else if ( uStatus != wLength ) {
		errRender(
			error,
			"usbControlRead(): expected to read %d bytes but actually read %d",
			wLength, uStatus
		);
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}

DLLEXPORT(int) usbControlWrite(
	struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	const uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = libusb_control_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbControlWrite(): %s", libusb_error_name(uStatus));
		FAIL(uStatus);
	} else if ( uStatus != wLength ) {
		errRender(
			error,
			"usbControlWrite(): expected to write %d bytes but actually wrote %d",
			wLength, uStatus
		);
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}

DLLEXPORT(int) usbBulkRead(
	struct USBDevice *dev, uint8 endpoint, uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int numRead;
	int uStatus = libusb_bulk_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_IN | endpoint,
		(uint8 *)data,
		count,
		&numRead,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbBulkRead(): %s", libusb_error_name(uStatus));
		FAIL(uStatus);
	} else if ( (uint32)numRead != count ) {
		errRender(
			error,
			"usbBulkRead(): expected to read %d bytes but actually read %d (uStatus = %d): %s",
			count, numRead, uStatus, libusb_error_name(uStatus)
		);
		//#ifdef WIN32
		//	Sleep(5000);
		//#else
		//	usleep(5000000);
		//#endif
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}

DLLEXPORT(int) usbBulkWrite(
	struct USBDevice *dev, uint8 endpoint, const uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int numWritten;
	int uStatus = libusb_bulk_transfer(
		unwrap(dev),
		LIBUSB_ENDPOINT_OUT | endpoint,
		(uint8 *)data,
		count,
		&numWritten,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbBulkWrite(): %s", libusb_error_name(uStatus));
		FAIL(uStatus);
	} else if ( (uint32)numWritten != count ) {
		errRender(
			error,
			"usbBulkWrite(): expected to write %d bytes but actually wrote %d",
			count, numWritten
		);
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}

DLLEXPORT(struct libusb_context *) usbGetContext(void) {
	return m_ctx;
}
