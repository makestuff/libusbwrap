#include <makestuff/libusbwrap.h>
#include <makestuff/liberror.h>
#include <stdio.h>

int main(int argc, const char *argv[]) {
	int retVal = 0;
	struct USBDevice *device = NULL;
	const char *error = NULL;
	const char *vp;
	USBStatus uStatus;
	if ( argc != 2 ) {
		fprintf(stderr, "Synopsis: %s <VID:PID>\n", argv[0]);
		FAIL_RET(1, cleanup);
	}
	vp = argv[1];
	uStatus = usbInitialise(0, &error);
	CHECK_STATUS(uStatus, 2, cleanup);
	uStatus = usbOpenDevice(vp, 1, 0, 0, &device, &error);
	CHECK_STATUS(uStatus, 3, cleanup);
	uStatus = usbPrintConfiguration(device, stdout, &error);
	CHECK_STATUS(uStatus, 4, cleanup);
cleanup:
	if ( device ) {
		usbCloseDevice(device, 0);
	}
	if ( error ) {
		fprintf(stderr, "%s: %s\n", argv[0], error);
		errFree(error);
	}
	return retVal;
}
