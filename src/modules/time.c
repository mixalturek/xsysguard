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

struct strftime_args {
	char *format;
	xsg_string_t *buffer;
};

/******************************************************************************/

static double get_time(void *arg) {
	return (double) time(NULL);
}

static char *get_strftime(void *arg) {
	struct strftime_args *args;
	time_t curtime;
	struct tm *loctime;

	args = (struct strftime_args *) arg;

	curtime = time(NULL);
	loctime = localtime(&curtime);

	while (1) {
		size_t len;
		len = strftime(args->buffer->str, args->buffer->len, args->format, loctime);
		if (likely(len < args->buffer->len))
			break;
		args->buffer = xsg_string_set_size(args->buffer, args->buffer->len + 8);
	}

	return args->buffer->str;
}

/******************************************************************************/

void parse_double(uint32_t id, uint64_t update, double (**func)(void *), void **arg) {
	*func = get_time;
	*arg = NULL;
}

void parse_string(uint32_t id, uint64_t update, char * (**func)(void *), void **arg) {
	struct strftime_args *args;

	args = xsg_new0(struct strftime_args, 1);

	args->format = xsg_conf_read_string();
	args->buffer = xsg_string_sized_new(8);

	*func = get_strftime;
	*arg = args;
}

char *info() {
	return "format date and time with strftime(3)";
}

