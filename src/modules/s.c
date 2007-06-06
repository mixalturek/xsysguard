/* s.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005 Sascha Wessel <sawe@users.sf.net>
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

#undef XSG_LOG_DOMAIN
#define XSG_LOG_DOMAIN "string"

/******************************************************************************/

static char *get_string(void *arg) {
	char *s;

	s = (char *) arg;

	xsg_debug("get_string: \"%s\"", s);

	return s;
}

/******************************************************************************/

void parse_fill(uint32_t count, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
	unsigned i;
	char *str;

	str = xsg_conf_read_string();

	for (i = 0; i < count; i++) {
		arg[i] = (void *) str;
		s[i] = get_string;
	}
}

void parse(uint64_t update, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = xsg_conf_read_string();
	*s = get_string;
}

char *info(void) {
	return "always returns the same string";
}

int version(void) {
	return XSG_API_VERSION;
}

