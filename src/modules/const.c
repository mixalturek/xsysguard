/* const.c
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

static void *get_int(void *arg) {
	g_message("Get (int) %" PRId64, *(int64_t *) arg);
	return arg;
}

static void *get_double(void *arg) {
	g_message("Get (double) %f", *(double *) arg);
	return arg;
}

static void *get_string(void *arg) {
	g_message("Get (string) \"%s\"", (char *) arg);
	return arg;
}

void parse(xsg_var_t *var, uint16_t id, uint64_t update) {
	if (xsg_conf_find_command("int")) {
		int64_t *i;

		i = g_new0(int64_t, 1);
		*i = xsg_conf_read_int();

		var->type = XSG_INT;
		var->func = get_int;
		var->args = (void *) i;
	} else if (xsg_conf_find_command("double")) {
		double *d;

		d = g_new0(double, 1);
		*d = xsg_conf_read_double();

		var->type = XSG_DOUBLE;
		var->func = get_double;
		var->args = (void *) d;
	} else if (xsg_conf_find_command("string")) {
		char *s;

		s = xsg_conf_read_string();

		var->type = XSG_STRING;
		var->func = get_string;
		var->args = (void *) s;
	} else {
		xsg_conf_error("int, double or string");
	}
}

char *info() {
	return "always returns the same constant value";
}

