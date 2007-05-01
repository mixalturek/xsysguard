/* num.c
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

/******************************************************************************/

static double get_double(void *arg) {
	double d;

	d = * (double *) arg;

	xsg_debug("get_double: %f", d);

	return d;
}

/******************************************************************************/

void parse_double(uint32_t id, uint64_t update, double (**func)(void *), void **arg) {
	double *d;

	d = xsg_new0(double, 1);
	*d = xsg_conf_read_double();
	*arg = (void *) d;
	*func = get_double;
}

char *info() {
	return "always returns the same number";
}

