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

-include Makefile.config

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

VERSION := $(shell cat ./VERSION)
ifndef VERSION
$(error cannot read VERSION file)
endif
SVNVERSION := $(shell test -d ./.svn && svnversion -nc . | sed -e 's/^[^:]*://;s/[A-Za-z]//')
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

################################################################################

all: modules xsysguardd xsysguard doc data

modules:
	$(MAKE) -C src modules

xsysguardd:
	$(MAKE) -C src xsysguardd

xsysguard:
	$(MAKE) -C src xsysguard

doc:
	$(MAKE) -C doc all

data:
	$(MAKE) -C data all

clean:
	$(MAKE) -C src clean
	$(MAKE) -C doc clean
	$(MAKE) -C data clean

distclean: clean

install:
	$(MAKE) -C src install
	$(MAKE) -C doc install
	$(MAKE) -C data install

install-strip:
	$(MAKE) -C src install-strip
	$(MAKE) -C doc install
	$(MAKE) -C data install

config: clean
	$(RM) Makefile.config
	echo "prefix     := $(prefix)" >> Makefile.config
	echo "CC         := $(CC)" >> Makefile.config
	echo "CFLAGS     := $(CFLAGS)" >> Makefile.config
	echo "LDFLAGS    := $(LDFLAGS)" >> Makefile.config
	echo "DESTDIR    := $(DESTDIR)" >> Makefile.config

.PHONY: all clean distclean install install-strip xsysguardd xsysguard modules doc data config

