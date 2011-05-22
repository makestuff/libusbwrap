/* 
 * Copyright (C) 2009-2010 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
//#ifdef WIN32
//	#include <Windows.h>
//#else
//	#define _BSD_SOURCE
//	#include <unistd.h>
//#endif
#include <makestuff.h>
#include <liberror.h>
#include "libusbwrap.h"

// Initialise USB
//
void usbInitialise(void) {
	usb_init();
}

// Find the descriptor of the first occurance of the specified device
//
USBStatus usbIsDeviceAvailable(uint16 vid, uint16 pid, bool *isAvailable, const char **error) {
	struct usb_device *thisDevice;
	struct usb_bus *bus;
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
USBStatus usbOpenDevice(
	uint16 vid, uint16 pid, int configuration, int interface, int alternateInterface,
	UsbDeviceHandle **devHandlePtr, const char **error)
{
	USBStatus returnCode;
	struct usb_bus *bus;
	struct usb_device *thisDevice;
	UsbDeviceHandle *deviceHandle;
	int uStatus;
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
			uStatus = usb_claim_interface(deviceHandle, interface);
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
