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
#include <stdio.h>
#include <string.h>
#include <makestuff.h>
#include <liberror.h>
#ifdef WIN32
	#include <lusb0_usb.h>
#else
	#include <usb.h>
#endif
#include "libusbwrap.h"

// Return true if vp is VVVV:PPPP where V and P are hex digits:
//
DLLEXPORT(bool) usbValidateVidPid(const char *vp) {
	int i;
	char ch;
	if ( !vp ) {
		return false;
	}
	if ( strlen(vp) != 9 ) {
		return false;
	}
	if ( vp[4] != ':' ) {
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
	return true;
}

// Initialise USB
//
DLLEXPORT(void) usbInitialise(void) {
	usb_init();
}

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(int) usbIsDeviceAvailable(const char *vp, bool *isAvailable, const char **error) {
	struct usb_device *thisDevice;
	struct usb_bus *bus;
	uint16 vid, pid;
	if ( !usbValidateVidPid(vp) ) {
		errRender(error, "The supplied VID:PID \"%s\" is invalid; it should look like 04B4:8613", vp);
		return USB_INVALID_VIDPID;
	}
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	usb_find_busses();
	bus = usb_get_busses();
	if ( !bus ) {
		errRender(error, "No USB buses found. Did you forget to call usbInitialise()?");
		return USB_NO_BUSES;
	}
	usb_find_devices();
	bus = usb_get_busses();
	do {
		thisDevice = bus->devices;
		while ( thisDevice && (thisDevice->descriptor.idVendor != vid ||
							   thisDevice->descriptor.idProduct != pid) )
		{
			thisDevice = thisDevice->next;
		}
		bus = bus->next;
	} while ( bus && !thisDevice );  // break out if I run out of buses, or device found
	*isAvailable = thisDevice ? true : false;
	return USB_SUCCESS;
}

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(int) usbOpenDevice(
	const char *vp, int configuration, int iface, int alternateInterface,
	struct usb_dev_handle **devHandlePtr, const char **error)
{
	USBStatus returnCode;
	struct usb_bus *bus;
	struct usb_device *thisDevice;
	struct usb_dev_handle *deviceHandle;
	int uStatus;
	uint16 vid, pid;
	if ( !usbValidateVidPid(vp) ) {
		errRender(error, "The supplied VID:PID \"%s\" is invalid; it should look like 04B4:8613", vp);
		return USB_INVALID_VIDPID;
	}
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	usb_find_busses();
	bus = usb_get_busses();
	if ( bus ) {
		// This system has one or more USB buses...let's step through them looking for the specified VID/PID
		//
		usb_find_devices();
		do {
			thisDevice = bus->devices;
			while ( thisDevice && (thisDevice->descriptor.idVendor != vid || thisDevice->descriptor.idProduct != pid) ) {
				thisDevice = thisDevice->next;
			}
			bus = bus->next;
		} while ( bus && !thisDevice );  // will break out if I run out of buses, or if device found
		if ( !thisDevice ) {
			// The VID/PID was not found after scanning all buses
			//
			*devHandlePtr = NULL;
			errRender(error, "Device %04X:%04X not found", vid, pid);
			FAIL(USB_DEVICE_NOT_FOUND);
		} else {
			// The VID/PID was found; let's see if we can open the device
			//
			deviceHandle = usb_open(thisDevice);
			if ( deviceHandle == NULL ) {
				*devHandlePtr = NULL;
				errRender(error, "usb_open(): %s", usb_strerror());
				FAIL(USB_CANNOT_OPEN_DEVICE);
			}
			*devHandlePtr = deviceHandle;  // Return the valid device handle anyway, even if subsequent operations fail
			uStatus = usb_set_configuration(deviceHandle, configuration);
			if ( uStatus < 0 ) {
				errRender(error, "usb_set_configuration(): %s", usb_strerror());
				FAIL(USB_CANNOT_SET_CONFIGURATION);
			}
			uStatus = usb_claim_interface(deviceHandle, iface);
			if ( uStatus < 0 ) {
				errRender(error, "usb_claim_interface(): %s", usb_strerror());
				FAIL(USB_CANNOT_CLAIM_INTERFACE);
			}
			if ( alternateInterface ) {
				uStatus = usb_set_altinterface(deviceHandle, alternateInterface);
				if ( uStatus < 0 ) {
					errRender(error, "usb_set_altinterface(): %s", usb_strerror());
					FAIL(USB_CANNOT_SET_ALTINT);
				}
			}
			return USB_SUCCESS;
		}
	}
	// No USB buses on this system!?!?
	//
	*devHandlePtr = NULL;
	errRender(error, "No USB buses found. Did you forget to call usbInitialise()?");
	returnCode = USB_NO_BUSES;
cleanup:
	return returnCode;
}

DLLEXPORT(void) usbCloseDevice(struct usb_dev_handle *dev, int iface) {
	usb_release_interface(dev, iface);
	usb_close(dev);
}

DLLEXPORT(int) usbControlRead(
	struct usb_dev_handle *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = usb_control_msg(
		dev,
		USB_ENDPOINT_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(char *)data,
		wLength,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbControlRead(): %s", usb_strerror());
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
	struct usb_dev_handle *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	const uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = usb_control_msg(
		dev,
		USB_ENDPOINT_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(char *)data,
		wLength,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbControlWrite(): %s", usb_strerror());
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
	struct usb_dev_handle *dev, uint8 endpoint, uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = usb_bulk_read(
		dev,
		USB_ENDPOINT_IN | endpoint,
		(char *)data,
		count,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbBulkRead(): %s", usb_strerror());
		FAIL(uStatus);
	} else if ( (uint32)uStatus != count ) {
		errRender(
			error,
			"usbBulkRead(): expected to read %d bytes but actually read %d",
			count, uStatus
		);
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}

DLLEXPORT(int) usbBulkWrite(
	struct usb_dev_handle *dev, uint8 endpoint, const uint8 *data, uint32 count,
	uint32 timeout, const char **error)
{
	int returnCode = USB_SUCCESS;
	int uStatus = usb_bulk_write(
		dev,
		USB_ENDPOINT_OUT | endpoint,
		(char *)data,
		count,
		timeout
	);
	if ( uStatus < 0 ) {
		errRender(
			error, "usbBulkWrite(): %s", usb_strerror());
		FAIL(uStatus);
	} else if ( (uint32)uStatus != count ) {
		errRender(
			error,
			"usbBulkWrite(): expected to read %d bytes but actually read %d",
			count, uStatus
		);
		FAIL(USB_CONTROL);
	}
cleanup:
	return returnCode;
}
