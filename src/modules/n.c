/* n.c
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
#define XSG_LOG_DOMAIN "number"

/******************************************************************************/

static double get_number(void *arg) {
	double d;

	d = * (double *) arg;

	xsg_debug("get_number: %f", d);

	return d;
}

/******************************************************************************/

void parse_hist(uint32_t count, uint32_t id, uint64_t update, double (**n)(void *), char *(**s)(void *), void **arg) {
	double *num;
	unsigned i;

	num = xsg_new(double, count);
	num[0] = xsg_conf_read_double();

	for (i = 0; i < count; i++) {
		arg[i] = (void *) num;
		n[i] = get_number;
	}
}

void parse(xsg_var_t *var, uint64_t update, double (**n)(void *), char *(**s)(void *), void **arg) {
	double *d;

	d = xsg_new(double, 1);
	*d = xsg_conf_read_double();
	*arg = (void *) d;
	*n = get_number;
}

char *info() {
	return "always returns the same number";
}

