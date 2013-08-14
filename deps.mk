ifeq ($(OS),Windows_NT)
	LIBUSB_VERSION := 1.0.14
	MACHINE := $(shell echo $${MACHINE})
	ifeq ($(MACHINE),x64)
		LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win/MS64/dll/libusb-1.0.lib
		EXTRADLLS_REL := $(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win/MS64/dll/libusb-1.0.dll
	else ifeq ($(MACHINE),x86)
		LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win/MS32/dll/libusb-1.0.lib
		EXTRADLLS_REL := $(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win/MS32/dll/libusb-1.0.dll
	else ifndef MACHINE
$(error MACHINE must be set to x86 or x64)
	else
$(error Invalid target architecture: MACHINE=$(MACHINE))
	endif
	LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)
	EXTRADLLS_DBG := $(EXTRADLLS_REL)
	EXTRA_INCS := -I$(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win/include
	PRE_BUILD := $(ROOT)/3rd/libusbx-$(LIBUSB_VERSION)-win
else
	LINK_EXTRALIBS_REL := -lusb-1.0
	LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)
endif
