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
INSTALL_DATA 	?= ${INSTALL} -m 644

VERSION ?= $(shell cat ../VERSION)
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

HTML_FILES := $(patsubst txt/%.txt,html/%.html,$(wildcard txt/*.txt))
MAN_FILES  := $(addprefix man1/, xsysguard.1 xsysguardd.1)
CSS_FILES  := $(wildcard asciidoc/stylesheets/*.css)
IMG_FILES  := $(wildcard images/*.png)

ASCIIDOC  := python asciidoc/asciidoc.py --unsafe -e --attribute=revision=$(VERSION)
HIGHLIGHT ?= source-highlight
LANG_DEF  := source-highlight/xsysguard.lang
HL_CONF   := asciidoc/source-highlight-filter.conf
HL := $(shell echo | $(HIGHLIGHT) --lang-def=$(LANG_DEF) > /dev/null 2>&1 && echo $(HL_CONF))

################################################################################

ifneq ($(HL_CONF),$(HL))
$(info )
$(info *************************************************************)
$(info * WARNING: No proper 'source-highlight' found.              *)
$(info * Thus no code highlighting in html files.                  *)
$(info * Download: http://www.gnu.org/software/src-highlite/       *)
$(info *************************************************************)
$(info )
HL := asciidoc/source-filter.conf
endif

################################################################################

all: html man1

man1: $(CURDIR)/man1 $(MAN_FILES)

html: $(CURDIR)/html $(HTML_FILES)

clean:
	$(RM) -r man1
	$(RM) -r html

distclean: clean

install:
	test -d man1 || $(MAKE) man1
	test -d html || $(MAKE) html
	$(INSTALL) -d $(DESTDIR)$(man1dir)
	$(INSTALL) -d $(DESTDIR)$(docdir)
	$(INSTALL) -d $(DESTDIR)$(docdir)/images
	$(INSTALL) -d $(DESTDIR)$(docdir)/stylesheets
	$(INSTALL_DATA) $(MAN_FILES) $(DESTDIR)$(man1dir)
	$(INSTALL_DATA) $(HTML_FILES) $(DESTDIR)$(docdir)
	$(INSTALL_DATA) $(IMG_FILES) $(DESTDIR)$(docdir)/images
	$(INSTALL_DATA) $(CSS_FILES) $(DESTDIR)$(docdir)/stylesheets

$(CURDIR)/man1:
	mkdir -p $@

$(CURDIR)/html:
	mkdir -p $@

html/%.1.html: txt/%.1.txt
	$(ASCIIDOC) \
	-b xhtml11 -d manpage \
	-f asciidoc/asciidoc.conf \
	-f asciidoc/xhtml11.conf \
	-f asciidoc/xhtml11-quirks.conf \
	-f asciidoc/layout.conf \
	-f $(HL) \
	-o $@ $<

html/%.html: txt/%.txt
	$(ASCIIDOC) \
	-b xhtml11 -d article \
	-f asciidoc/asciidoc.conf \
	-f asciidoc/xhtml11.conf \
	-f asciidoc/xhtml11-quirks.conf \
	-f asciidoc/layout.conf \
	-f $(HL) \
	-o $@ $<

man1/%.1: txt/%.1.txt
	$(ASCIIDOC) \
	-b docbook -d manpage \
	-f asciidoc/asciidoc.conf \
	-f asciidoc/docbook.conf \
	-o - $< | \
	xsltproc --nonet -o $@ asciidoc/manpage.xsl - || \
	$(ASCIIDOC) \
	-b docbook -d manpage \
	-f asciidoc/asciidoc.conf \
	-f asciidoc/docbook.conf \
	-o - $< | \
	xsltproc --load-trace -o $@ asciidoc/manpage.xsl -

.PHONY: all man1 html clean distclean install

