ifeq ($(OS),Windows_NT)
	LIBUSB_VERSION := 1.0.18
	MACHINE := $(shell echo $${MACHINE})
	PLATFORM := $(shell echo $${PLATFORM})
	ifeq ($(MACHINE),x64)
		ifeq ($(PLATFORM),mingw)
			LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MinGW64/dll/libusb-1.0.dll
			EXTRADLLS_REL := $(LINK_EXTRALIBS_REL)
		else
			LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MS64/dll/libusb-1.0.lib
			EXTRADLLS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MS64/dll/libusb-1.0.dll
		endif
	else ifeq ($(MACHINE),x86)
		ifeq ($(PLATFORM),mingw)
			LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MinGW32/dll/libusb-1.0.dll
			EXTRADLLS_REL := $(LINK_EXTRALIBS_REL)
		else
			LINK_EXTRALIBS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MS32/dll/libusb-1.0.lib
			EXTRADLLS_REL := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/MS32/dll/libusb-1.0.dll
		endif
	else ifndef MACHINE
$(error MACHINE must be set to x86 or x64)
	else
$(error Invalid target architecture: MACHINE=$(MACHINE))
	endif
	LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)
	EXTRADLLS_DBG := $(EXTRADLLS_REL)
	EXTRA_INCS := -I$(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win/include
	PRE_BUILD := $(ROOT)/3rd/libusb-$(LIBUSB_VERSION)-win
else
	LINK_EXTRALIBS_REL := -lusb-1.0
	LINK_EXTRALIBS_DBG := $(LINK_EXTRALIBS_REL)
endif
