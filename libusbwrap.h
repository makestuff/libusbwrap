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

/**
 * @file libusbwrap.h
 *
 * The <b>USBWrap</b> library makes it easier to use <a href="http://www.libusb.org">LibUSB</a> by
 * providing some useful utilities and defaults for common operations.
 */
#ifndef LIBUSBWRAP_H
#define LIBUSBWRAP_H

#include <makestuff.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	// ---------------------------------------------------------------------------------------------
	// Type definitions
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Enumerations
	 * @{
	 */
	/**
	 * Return codes from the functions.
	 */
	typedef enum {
		USB_SUCCESS = 0,               ///< The operation completed successfully.
		USB_INVALID_VIDPID,            ///< The VID:PID could not be parsed.
		USB_NO_BUSES,                  ///< Did you forget to call \c usbInitialise()?
		USB_DEVICE_NOT_FOUND,          ///< The specified device is not connected.
		USB_CANNOT_OPEN_DEVICE,        ///< The call to \c usb_open() failed.
		USB_CANNOT_SET_CONFIGURATION,  ///< Couldn't set the supplied configuration. Does it exist?
		USB_CANNOT_CLAIM_INTERFACE,    ///< Couldn't claim the supplied interface. Does it exist?
		USB_CANNOT_SET_ALTINT,         ///< Couldn't set the supplied alternate interface.
		USB_GET_DESCRIPTOR             ///< Couldn't get the supplied descriptor.
	} USBStatus;
	//@}
	
	// Forward-declaration of the LibUSB handle
	struct usb_dev_handle;
	
	// ---------------------------------------------------------------------------------------------
	// Functions
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Functions
	 * @{
	 */	
	/**
	 * @brief Initialise LibUSB.
	 */
	void usbInitialise(void);

	/**
	 * @brief Determine whether or not the specified device is attached.
	 *
	 * Scan the USB buses on the system looking for a device matching the supplied VID:PID. Return
	 * true as soon as a matching device is found, else return false if a match was not found.
	 *
	 * @param vid The Vendor ID to look for.
	 * @param pid The Product ID to look for.
	 * @param isAvailable A pointer to a \c bool which will be set on exit to the result of the search.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_NO_BUSES if no buses could be found. Did you forget usbInitialise()?
	 */
	USBStatus usbIsDeviceAvailable(
		uint16 vid, uint16 pid, bool *isAvailable, const char **error
	) WARN_UNUSED_RESULT;
	
	/**
	 * @brief Open a connection to the device matching a numeric VID:PID.
	 *
	 * Scan the USB buses on the system looking for a device matching the supplied VID:PID. If found,
	 * establish a connection to the device and enable the supplied configuration and interface.
	 *
	 * @param vid The Vendor ID to look for.
	 * @param pid The Product ID to look for.
	 * @param configuration The USB configuration to enable on the device.
	 * @param iface The USB interface to enable on the device.
	 * @param alternateInterface The USB alternate interface to choose.
	 * @param devHandlePtr A pointer to a <code>struct usb_dev_handle*</code> to be set on exit to
	 *            point to the newly-allocated LibUSB structure.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_NO_BUSES if no buses could be found. Did you forget usbInitialise()?
	 *     - \c USB_DEVICE_NOT_FOUND if a device matching the VID:PID was not found.
	 *     - \c USB_CANNOT_OPEN_DEVICE if the call to \c usb_open() failed.
	 *     - \c USB_CANNOT_SET_CONFIGURATION if \c usb_set_configuration() failed.
	 *     - \c USB_CANNOT_CLAIM_INTERFACE if \c usb_claim_interface() failed.
	 *     - \c USB_CANNOT_SET_ALTINT if \c usb_set_altinterface() failed.
	 */
	USBStatus usbOpenDevice(
		uint16 vid, uint16 pid, int configuration, int iface, int alternateInterface,
		struct usb_dev_handle **devHandlePtr, const char **error
	) WARN_UNUSED_RESULT;
	
	/**
	 * @brief Open a connection to the device matching a string VID:PID.
	 *
	 * Scan the USB buses on the system looking for a device matching the supplied VID:PID. If found,
	 * establish a connection to the device and enable the supplied configuration and interface.
	 *
	 * @param vp The Vendor ID and Product ID to look for (e.g "04B4:8613").
	 * @param configuration The USB configuration to enable on the device.
	 * @param iface The USB interface to enable on the device.
	 * @param alternateInterface The USB alternate interface to choose.
	 * @param devHandlePtr A pointer to a <code>struct usb_dev_handle*</code> to be set on exit to
	 *            point to the newly-allocated LibUSB structure.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_INVALID_VIDPID if the supplied VID:PID could not be parsed.
	 *     - \c USB_NO_BUSES if no buses could be found. Did you forget usbInitialise()?
	 *     - \c USB_DEVICE_NOT_FOUND if a device matching the VID:PID was not found.
	 *     - \c USB_CANNOT_OPEN_DEVICE if the call to \c usb_open() failed.
	 *     - \c USB_CANNOT_SET_CONFIGURATION if \c usb_set_configuration() failed.
	 *     - \c USB_CANNOT_CLAIM_INTERFACE if \c usb_claim_interface() failed.
	 *     - \c USB_CANNOT_SET_ALTINT if \c usb_set_altinterface() failed.
	 */
	USBStatus usbOpenDeviceVP(
		const char *vp, int configuration, int iface, int alternateInterface,
		struct usb_dev_handle **devHandlePtr, const char **error
	) WARN_UNUSED_RESULT;
	
	/**
	 * @brief Print a human-friendly hierarchical representation of a device's USB configuration.
	 * @param deviceHandle A pointer returned by \c usbOpenDevice() or \c usbOpenDeviceVP().
	 * @param stream A \c stdio.h stream to write the configuration to.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_GET_DESCRIPTOR if a referenced descriptor cannot be retrieved from the device.
	 */
	USBStatus usbPrintConfiguration(
		struct usb_dev_handle *deviceHandle, FILE *stream, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Free an error allocated when one of the other functions fails.
	 *
	 * @param err An error message previously allocated by one of the other library functions.
	 */
	void usbFreeError(const char *err);
	//@}

#ifdef __cplusplus
}
#endif

#endif
