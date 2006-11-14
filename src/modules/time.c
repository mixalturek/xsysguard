/* time.c
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
#include <time.h>

/******************************************************************************/

struct time_args {
	char *format;
	xsg_string_t *buffer;
};

static void *get_time(void *arg) {
	struct time_args *args;
	time_t curtime;
	struct tm *loctime;

	args = (struct time_args *) arg;

	curtime = time(0);
	loctime = localtime(&curtime);

	while (1) {
		size_t len;
		len = strftime(args->buffer->str, args->buffer->len, args->format, loctime);
		if (len < args->buffer->len)
			break;
		args->buffer = xsg_string_set_size(args->buffer, args->buffer->len + 8);
	}

	xsg_message("Get (\"%s\") \"%s\"", args->format, args->buffer->str);

	return (void *) args->buffer->str;
}

void parse(xsg_var_t *var, uint16_t id, uint64_t update) {
	struct time_args *args;

	args = g_new0(struct time_args, 1);

	args->format = xsg_conf_read_string();
	args->buffer = xsg_string_sized_new(8);

	var->type = XSG_STRING;
	var->func = get_time;
	var->args = args;
}

char *info() {
	return "format date and time with strftime(3)";
}

