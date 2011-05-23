#
# Copyright (C) 2011 Chris McClelland
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
ROOT    := $(realpath ../..)
DEPS    := error
TYPE    := lib
SUBDIRS :=

ifeq ($(OS),Windows_NT)
	LIBUSB_VERSION := 1.2.4.0
	GENEXTRALIBS_REL := echo $(ROOT)/3rd/libusb-win32-bin-$(LIBUSB_VERSION)/lib/msvc/libusb.lib
	GENEXTRALIBS_DBG := $(GENEXTRALIBS_REL)
	EXTRA_INCS := -I$(ROOT)/3rd/libusb-win32-bin-$(LIBUSB_VERSION)/include
	PRE_BUILD := $(ROOT)/3rd/libusb-win32-bin-$(LIBUSB_VERSION)
else
	GENEXTRALIBS_REL := echo -lusb
	GENEXTRALIBS_DBG := $(GENEXTRALIBS_REL)
endif

-include $(ROOT)/common/top.mk
