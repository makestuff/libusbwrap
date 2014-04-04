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
	libusb_device_handle *retVal = NULL;
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
	int status, count;
	CHECK_STATUS(
		!m_ctx, USB_INIT, cleanup,
		"usbIsDeviceAvailable(): you forgot to call usbInitialise()!");
	count = (int)libusb_get_device_list(m_ctx, &devList);
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

struct TransferWrapper {
	struct libusb_transfer *transfer;
	int completed;
	struct AsyncTransferFlags flags;
	uint8 buffer[0x10000];  // can use this...
	uint8 *bufPtr;          // ...or this.
};
struct TransferWrapper *createTransfer(void) {
	struct TransferWrapper *retVal = (struct TransferWrapper *)calloc(1, sizeof(struct TransferWrapper));
	CHECK_STATUS(retVal == NULL, NULL, exit);
	retVal->transfer = libusb_alloc_transfer(0);
	CHECK_STATUS(retVal->transfer == NULL, NULL, freeWrap);
	return retVal;
freeWrap:
	free((void*)retVal);
exit:
	return NULL;
}

static void destroyTransfer(struct TransferWrapper *tx) {
	if ( tx ) {
		libusb_free_transfer(tx->transfer);
		free((void*)tx);
	}
}

// Find the descriptor of the first occurance of the specified device
//
DLLEXPORT(USBStatus) usbOpenDevice(
	const char *vp, int configuration, int iface, int altSetting,
	struct USBDevice **devHandlePtr, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	uint16 vid, pid, did;
	int status;
	struct USBDevice *newWrapper;
	struct libusb_device_handle *newHandle;
	CHECK_STATUS(
		!m_ctx, USB_INIT, exit,
		"usbOpenDevice(): you forgot to call usbInitialise()!");
	CHECK_STATUS(
		!usbValidateVidPid(vp), USB_INVALID_VIDPID, exit,
		"usbOpenDevice(): "FORMAT_ERR, vp);
	vid = (uint16)strtoul(vp, NULL, 16);
	pid = (uint16)strtoul(vp+5, NULL, 16);
	did = (uint16)((strlen(vp) == 14) ? strtoul(vp+10, NULL, 16) : 0x0000);
	newWrapper = (struct USBDevice *)malloc(sizeof(struct USBDevice));
	CHECK_STATUS(newWrapper == NULL, USB_ALLOC_ERR, exit, "usbOpenDevice(): Out of memory!");
	status = queueInit(&newWrapper->queue, 4, (CreateFunc)createTransfer, (DestroyFunc)destroyTransfer);
	CHECK_STATUS(status, USB_ALLOC_ERR, freeWrap, "usbOpenDevice(): Out of memory!");
	newHandle = libusbOpenWithVidPid(m_ctx, vid, pid, did, error);
	CHECK_STATUS(!newHandle, USB_CANNOT_OPEN_DEVICE, freeQueue, "usbOpenDevice()");
	status = libusb_set_configuration(newHandle, configuration);
	CHECK_STATUS(
		status < 0, USB_CANNOT_SET_CONFIGURATION, closeDev,
		"usbOpenDevice(): %s", libusb_error_name(status));
	status = libusb_claim_interface(newHandle, iface);
	CHECK_STATUS(
		status < 0, USB_CANNOT_CLAIM_INTERFACE, closeDev,
		"usbOpenDevice(): %s", libusb_error_name(status));
	status = libusb_set_interface_alt_setting(newHandle, iface, altSetting);
	CHECK_STATUS(
		status < 0, USB_CANNOT_SET_ALTINT, release,
		"usbOpenDevice(): %s", libusb_error_name(status));
	newWrapper->handle = newHandle;	
	*devHandlePtr = newWrapper;
	return USB_SUCCESS;
release:
	libusb_release_interface(newHandle, iface);
closeDev:
	libusb_close(newHandle);	
freeQueue:
	queueDestroy(&newWrapper->queue);
freeWrap:
	free((void*)newWrapper);
exit:
	*devHandlePtr = NULL;
	return retVal;
}

DLLEXPORT(void) usbCloseDevice(struct USBDevice *dev, int iface) {
	if ( dev ) {
		struct libusb_device_handle *ptr = dev->handle;
		libusb_release_interface(ptr, iface);
		libusb_close(ptr);
		queueDestroy(&dev->queue);
		free((void*)dev);
	}
}

