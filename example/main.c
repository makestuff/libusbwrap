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
	int retVal = 0, uStatus;
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
	uStatus = bulkWriteAsyncPrepare(deviceHandle, &ptr);  // Write request command
	CHECK_STATUS(uStatus, 5, cleanup);
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
	uStatus = bulkWriteAsyncSubmit(deviceHandle, 2, (uint32)(ptr-buf), 1000);
	CHECK_STATUS(uStatus, 5, cleanup);

	// Submit the read
	uStatus = bulkReadAsync(deviceHandle, 6, CHUNK_SIZE, 9000);  // Read response data
	CHECK_STATUS(uStatus, 5, cleanup);

	// Wait for them to be serviced
	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 100, cleanup);
	printCompletionReport(&completionReport);
	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 101, cleanup);
	printCompletionReport(&completionReport);

cleanup:
	usbCloseDevice(deviceHandle, 0);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		usbFreeError(error);
	}
	return retVal;
}

int multiRead(void) {
	int retVal = 0, uStatus;
	struct USBDevice *deviceHandle = NULL;
	const char *error = NULL;
	const uint8 buf[] = {
		0x80,
		(uint8)(CHUNK_SIZE >> 24),
		(uint8)(CHUNK_SIZE >> 16),
		(uint8)(CHUNK_SIZE >> 8),
		(uint8)(CHUNK_SIZE & 0xFF)
	};
	struct CompletionReport completionReport;
	uint32 i, numBytes;
	double totalTime, speed;
	#ifdef WIN32
		LARGE_INTEGER tvStart, tvEnd, freq;
		DWORD_PTR mask = 1;
		SetThreadAffinityMask(GetCurrentThread(), mask);
		QueryPerformanceFrequency(&freq);
	#else
		struct timeval tvStart, tvEnd;
		long long startTime, endTime;
	#endif

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
	uStatus = bulkWriteAsync(deviceHandle, 2, buf, 5, 9000);  // Write request command
	CHECK_STATUS(uStatus, 5, cleanup);
	uStatus = bulkReadAsync(deviceHandle, 6, CHUNK_SIZE, 9000);  // Read response data
	CHECK_STATUS(uStatus, 5, cleanup);

	uStatus = bulkWriteAsync(deviceHandle, 2, buf, 5, 9000);  // Write request command
	CHECK_STATUS(uStatus, 5, cleanup);
	uStatus = bulkReadAsync(deviceHandle, 6, CHUNK_SIZE, 9000);  // Read response data
	CHECK_STATUS(uStatus, 5, cleanup);

	// On each iteration, await completion and send a new read command
	i = 100;
	numBytes = (i+2)*CHUNK_SIZE;
	while ( i-- ) {
		uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
		CHECK_STATUS(uStatus, 100, cleanup);
		printCompletionReport(&completionReport);
		uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
		CHECK_STATUS(uStatus, 101, cleanup);
		printCompletionReport(&completionReport);
		
		uStatus = bulkWriteAsync(deviceHandle, 2, buf, 5, 9000);  // Write request command
		CHECK_STATUS(uStatus, 5, cleanup);
		uStatus = bulkReadAsync(deviceHandle, 6, CHUNK_SIZE, 9000);  // Read response data
		CHECK_STATUS(uStatus, 5, cleanup);
	}

	// Wait for the stragglers...
	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 100, cleanup);
	printCompletionReport(&completionReport);
	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 101, cleanup);
	printCompletionReport(&completionReport);

	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 100, cleanup);
	printCompletionReport(&completionReport);
	uStatus = bulkAwaitCompletion(deviceHandle, &completionReport);
	CHECK_STATUS(uStatus, 101, cleanup);
	printCompletionReport(&completionReport);

	// Record stop time
	#ifdef WIN32
		QueryPerformanceCounter(&tvEnd);
		totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
		totalTime /= freq.QuadPart;
		speed = (double)numBytes / (1024*1024*totalTime);
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
		speed = (double)numBytes / (1024*1024*totalTime);
	#endif

	// Print execution time
	printf("Time: %0.3fs\nSpeed: %0.3fMiB/s\n", totalTime, speed);

cleanup:
	usbCloseDevice(deviceHandle, 0);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		usbFreeError(error);
	}
	return retVal;
}

int main(void) {
	return multiRead();
	//return bufferRead();
}
