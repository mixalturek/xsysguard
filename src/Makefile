#!/usr/bin/make -f
#
# This file is part of xsysguard <http://xsysguard.sf.net>
# Copyright (C) 2005-2008 Sascha Wessel <sawe@users.sf.net>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

-include ../Makefile.config

prefix 		?= /usr/local
exec_prefix 	?= $(prefix)
bindir 		?= $(exec_prefix)/bin
libdir 		?= $(exec_prefix)/lib
datarootdir 	?= $(prefix)/share
datadir 	?= $(datarootdir)
includedir 	?= $(prefix)/include
docdir 		?= $(datarootdir)/doc/xsysguard
mandir 		?= $(datarootdir)/man
man1dir 	?= $(mandir)/man1

INSTALL 	?= install
INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA 	?= $(INSTALL) -m 644

VERSION := $(shell cat ../VERSION)
ifndef VERSION
$(error cannot read VERSION file)
endif
SVNVERSION := $(shell test -d ../.svn && svnversion -nc .. | sed -e 's/^[^:]*://;s/[A-Za-z]//')
ifdef SVNVERSION
VERSION := $(VERSION).svn$(SVNVERSION)
endif

DATA_DIR    := $(datadir)/xsysguard

CONFIG_DIR  := $(DATA_DIR)/configs
IMAGE_DIR   := $(DATA_DIR)/images
FONT_DIR    := $(DATA_DIR)/fonts
MODULE_DIR  := $(libdir)/xsysguard

CONFIG_PATH ?= ~/.xsysguard/configs:$(CONFIG_DIR)
IMAGE_PATH  ?= ~/.xsysguard/images:$(IMAGE_DIR)
FONT_PATH   ?= ~/.xsysguard/fonts:$(FONT_DIR)
MODULE_PATH ?= ~/.xsysguard/modules:$(MODULE_DIR)

################################################################################

UNAME ?= $(shell uname -s || uname)

################################################################################

CFLAGS      ?= -O2 -g -Wall
LDFLAGS     ?= 

ALL_CFLAGS  := -Iinclude -rdynamic -D$(UNAME) $(CFLAGS)
ALL_LDFLAGS := -fPIC -lm -ldl -lXext -lX11 $(LDFLAGS)

################################################################################

COMMON_SRC     += include/xsysguard.h types.h
COMMON_SRC     += main.c main.h 
COMMON_SRC     += conf.c conf.h
COMMON_SRC     += modules.c modules.h
COMMON_SRC     += rpn.c rpn.h
COMMON_SRC     += scanf.c scanf.h
COMMON_SRC     += utils.c string.c list.c hash.c buffer.c

XSYSGUARD_SRC  := $(COMMON_SRC) xsysguard.c
XSYSGUARD_SRC  += var.c var.h
XSYSGUARD_SRC  += printf.c printf.h
XSYSGUARD_SRC  += angle.c angle.h
XSYSGUARD_SRC  += imlib.c imlib.h
XSYSGUARD_SRC  += fontconfig.c fontconfig.h
XSYSGUARD_SRC  += update.c update.h
XSYSGUARD_SRC  += window.c window.h
XSYSGUARD_SRC  += xrender.c xrender.h
XSYSGUARD_SRC  += widgets.c widgets.h
XSYSGUARD_SRC  += widget_line.c widget_line.h
XSYSGUARD_SRC  += widget_rectangle.c widget_rectangle.h
XSYSGUARD_SRC  += widget_ellipse.c widget_ellipse.h
XSYSGUARD_SRC  += widget_polygon.c widget_polygon.h
XSYSGUARD_SRC  += widget_image.c widget_image.h
XSYSGUARD_SRC  += widget_barchart.c widget_barchart.h
XSYSGUARD_SRC  += widget_linechart.c widget_linechart.h
XSYSGUARD_SRC  += widget_areachart.c widget_areachart.h
XSYSGUARD_SRC  += widget_text.c widget_text.h

XSYSGUARDD_SRC := $(COMMON_SRC) xsysguardd.c
XSYSGUARDD_SRC += vard.c vard.h
XSYSGUARDD_SRC += writebuffer.c writebuffer.h

################################################################################

ifeq (0,$(MAKELEVEL))
$(info )
$(info **************** CONFIGURATION ****************)
$(info UNAME:         $(UNAME))
$(info VERSION:       $(VERSION))
$(info DESTDIR:       $(DESTDIR))
$(info prefix:        $(prefix))
$(info bindir:        $(bindir))
$(info mandir:        $(mandir))
$(info docdir:        $(docdir))
$(info datadir:       $(datadir))
$(info includedir:    $(includedir))
$(info CONFIG_PATH:   $(CONFIG_PATH))
$(info IMAGE_PATH:    $(IMAGE_PATH))
$(info FONT_PATH:     $(FONT_PATH))
$(info MODULE_PATH:   $(MODULE_PATH))
$(info ***********************************************)
$(info )
endif

################################################################################

XSYSGUARD_DEFINES := -D_GNU_SOURCE
XSYSGUARD_DEFINES += -D XSG_LOG_DOMAIN=NULL
XSYSGUARD_DEFINES += -D XSYSGUARD_VERSION=\"$(VERSION)\"
XSYSGUARD_DEFINES += -D XSYSGUARD_MODULE_PATH=\"$(MODULE_PATH)\"
XSYSGUARD_DEFINES += -D XSYSGUARD_CONFIG_PATH=\"$(CONFIG_PATH)\" 
XSYSGUARD_DEFINES += -D XSYSGUARD_IMAGE_PATH=\"$(IMAGE_PATH)\"
XSYSGUARD_DEFINES += -D XSYSGUARD_FONT_PATH=\"$(FONT_PATH)\"

XSYSGUARDD_DEFINES := -D_GNU_SOURCE
XSYSGUARDD_DEFINES += -D XSG_LOG_DOMAIN=NULL
XSYSGUARDD_DEFINES += -D XSYSGUARD_VERSION=\"$(VERSION)\"
XSYSGUARDD_DEFINES += -D XSYSGUARD_MODULE_PATH=\"$(MODULE_PATH)\"

################################################################################

all: modules xsysguardd xsysguard

xsysguard: $(XSYSGUARD_SRC)
	$(CC) -o $@ $(shell pkg-config --cflags --libs imlib2 || imlib2-config --cflags --libs) $(ALL_CFLAGS) $(ALL_LDFLAGS) $(filter %.c,$^) $(XSYSGUARD_DEFINES)

xsysguardd: $(XSYSGUARDD_SRC)
	$(CC) -o $@ $(ALL_CFLAGS) $(ALL_LDFLAGS) $(filter %.c,$^) $(XSYSGUARDD_DEFINES)

modules:
	$(MAKE) -C modules all

clean:
	$(RM) xsysguardd
	$(RM) xsysguard
	$(MAKE) -C modules clean

distclean: clean

install: all
	$(INSTALL_PROGRAM) -D xsysguardd $(DESTDIR)$(bindir)/xsysguardd
	$(INSTALL_PROGRAM) -D xsysguard $(DESTDIR)$(bindir)/xsysguard
	$(INSTALL_DATA) -D include/xsysguard.h $(DESTDIR)$(includedir)/xsysguard.h
	$(MAKE) -C modules install

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

.PHONY: all clean distclean install install-strip modules

