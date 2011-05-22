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
#ifndef LIBUSBWRAP_H
#define LIBUSBWRAP_H

#include <makestuff.h>
#include <stdio.h>

typedef enum {
	USB_SUCCESS = 0,
	USB_NO_BUSES,
	USB_DEVICE_NOT_FOUND,
	USB_CANNOT_OPEN_DEVICE,
	USB_CANNOT_SET_CONFIGURATION,
	USB_CANNOT_CLAIM_INTERFACE,
	USB_CANNOT_SET_ALTINT,
	USB_GET_DESCRIPTOR
} USBStatus;

struct usb_dev_handle;

void usbInitialise(void);

USBStatus usbIsDeviceAvailable(
	uint16 vid, uint16 pid, bool *isAvailable, const char **error
) WARN_UNUSED_RESULT;

USBStatus usbOpenDevice(
	uint16 vid, uint16 pid, int configuration, int iface, int alternateInterface,
	struct usb_dev_handle **devHandlePtr, const char **error
) WARN_UNUSED_RESULT;

USBStatus usbPrintConfiguration(
	struct usb_dev_handle *deviceHandle, FILE *stream, const char **error
) WARN_UNUSED_RESULT;

#endif
