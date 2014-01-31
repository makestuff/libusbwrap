/* 
 * Copyright (C) 2009-2011 Chris McClelland
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libusbwrap.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif
#include "random.h"

#ifdef NDEBUG
	static inline void printCompletionReport(const struct CompletionReport *report) {
		// Do nothing
		(void)report;
	}
#else
	void printCompletionReport(const struct CompletionReport *report) {
		const uint8 *ptr = report->buffer;
		uint32 len = report->actualLength;
		printf(
			"%s completed: requested %d, actually transferred %d:",
			report->flags.isRead ? "Read" : "Write",
			report->requestLength, report->actualLength
		);
		if ( len > 8 ) {
			len = 8;
			while ( len-- ) {
				printf(" %02X", *ptr++);
			}
			printf("...\n");
		} else {
			while ( len-- ) {
				printf(" %02X", *ptr++);
			}
			printf("\n");
		}
	}
#endif

#define CHUNK_SIZE 4096

int bufferRead(void) {
	int retVal = 0;
	USBStatus uStatus;
	struct USBDevice *deviceHandle = NULL;
	const char *error = NULL;
	uint8 *ptr;
	const uint8 *buf;
	struct CompletionReport completionReport;

	// Init library
	uStatus = usbInitialise(0, &error);
	CHECK_STATUS(uStatus, 1, cleanup);

	// Open device
	uStatus = usbOpenDevice("1d50:602b", 1, 0, 0, &deviceHandle, &error);
	CHECK_STATUS(uStatus, 2, cleanup);

	// Select CommFPGA conduit (FX2 slave FIFOs = 0x0001)
	uStatus = usbControlWrite(deviceHandle, 0x80, 0x0000, 0x0001, NULL, 0, 1000, &error);
	CHECK_STATUS(uStatus, 3, cleanup);

	// Get the next available 64KiB write buffer
	uStatus = usbBulkWriteAsyncPrepare(deviceHandle, &ptr, &error);  // Write request command
	CHECK_STATUS(uStatus, 4, cleanup);
	buf = ptr;

	// Populate the buffer with a couple of FPGA write commands and one FPGA read command
   *ptr++ = 0x00; // write ch0
	*ptr++ = (uint8)(CHUNK_SIZE >> 24);
	*ptr++ = (uint8)(CHUNK_SIZE >> 16);
	*ptr++ = (uint8)(CHUNK_SIZE >> 8);
	*ptr++ = (uint8)(CHUNK_SIZE & 0xFF);
	memcpy(ptr, randomData, CHUNK_SIZE);
	ptr += CHUNK_SIZE;

   *ptr++ = 0x00; // write ch0
	*ptr++ = (uint8)(CHUNK_SIZE >> 24);
	*ptr++ = (uint8)(CHUNK_SIZE >> 16);
	*ptr++ = (uint8)(CHUNK_SIZE >> 8);
	*ptr++ = (uint8)(CHUNK_SIZE & 0xFF);
	memcpy(ptr, randomData+CHUNK_SIZE, CHUNK_SIZE);
	ptr += CHUNK_SIZE;

   *ptr++ = 0x80; // read ch0
	*ptr++ = (uint8)(CHUNK_SIZE >> 24);
	*ptr++ = (uint8)(CHUNK_SIZE >> 16);
	*ptr++ = (uint8)(CHUNK_SIZE >> 8);
	*ptr++ = (uint8)(CHUNK_SIZE & 0xFF);
	
	// Submit the write
	uStatus = usbBulkWriteAsyncSubmit(deviceHandle, 2, (uint32)(ptr-buf), 1000, &error);
	CHECK_STATUS(uStatus, 5, cleanup);

	// Submit the read
	uStatus = usbBulkReadAsync(deviceHandle, 6, NULL, CHUNK_SIZE, 9000, &error);  // Read response data
	CHECK_STATUS(uStatus, 6, cleanup);

	// Wait for them to be serviced
	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 7, cleanup);
	printCompletionReport(&completionReport);
	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 8, cleanup);
	printCompletionReport(&completionReport);
cleanup:
	usbCloseDevice(deviceHandle, 0);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		usbFreeError(error);
	}
	return retVal;
}

int multiRead(uint32 reqCount, uint32 reqSize, double *speed) {
	int retVal = 0;
	USBStatus uStatus;
	struct USBDevice *deviceHandle = NULL;
	const char *error = NULL;
	uint8 buf[5];
	struct CompletionReport completionReport;
	uint32 numBytes;
	double totalTime;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif

	// We can make one FPGA request message and re-use it
	buf[0] = 0x80;  // write ch0
	buf[1] = (uint8)(reqSize >> 24);
	buf[2] = (uint8)(reqSize >> 16);
	buf[3] = (uint8)(reqSize >> 8);
	buf[4] = (uint8)(reqSize & 0xFF);

	// Init library
	uStatus = usbInitialise(0, &error);
	CHECK_STATUS(uStatus, 1, cleanup);

	// Open device
	uStatus = usbOpenDevice("1d50:602b", 1, 0, 0, &deviceHandle, &error);
	CHECK_STATUS(uStatus, 2, cleanup);

	// Select CommFPGA conduit (FX2 slave FIFOs = 0x0001)
	uStatus = usbControlWrite(deviceHandle, 0x80, 0x0000, 0x0001, NULL, 0, 1000, &error);
	CHECK_STATUS(uStatus, 3, cleanup);

	// Record start time
	#ifdef WIN32
		QueryPerformanceCounter(&tvStart);
	#else
		gettimeofday(&tvStart, NULL);
	#endif

	// Send a couple of read commands to the FPGA
	uStatus = usbBulkWriteAsync(deviceHandle, 2, buf, 5, 9000, &error);  // Write request command
	CHECK_STATUS(uStatus, 4, cleanup);
	uStatus = usbBulkReadAsync(deviceHandle, 6, NULL, reqSize, 9000, &error);  // Read response data
	CHECK_STATUS(uStatus, 5, cleanup);

	uStatus = usbBulkWriteAsync(deviceHandle, 2, buf, 5, 9000, &error);  // Write request command
	CHECK_STATUS(uStatus, 6, cleanup);
	uStatus = usbBulkReadAsync(deviceHandle, 6, NULL, reqSize, 9000, &error);  // Read response data
	CHECK_STATUS(uStatus, 7, cleanup);

	// On each iteration, await completion and send a new read command
	numBytes = (reqCount+2)*reqSize;
	while ( reqCount-- ) {
		uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
		CHECK_STATUS(uStatus, 8, cleanup);
		printCompletionReport(&completionReport);
		uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
		CHECK_STATUS(uStatus, 9, cleanup);
		printCompletionReport(&completionReport);
		
		uStatus = usbBulkWriteAsync(deviceHandle, 2, buf, 5, 9000, &error);  // Write request command
		CHECK_STATUS(uStatus, 10, cleanup);
		uStatus = usbBulkReadAsync(deviceHandle, 6, NULL, reqSize, 9000, &error);  // Read response data
		CHECK_STATUS(uStatus, 11, cleanup);
	}

	// Wait for the stragglers...
	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 12, cleanup);
	printCompletionReport(&completionReport);
	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 13, cleanup);
	printCompletionReport(&completionReport);

	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 14, cleanup);
	printCompletionReport(&completionReport);
	uStatus = usbBulkAwaitCompletion(deviceHandle, &completionReport, &error);
	CHECK_STATUS(uStatus, 15, cleanup);
	printCompletionReport(&completionReport);

	// Record stop time
	#ifdef WIN32
		QueryPerformanceCounter(&tvEnd);
		totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
		totalTime /= freq.QuadPart;
		*speed = (double)numBytes / (1024*1024*totalTime);
	#else
		gettimeofday(&tvEnd, NULL);
		startTime = tvStart.tv_sec;
		startTime *= 1000000;
		startTime += tvStart.tv_usec;
		endTime = tvEnd.tv_sec;
		endTime *= 1000000;
		endTime += tvEnd.tv_usec;
		totalTime = (double)(endTime - startTime);
		totalTime /= 1000000;  // convert from uS to S.
		*speed = (double)numBytes / (1024*1024*totalTime);
	#endif

cleanup:
	usbCloseDevice(deviceHandle, 0);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		usbFreeError(error);
	}
	return retVal;
}

int main(int argc, const char *argv[]) {
	int retVal;
	uint32 reqCount;
	uint32 reqSize;
	double speed;
	if ( argc != 3 ) {
		fprintf(stderr, "Synopsis: example <reqCount> <reqSize>\n");
		FAIL(1, cleanup);
	}
	reqCount = (uint32)strtoul(argv[1], NULL, 10);
	if ( reqCount < 2 ) {
		fprintf(stderr, "example: Please ensure reqCount>=2\n");
		FAIL(2, cleanup);
	}
	reqSize = (uint32)strtoul(argv[2], NULL, 10);
	if ( reqSize < 1024 || reqSize > 65536 ) {
		fprintf(stderr, "example: Please ensure 1024<=reqSize<=65536\n");
		FAIL(3, cleanup);
	}

	retVal = multiRead(reqCount-2, reqSize, &speed);
	CHECK_STATUS(retVal, retVal, cleanup);

	//printf("reqCount = %d; reqSize = %d; speed = %0.3fMiB/s\n", reqCount, reqSize, speed);
	printf("%d,%0.3f\n", reqSize, speed);

	//return bufferRead();
cleanup:
	return retVal;
}
