#!/usr/bin/make -f
#
# This file is part of xsysguard <http://xsysguard.sf.net>
# Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
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

-include ../../Makefile.config

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

VERSION := $(shell cat ../../VERSION)
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

CFLAGS      ?= -O2 -g
LDFLAGS     ?= 

ALL_CFLAGS  := -I../include -D$(UNAME) $(CFLAGS)
ALL_LDFLAGS := -fPIC -shared $(LDFLAGS)

################################################################################

MODULES := $(patsubst %.mk,%.Module,$(wildcard *.mk))

################################################################################

ifndef RECURSIVE_MODULE_BUILD
export RECURSIVE_MODULE_BUILD := 1

all: $(MODULES)

clean:
	$(RM) *.o *.so

distclean: clean

install:
	$(INSTALL) -d $(DESTDIR)$(MODULE_DIR)
	$(INSTALL_DATA) -t $(DESTDIR)$(MODULE_DIR) $(wildcard *.so)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install

endif

%.Module:%.mk
	@-$(MAKE) --no-print-directory --file=$^

%.so: %.c
	$(CC) -o $@ $^ $(ALL_CFLAGS) $(ALL_LDFLAGS)

.PHONY: all clean distclean install install-strip