DLLEXPORT(USBStatus) usbControlRead(
	struct USBDevice *dev, uint8 bRequest, uint16 wValue, uint16 wIndex,
	uint8 *data, uint16 wLength,
	uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	int status = libusb_control_transfer(
		dev->handle,
		LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	CHECK_STATUS(
		status == LIBUSB_ERROR_TIMEOUT, USB_TIMEOUT, cleanup,
		"usbControlRead(): Timeout!");
	CHECK_STATUS(
		status < 0, USB_CONTROL, cleanup,
		"usbControlRead(): %s", libusb_error_name(status));
	CHECK_STATUS(
		status != wLength, USB_CONTROL, cleanup,
		"usbControlRead(): Expected to read %d bytes but actually read %d", wLength, status);
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
		dev->handle,
		LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
		bRequest,
		wValue,
		wIndex,
		(uint8 *)data,
		wLength,
		timeout
	);
	CHECK_STATUS(
		status == LIBUSB_ERROR_TIMEOUT, USB_TIMEOUT, cleanup,
		"usbControlWrite(): Timeout");
	CHECK_STATUS(
		status < 0, USB_CONTROL, cleanup,
		"usbControlWrite(): %s", libusb_error_name(status));
	CHECK_STATUS(
		status != wLength, USB_CONTROL, cleanup,
		"usbControlWrite(): Expected to write %d bytes but actually wrote %d", wLength, status);
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
		dev->handle,
		LIBUSB_ENDPOINT_IN | endpoint,
		data,
		(int)count,
		&numRead,
		timeout
	);
	CHECK_STATUS(
		status == LIBUSB_ERROR_TIMEOUT, USB_TIMEOUT, cleanup,
		"usbBulkRead(): Timeout");
	CHECK_STATUS(
		status < 0, USB_BULK, cleanup,
		"usbBulkRead(): %s", libusb_error_name(status));
	CHECK_STATUS(
		(uint32)numRead != count, USB_BULK, cleanup,
		"usbBulkRead(): Expected to read %d bytes but actually read %d (status = %d): %s",
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
		dev->handle,
		LIBUSB_ENDPOINT_OUT | endpoint,
		(uint8 *)data,
		(int)count,
		&numWritten,
		timeout
	);
	CHECK_STATUS(
		status == LIBUSB_ERROR_TIMEOUT, USB_TIMEOUT, cleanup,
		"usbBulkWrite(): Timeout");
	CHECK_STATUS(
		status < 0, USB_BULK, cleanup,
		"usbBulkWrite(): %s", libusb_error_name(status));
	CHECK_STATUS(
		(uint32)numWritten != count, USB_BULK, cleanup,
		"usbBulkWrite(): Expected to write %d bytes but actually wrote %d (status = %d): %s",
		count, numWritten, status, libusb_error_name(status));
cleanup:
	return retVal;
}

static void LIBUSB_CALL bulk_transfer_cb(struct libusb_transfer *transfer) {
	int *completed = transfer->user_data;
	*completed = 1;
}

