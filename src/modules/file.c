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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

/******************************************************************************/

typedef struct _file_t {
	char *filename;
	uint64_t update;
	xsg_buffer_t *buffer;
} file_t;

/******************************************************************************/

static xsg_list_t *file_list = NULL;

/******************************************************************************/

static xsg_buffer_t *
find_buffer(const char *filename, uint64_t update)
{
	xsg_list_t *l;
	file_t *f;

	for (l = file_list; l; l = l->next) {
		f = l->data;

		if (strcmp(f->filename, filename) == 0 && update == f->update) {
			if ((update % f->update) == 0) {
				return f->buffer;
			}
			if ((f->update % update) == 0) {
				f->update = update;
				return f->buffer;
			}
		}
	}

	f = xsg_new(file_t, 1);

	f->filename = xsg_strdup(filename);
	f->update = update;
	f->buffer = xsg_buffer_new();

	file_list = xsg_list_append(file_list, (void *) f);

	return f->buffer;
}

/******************************************************************************/

static void
update_file(file_t *f)
{
	int fd;
	int e;
#if 0
	if (!xsg_file_test(f->filename, XSG_FILE_TEST_IS_REGULAR)) {
		xsg_warning("Not a regular file: %s", f->filename);
		return;
	}
#endif
	fd = open(f->filename, O_RDONLY | O_NONBLOCK);

	if (fd == -1) {
		xsg_warning("cannot open file %s: %s", f->filename,
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
			xsg_warning("cannot read from file %s: %s", f->filename,
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
update_files(uint64_t update)
{
	xsg_list_t *l;

	for (l = file_list; l; l = l->next) {
		file_t *f = l->data;

		if (update % f->update == 0) {
			update_file(f);
		}
	}
}

/******************************************************************************/

static void
parse(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	char *filename;
	xsg_buffer_t *buffer;

	filename = xsg_conf_read_string();

	buffer = find_buffer(filename, update);

	xsg_free(filename);

	xsg_buffer_parse(buffer, NULL, num, str, arg);

	xsg_main_add_update_func(update_files);
}

static const char *
help(void)
{
	static xsg_string_t *string = NULL;

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	xsg_buffer_help(string, xsg_module.name, "<filename>");

	return string->str;
}

xsg_module_t xsg_module = {
	parse, help, "read regular files"
};

