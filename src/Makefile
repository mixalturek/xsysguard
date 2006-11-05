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
INSTALL_DATA 	?= ${INSTALL} -m 644

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

CFLAGS      += -Wall
CFLAGS      += -O3
CFLAGS      += -g

ALL_CFLAGS  := $(shell pkg-config --cflags glib-2.0) -Iinclude -rdynamic
ALL_LDFLAGS := $(shell pkg-config --libs glib-2.0) -ldl

ifndef DISABLE_XRENDER
XSG_LDFLAGS += -lXrender -lXcomposite
XSG_CFLAGS  += -D ENABLE_XRENDER=1
endif

COMMON_SRC     := main.c main.h 
COMMON_SRC     += conf.c conf.h
COMMON_SRC     += modules.c modules.h
COMMON_SRC     += utils.c
COMMON_SRC     += var.h
XSYSGUARD_SRC  := $(COMMON_SRC) xsysguard.c var.c widgets.c widgets.h
XSYSGUARDD_SRC := $(COMMON_SRC) xsysguardd.c vard.c


all: xsysguardd xsysguard
	$(MAKE) -C modules all

xsysguard: $(XSYSGUARD_SRC)
	$(CC) -o $@ $(shell pkg-config --cflags --libs imlib2 || imlib2-config --cflags --libs) \
	$(ALL_CFLAGS) $(XSG_CFLAGS) $(CFLAGS) \
	$(ALL_LDFLAGS) $(XSG_LDFLAGS) $(LDFLAGS) \
	$(filter %.c,$^) \
	-D CONFIG_DIR=\"$(CONFIG_DIR)\" \
	-D IMAGE_DIR=\"$(IMAGE_DIR)\" \
	-D FONT_DIR=\"$(FONT_DIR)\" \
	-include include/xsysguard.h

xsysguardd: $(XSYSGUARDD_SRC)
	$(CC) -o $@ \
	$(ALL_CFLAGS) $(CFLAGS) \
	$(ALL_LDFLAGS) $(LDFLAGS) \
	$(filter %.c,$^) \
	-include include/xsysguard.h

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

