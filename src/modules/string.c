/* string.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <xsysguard.h>

/******************************************************************************/

static char *get_string(void *arg) {
	char *s;

	s = (char *) arg;

	xsg_debug("get_string: \"%s\"", s);

	return s;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	uint32_t i;
	char *s;

	s = xsg_conf_read_string();

	for (i = 0; i < n; i++) {
		arg[i] = (void *) s;
		str[i] = get_string;
	}
}

char *info(void) {
	return "always returns the same string";
}

