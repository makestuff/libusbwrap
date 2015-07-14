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
#include <liberror.h>
#include <stdio.h>
#include "private.h"

// Print out the configuration tree
//
DLLEXPORT(USBStatus) usbPrintConfiguration(struct USBDevice *dev, FILE *stream, const char **error) {
	USBStatus retVal = USB_SUCCESS;
	uint8 descriptorBuffer[1024];
	uint8 *ptr = descriptorBuffer;
	uint8 endpointNum, interfaceNum;
	struct libusb_config_descriptor *configDesc;
	struct libusb_interface_descriptor *interfaceDesc;
	struct libusb_endpoint_descriptor *endpointDesc;
	int status = libusb_control_transfer(
		dev->handle,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE,
		LIBUSB_REQUEST_GET_DESCRIPTOR,  // bRequest
		0x0200,                         // wValue
		0x0000,                         // wIndex
		descriptorBuffer,
		256,                            // wLength
		5000                            // timeout (ms)
	);
	CHECK_STATUS(
		status <= 0, USB_CANNOT_GET_DESCRIPTOR, cleanup,
		"Failed to get descriptor: %s", libusb_error_name(status));
	configDesc = (struct libusb_config_descriptor *)ptr;
	fprintf(
		stream,
		"configDescriptor {\n    bLength = 0x%02X\n    bDescriptorType = 0x%02X\n    wTotalLength = 0x%04X\n    bNumInterfaces = 0x%02X\n    bConfigurationValue = 0x%02X\n    iConfiguration = 0x%02X\n    bmAttributes = 0x%02X\n    MaxPower = 0x%02X\n",
		configDesc->bLength,
		configDesc->bDescriptorType,
		littleEndian16(configDesc->wTotalLength),
		configDesc->bNumInterfaces,
		configDesc->bConfigurationValue,
		configDesc->iConfiguration,
		configDesc->bmAttributes,
		configDesc->MaxPower
	);
	ptr += configDesc->bLength;
	interfaceNum = configDesc->bNumInterfaces;
	while ( interfaceNum-- ) {
		interfaceDesc = (struct libusb_interface_descriptor *)ptr;
		fprintf(
			stream,
			"    interfaceDescriptor {\n        bLength = 0x%02X\n        bDescriptorType = 0x%02X\n        bInterfaceNumber = 0x%02X\n        bAlternateSetting = 0x%02X\n        bNumEndpoints = 0x%02X\n        bInterfaceClass = 0x%02X\n        bInterfaceSubClass = 0x%02X\n        bInterfaceProtocol = 0x%02X\n        iInterface = 0x%02X\n",
			interfaceDesc->bLength,
			interfaceDesc->bDescriptorType,
			interfaceDesc->bInterfaceNumber,
			interfaceDesc->bAlternateSetting,
			interfaceDesc->bNumEndpoints,
			interfaceDesc->bInterfaceClass,
			interfaceDesc->bInterfaceSubClass,
			interfaceDesc->bInterfaceProtocol,
			interfaceDesc->iInterface
		);
		ptr += interfaceDesc->bLength;			
		endpointNum = interfaceDesc->bNumEndpoints;
		while ( endpointNum-- ) {
			endpointDesc = (struct libusb_endpoint_descriptor *)ptr;
			fprintf(
				stream,
				"        endpointDescriptor {\n            bLength = 0x%02X\n            bDescriptorType = 0x%02X\n            bEndpointAddress = 0x%02X\n            bmAttributes = 0x%02X\n            wMaxPacketSize = 0x%02X\n            bInterval = 0x%02X\n            bRefresh = 0x%02X\n            bSynchAddress = 0x%02X\n        }\n",
				endpointDesc->bLength,
				endpointDesc->bDescriptorType,
				endpointDesc->bEndpointAddress,
				endpointDesc->bmAttributes,
				littleEndian16(endpointDesc->wMaxPacketSize),
				endpointDesc->bInterval,
				endpointDesc->bRefresh,
				endpointDesc->bSynchAddress
			);
			ptr += endpointDesc->bLength;
		}
		fprintf(stream, "    }\n");
	}
	fprintf(stream, "}\n");
cleanup:
	return retVal;
}
