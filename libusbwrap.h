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
		USB_INIT,                      ///< LibUSB failed to initialise.
		USB_NO_BUSES,                  ///< Did you forget to call \c usbInitialise()?
		USB_DEVICE_NOT_FOUND,          ///< The specified device is not connected.
		USB_CANNOT_OPEN_DEVICE,        ///< The call to \c usb_open() failed.
		USB_CANNOT_SET_CONFIGURATION,  ///< Couldn't set the supplied configuration. Does it exist?
		USB_CANNOT_CLAIM_INTERFACE,    ///< Couldn't claim the supplied interface. Does it exist?
		USB_CANNOT_SET_ALTINT,         ///< Couldn't set the supplied alternate interface.
		USB_CANNOT_GET_DESCRIPTOR,     ///< Couldn't get the supplied descriptor.
		USB_CONTROL,                   ///< A USB control message failed.
		USB_BULK,                      ///< A USB bulk read or write failed.
		USB_ALLOC_ERR,                 ///< An allocation failed.
		USB_EMPTY_QUEUE,               ///< An attempt to take an item from an empty work queue.
		USB_ASYNC_SUBMIT,              ///< Async submission error.
		USB_ASYNC_EVENT,               ///< Async event error.
		USB_ASYNC_TRANSFER,            ///< Async transfer error.
		USB_ASYNC_SIZE,                ///< Async API transfers must be 64KiB or smaller.
		USB_TIMEOUT                    ///< An operation timed out.
	} USBStatus;
	//@}
	
	// Forward-declaration of the LibUSB handle
	struct USBDevice;

	struct AsyncTransferFlags {
		uint32 isRead : 1;
	};		

	struct CompletionReport {
		const uint8 *buffer;
		uint32 requestLength;
		uint32 actualLength;
		struct AsyncTransferFlags flags;
	};
	
	// ---------------------------------------------------------------------------------------------
	// Functions
	// ---------------------------------------------------------------------------------------------
	/**
	 * @name Functions
	 * @{
	 */	
	/**
	 * @brief Initialise LibUSB with the given log-level.
	 *
	 * This may fail if LibUSB cannot talk to the USB host controllers through its kernel driver.
	 *
	 * @param debugLevel 0->none, 1, 2, 3->lots.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_INIT if there were problems initialising LibUSB.
	 */
	DLLEXPORT(USBStatus) usbInitialise(
		int debugLevel, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Determine whether or not the specified device is attached.
	 *
	 * Scan the USB buses on the system looking for a device matching the supplied VID:PID. Return
	 * true as soon as a matching device is found, else return false if a match was not found.
	 *
	 * @param vp The Vendor ID and Product ID to look for (e.g "04B4:8613").
	 * @param isAvailable A pointer to a \c bool which will be set on exit to the result of the search.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns
	 *     - \c USB_SUCCESS if the operation completed successfully.
	 *     - \c USB_NO_BUSES if no buses could be found. Did you forget usbInitialise()?
	 *     - \c USB_INVALID_VIDPID if the supplied VID:PID could not be parsed.
	 */
	DLLEXPORT(USBStatus) usbIsDeviceAvailable(
		const char *vp, bool *isAvailable, const char **error
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
	 * @param devHandlePtr A pointer to a <code>struct USBDevice*</code> to be set on exit to
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
	DLLEXPORT(USBStatus) usbOpenDevice(
		const char *vp, int configuration, int iface, int alternateInterface,
		struct USBDevice **devHandlePtr, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Close a previously-opened device.
	 *
	 * @param dev The target device.
	 * @param iface The interface previously claimed.
	 */
	DLLEXPORT(void) usbCloseDevice(struct USBDevice *dev, int iface);

	
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
	DLLEXPORT(USBStatus) usbPrintConfiguration(
		struct USBDevice *deviceHandle, FILE *stream, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Validate a VID:PID string.
	 *
	 * Return true if the supplied string is of the form VVVV:PPPP where the Vs and Ps are hex
	 * digits, else return false.
	 *
	 * @param vp The VID:PID to verify.
	 * @returns \c true if the supplied string is a valid VID:PID.
	 */
	DLLEXPORT(bool) usbValidateVidPid(const char *vp);

	/**
	 * @brief Free an error allocated when one of the other functions fails.
	 *
	 * @param err An error message previously allocated by one of the other library functions.
	 */
	DLLEXPORT(void) usbFreeError(const char *err);

	/**
	 * @brief Read data from the control endpoint.
	 *
	 * @param dev The target device.
	 * @param bRequest The request field for the setup packet.
	 * @param wValue The value field for the setup packet.
	 * @param wIndex The index field for the setup packet.
	 * @param data Suitably-sized buffer for the IN data to be received from the device.
	 * @param wLength The length field for the setup packet. Buffer should be at least this size.
	 * @param timeout The timeout in milliseconds.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns An error code.
	 */
	DLLEXPORT(USBStatus) usbControlRead(
		struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
		uint8 *data, uint16 wLength,
		uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Write data to the control endpoint.
	 *
	 * @param dev The target device.
	 * @param bRequest The request field for the setup packet.
	 * @param wValue The value field for the setup packet.
	 * @param wIndex The index field for the setup packet.
	 * @param data Suitably-sized buffer containing the OUT data to be sent to the device.
	 * @param wLength The length field for the setup packet. Buffer should be at least this size.
	 * @param timeout The timeout in milliseconds.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns An error code.
	 */
	DLLEXPORT(USBStatus) usbControlWrite(
		struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
		const uint8 *data, uint16 wLength,
		uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Read data from a bulk endpoint.
	 *
	 * @param dev The target device.
	 * @param endpoint The endpoint to read from.
	 * @param data Suitably-sized buffer for the IN data to be received from the device.
	 * @param numBytes The number of bytes to read. Buffer should be at least this size.
	 * @param timeout The timeout in milliseconds.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns An error code.
	 */
	DLLEXPORT(USBStatus) usbBulkRead(
		struct USBDevice *dev, uint8 endpoint, uint8 *data, uint32 numBytes,
		uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	/**
	 * @brief Write data to a bulk endpoint.
	 *
	 * @param dev The target device.
	 * @param endpoint The endpoint to read from.
	 * @param data Suitably-sized buffer containing the OUT data to be sent to the device.
	 * @param numBytes The number of bytes to write. Buffer should be at least this size.
	 * @param timeout The timeout in milliseconds.
	 * @param error A pointer to a <code>char*</code> which will be set on exit to an allocated
	 *            error message if something goes wrong. Responsibility for this allocated memory
	 *            passes to the caller and must be freed with \c usbFreeError(). If \c error is
	 *            \c NULL, no allocation is done and no message is returned, but the return code
	 *            will still be valid.
	 * @returns An error code.
	 */
	DLLEXPORT(USBStatus) usbBulkWrite(
		struct USBDevice *dev, uint8 endpoint, const uint8 *data, uint32 numBytes,
		uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	// Caller supplies the buffer
	DLLEXPORT(USBStatus) usbBulkWriteAsync(
		struct USBDevice *dev, uint8 endpoint, const uint8 *buffer, uint32 length, uint32 timeout,
		const char **error
	) WARN_UNUSED_RESULT;

	// Library gives the caller a buffer to populate...
	DLLEXPORT(USBStatus) usbBulkWriteAsyncPrepare(
		struct USBDevice *dev, uint8 **buffer,
		const char **error
	) WARN_UNUSED_RESULT;

	// ...and then submit later.
	DLLEXPORT(USBStatus) usbBulkWriteAsyncSubmit(
		struct USBDevice *dev, uint8 endpoint, uint32 length, uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(USBStatus) usbBulkReadAsync(
		struct USBDevice *dev, uint8 endpoint, uint8 *buffer, uint32 length, uint32 timeout, const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(USBStatus) usbBulkAwaitCompletion(
		struct USBDevice *dev, struct CompletionReport *report, const char **error
	) WARN_UNUSED_RESULT;

	DLLEXPORT(size_t) usbNumOutstandingRequests(
		struct USBDevice *dev
	);

	//@}

#ifdef __cplusplus
}
#endif

#endif