DLLEXPORT(USBStatus) usbBulkWriteAsync(
	struct USBDevice *dev, uint8 endpoint, const uint8 *buffer, uint32 length, uint32 timeout,
	const char **error)
{
	int retVal = USB_SUCCESS;
	struct TransferWrapper *wrapper;
	struct libusb_transfer *transfer;
	int *completed;
	int iStatus;
	USBStatus uStatus = queuePut(&dev->queue, (Item*)&wrapper);
	CHECK_STATUS(uStatus, uStatus, cleanup);
	transfer = wrapper->transfer;
	completed = &wrapper->completed;
	*completed = 0;
	wrapper->flags.isRead = 0;
	libusb_fill_bulk_transfer(
		transfer, dev->handle, LIBUSB_ENDPOINT_OUT | endpoint, (uint8 *)buffer, (int)length,
		bulk_transfer_cb, completed, timeout
	);
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	iStatus = libusb_submit_transfer(transfer);
	CHECK_STATUS(
		iStatus, USB_ASYNC_SUBMIT, cleanup,
		"usbBulkWriteAsync(): Submission error: %s", libusb_error_name(iStatus)
	);
	queueCommitPut(&dev->queue);
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkWriteAsyncPrepare(
	struct USBDevice *dev, uint8 **buffer, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	struct TransferWrapper *wrapper;
	USBStatus status = queuePut(&dev->queue, (Item*)&wrapper);
	CHECK_STATUS(status, status, cleanup, "usbBulkWriteAsyncPrepare(): Work queue insertion error");
	*buffer = wrapper->buffer;
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkWriteAsyncSubmit(
	struct USBDevice *dev, uint8 endpoint, uint32 length, uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	struct TransferWrapper *wrapper;
	struct libusb_transfer *transfer;
	int *completed;
	USBStatus uStatus;
	int iStatus;
	CHECK_STATUS(
		length > 0x10000, USB_ASYNC_SIZE, cleanup,
		"usbBulkWriteAsyncSubmit(): Transfer length exceeds 0x10000");
	uStatus = queuePut(&dev->queue, (Item*)&wrapper);
	CHECK_STATUS(uStatus, uStatus, cleanup, "usbBulkWriteAsyncSubmit(): Work queue insertion error");
	transfer = wrapper->transfer;
	completed = &wrapper->completed;
	*completed = 0;
	wrapper->flags.isRead = 0;
	libusb_fill_bulk_transfer(
		transfer, dev->handle, LIBUSB_ENDPOINT_OUT | endpoint, wrapper->buffer, (int)length,
		bulk_transfer_cb, completed, timeout
	);
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	iStatus = libusb_submit_transfer(transfer);
	CHECK_STATUS(
		iStatus, USB_ASYNC_SUBMIT, cleanup,
		"usbBulkWriteAsyncSubmit(): Submission error: %s", libusb_error_name(iStatus));
	queueCommitPut(&dev->queue);
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkReadAsync(
	struct USBDevice *dev, uint8 endpoint, uint8 *buffer, uint32 length, uint32 timeout, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	struct TransferWrapper *wrapper;
	struct libusb_transfer *transfer;
	int *completed;
	USBStatus uStatus;
	int iStatus;
	CHECK_STATUS(
		length > 0x10000, USB_ASYNC_SIZE, cleanup,
		"usbBulkReadAsync(): Transfer length exceeds 0x10000");
	uStatus = queuePut(&dev->queue, (Item*)&wrapper);
	CHECK_STATUS(uStatus, uStatus, cleanup, "usbBulkReadAsync(): Work queue insertion error");
	transfer = wrapper->transfer;
	completed = &wrapper->completed;
	*completed = 0;
	wrapper->flags.isRead = 1;
	if ( buffer ) {
		wrapper->bufPtr = buffer;
	} else {
		buffer = wrapper->buffer;
	}
	libusb_fill_bulk_transfer(
		transfer, dev->handle, LIBUSB_ENDPOINT_IN | endpoint, buffer, (int)length,
		bulk_transfer_cb, completed, timeout
	);
	transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
	iStatus = libusb_submit_transfer(transfer);
	CHECK_STATUS(
		iStatus, USB_ASYNC_SUBMIT, cleanup,
		"usbBulkReadAsync(): Submission error: %s", libusb_error_name(iStatus));
	queueCommitPut(&dev->queue);
cleanup:
	return retVal;
}

DLLEXPORT(USBStatus) usbBulkAwaitCompletion(
	struct USBDevice *dev, struct CompletionReport *report, const char **error)
{
	USBStatus retVal = USB_SUCCESS;
	struct TransferWrapper *wrapper;
	struct libusb_transfer *transfer;
	int *completed;
	int iStatus;
	struct timeval timeout = {UINT_MAX/1000, 1000*(UINT_MAX%1000)};
	                         // This horrible thing should boil down to a call to poll() with
	                         // timeout -1ms, which will be interpreted as "no timeout" on all
	                         // platforms.
	USBStatus uStatus = queueTake(&dev->queue, (Item*)&wrapper);
	CHECK_STATUS(uStatus, uStatus, exit, "usbBulkAwaitCompletion(): Work queue fetch error");
	transfer = wrapper->transfer;
	completed = &wrapper->completed;
	wrapper->bufPtr = NULL;
	while ( *completed == 0 ) {
		iStatus = libusb_handle_events_timeout_completed(m_ctx, &timeout, completed);
		if ( iStatus < 0 ) {
			if ( iStatus == LIBUSB_ERROR_INTERRUPTED ) {
				continue;
			}
			if ( libusb_cancel_transfer(transfer) == LIBUSB_SUCCESS ) {
				while ( *completed == 0 ) {
					if ( libusb_handle_events_timeout_completed(m_ctx, &timeout, completed) < 0 ) {
						break;
					}
				}
			}
			CHECK_STATUS(
				true, USB_ASYNC_EVENT, commit,
				"usbBulkAwaitCompletion(): Event error: %s", libusb_error_name(iStatus));
		}
	}

	report->buffer = transfer->buffer;
	report->requestLength = (uint32)transfer->length;
	report->actualLength = (uint32)transfer->actual_length;
	report->flags = wrapper->flags;

	switch ( transfer->status ) {
	case LIBUSB_TRANSFER_COMPLETED:
		iStatus = 0;
		break;
	case LIBUSB_TRANSFER_TIMED_OUT:
		iStatus = LIBUSB_ERROR_TIMEOUT;
		break;
	case LIBUSB_TRANSFER_STALL:
		iStatus = LIBUSB_ERROR_PIPE;
		break;
	case LIBUSB_TRANSFER_OVERFLOW:
		iStatus = LIBUSB_ERROR_OVERFLOW;
		break;
	case LIBUSB_TRANSFER_NO_DEVICE:
		iStatus = LIBUSB_ERROR_NO_DEVICE;
		break;
	case LIBUSB_TRANSFER_ERROR:
	case LIBUSB_TRANSFER_CANCELLED:
		iStatus = LIBUSB_ERROR_IO;
		break;
	default:
		iStatus = LIBUSB_ERROR_OTHER;
	}
	CHECK_STATUS(
		iStatus == LIBUSB_ERROR_TIMEOUT, USB_TIMEOUT, commit,
		"usbBulkAwaitCompletion(): Timeout");
	CHECK_STATUS(
		iStatus, USB_ASYNC_TRANSFER, commit,
		"usbBulkAwaitCompletion(): Transfer error: %s", libusb_error_name(iStatus));
commit:
	queueCommitTake(&dev->queue);
exit:
	return retVal;
}

DLLEXPORT(size_t) usbNumOutstandingRequests(struct USBDevice *dev) {
	return queueSize(&dev->queue);
}
