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

/******************************************************************************/

static double get_number(void *arg) {
	double d;

	d = * (double *) arg;

	xsg_debug("get_number: %f", d);

	return d;
}

/******************************************************************************/

void pparse(uint64_t update, xsg_var_t *var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	unsigned i;
	double *d;

	d = xsg_new(double, 1);
	d[0] = xsg_conf_read_double();

	for (i = 0; i < n; i++) {
		arg[i] = (void *) d;
		num[i] = get_number;
	}
}

void parse(uint64_t update, xsg_var_t *var, double (**num)(void *), char *(**str)(void *), void **arg) {
	double *d;

	d = xsg_new(double, 1);
	*d = xsg_conf_read_double();
	*arg = (void *) d;
	*num = get_number;
}

char *info(void) {
	return "always returns the same number";
}

