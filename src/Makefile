#!/usr/bin/make -f
#
# This file is part of xsysguard <http://xsysguard.sf.net>
# Copyright (C) 2005 Sascha Wessel <sawe@users.sf.net>
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

SHELL 		:= /bin/sh
CC 		?= gcc
INSTALL 	?= install
INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA 	?= $(INSTALL) -m 644

VERSION := $(shell cat ../VERSION)
ifndef VERSION
$(error cannot read VERSION file)
endif
SVNVERSION := $(shell test -d ../.svn && svnversion -nc .. | sed -e 's/^[^:]*://;s/[A-Za-z]//')
ifdef SVNVERSION
VERSION := $(VERSION).r$(SVNVERSION)
endif

DATA_DIR    := $(datadir)/xsysguard

CONFIG_DIR  := $(DATA_DIR)/configs
IMAGE_DIR   := $(DATA_DIR)/images
FONT_DIR    := $(DATA_DIR)/fonts
MODULE_DIR  := $(libdir)/xsysguard

CONFIG_PATH ?= ~/.xsysguard/configs:$(CONFIG_DIR)
IMAGE_PATH  ?= ~/.xsysguard/images:$(IMAGE_DIR)
FONT_PATH   ?= ~/.xsysguard/fonts:$(FONT_DIR):~/.fonts:/usr/share/fonts/truetype:/usr/X11R6/lib/X11/fonts/TrueType
MODULE_PATH ?= ~/.xsysguard/modules:$(MODULE_DIR)

CFLAGS      += -Wall
CFLAGS      += -O3
CFLAGS      += -g

ALL_CFLAGS  := -Iinclude -rdynamic
ALL_LDFLAGS := -lm -ldl

ifndef DISABLE_XRENDER
XSG_LDFLAGS += -lXrender -lXcomposite
XSG_CFLAGS  += -D ENABLE_XRENDER=1
endif

COMMON_SRC     := include/xsysguard.h
COMMON_SRC     += main.c main.h 
COMMON_SRC     += conf.c conf.h
COMMON_SRC     += modules.c modules.h
COMMON_SRC     += rpn.c rpn.h
COMMON_SRC     += string.c list.c utils.c
COMMON_SRC     += var.h
XSYSGUARD_SRC  := $(COMMON_SRC) xsysguard.c var.c widgets.c widgets.h printf.c printf.h widget.h
XSYSGUARD_SRC  += widget_text.c widget_text.h
XSYSGUARDD_SRC := $(COMMON_SRC) xsysguardd.c vard.c


all: xsysguardd xsysguard
	$(MAKE) -C modules all

xsysguard: $(XSYSGUARD_SRC)
	$(CC) -o $@ $(shell pkg-config --cflags --libs imlib2 || imlib2-config --cflags --libs) \
	$(ALL_CFLAGS) $(XSG_CFLAGS) $(CFLAGS) $(ALL_LDFLAGS) $(XSG_LDFLAGS) $(LDFLAGS) \
	$(filter %.c,$^) \
	-D XSYSGUARD_CONFIG_PATH=\"$(CONFIG_PATH)\" \
	-D XSYSGUARD_MODULE_PATH=\"$(MODULE_PATH)\" \
	-D XSYSGUARD_IMAGE_PATH=\"$(IMAGE_PATH)\" \
	-D XSYSGUARD_FONT_PATH=\"$(FONT_PATH)\" \
	-D VERSION=\"$(VERSION)\"

xsysguardd: $(XSYSGUARDD_SRC)
	$(CC) -o $@ $(ALL_CFLAGS) $(CFLAGS) $(ALL_LDFLAGS) $(LDFLAGS) \
	$(filter %.c,$^) \
	-D XSYSGUARD_MODULE_PATH=\"$(MODULE_PATH)\" \
	-D VERSION=\"$(VERSION)\"

clean:
	rm -f xsysguardd
	rm -f xsysguard
	$(MAKE) -C modules clean

distclean: clean

install: all
	$(INSTALL_PROGRAM) -D xsysguardd $(DESTDIR)$(bindir)/xsysguardd
	$(INSTALL_PROGRAM) -D xsysguard $(DESTDIR)$(bindir)/xsysguard
	$(INSTALL_DATA) -D include/xsysguard.h $(DESTDIR)$(includedir)/xsysguard.h
	$(MAKE) -C modules install

.PHONY: all clean distclean install

