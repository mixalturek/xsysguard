/* file.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2008 Sascha Wessel <sawe@users.sf.net>
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
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/******************************************************************************/

typedef struct _file_buffer_t {
	char *filename;
	uint64_t update;
	xsg_buffer_t *buffer;
} file_buffer_t;

/******************************************************************************/

static xsg_list_t *file_buffer_list = NULL;

/******************************************************************************/

static xsg_buffer_t *
find_file_buffer(const char *filename, uint64_t update)
{
	xsg_list_t *l;
	file_buffer_t *f;

	for (l = file_buffer_list; l; l = l->next) {
		f = l->data;

		if (!strcmp(f->filename, filename) && f->update == update) {
			return f->buffer;
		}
	}

	f = xsg_new(file_buffer_t, 1);

	f->filename = xsg_strdup(filename);
	f->update = update;
	f->buffer = xsg_buffer_new();

	file_buffer_list = xsg_list_append(file_buffer_list, (void *) f);

	return f->buffer;
}

/******************************************************************************/

static void
update_file_buffer(file_buffer_t *f)
{
	int fd;
	int e;

	if (!xsg_file_test(f->filename, XSG_FILE_TEST_IS_REGULAR)) {
		xsg_message("Not a regular file: %s", f->filename);
		return;
	}

	fd = open(f->filename, O_RDONLY | O_NONBLOCK);

	if (fd == -1) {
		xsg_message("cannot open file %s: %s", f->filename,
				strerror(errno));
		return;
	}

	while (TRUE) {
		char buf[4096];
		ssize_t n;

		n = read(fd, buf, sizeof(buf));

		if (n == 0) {
			break;
		}

		if (n == -1 && errno == EINTR) {
			continue;
		}

		if (n == -1 && errno == EAGAIN) {
			xsg_message("cannot read from file %s: %s", f->filename,
					strerror(errno));
			break;
		}

		if (n == -1) {
			xsg_message("cannot read from file %s: %s", f->filename,
					strerror(errno));
			break;
		}

		xsg_buffer_add(f->buffer, buf, n);
	}

	do {
		e = close(fd);
	} while (e == -1 && errno == EINTR);

	xsg_buffer_clear(f->buffer);
}

static void
update_files(uint64_t tick)
{
	xsg_list_t *l;

	for (l = file_buffer_list; l; l = l->next) {
		file_buffer_t *f = l->data;

		if (tick % f->update == 0) {
			update_file_buffer(f);
		}
	}
}

/******************************************************************************/

static void
parse_file(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	char *filename;
	xsg_buffer_t *buffer;

	filename = xsg_conf_read_string();

	buffer = find_file_buffer(filename, update);

	xsg_buffer_parse(buffer, NULL, num, str, arg);

	xsg_free(filename);

	xsg_main_add_update_func(update_files);
}

static const char *
help_file(void)
{
	static xsg_string_t *string = NULL;

	if (string != NULL) {
		return string->str;
	}

	string = xsg_string_new(NULL);

	xsg_buffer_help(string, XSG_MODULE_NAME, "<filename>");

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_file, help_file, "read regular files");

