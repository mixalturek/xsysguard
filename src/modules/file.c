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

typedef struct _file_buffer_t {
	char *filename;
	uint64_t update;
	xsg_buffer_t *buffer;
} file_buffer_t;

typedef struct _file_test_t {
	char *filename;
	uint64_t update;
	xsg_file_test_t test;
	double value;
} file_test_t;

/******************************************************************************/

static xsg_list_t *file_buffer_list = NULL;
static xsg_list_t *file_test_list = NULL;

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

static void *
add_file_test(const char *filename, uint64_t update, xsg_file_test_t test)
{
	xsg_list_t *l;
	file_test_t *f;

	for (l = file_test_list; l; l = l->next) {
		f = l->data;

		if (!strcmp(f->filename, filename) && f->test == test) {
			if ((update % f->update) == 0) {
				return (void *) f;
			}
			if ((f->update % update) == 0) {
				f->update = update;
				return (void *) f;
			}
		}
	}

	f = xsg_new(file_test_t, 1);

	f->filename = xsg_strdup(filename);
	f->update = update;
	f->test = test;
	f->value = DNAN;

	file_test_list = xsg_list_append(file_test_list, (void *) f);

	return (void *) f;
}

/******************************************************************************/

static void
update_file_buffer(file_buffer_t *f)
{
	int fd;
	int e;
#if 0
	if (!xsg_file_test(f->filename, XSG_FILE_TEST_IS_REGULAR)) {
		xsg_message("Not a regular file: %s", f->filename);
		return;
	}
#endif
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
update_file_test(file_test_t *f)
{
	f->value = xsg_file_test(f->filename, f->test) ? 1.0 : 0.0;
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

	for (l = file_test_list; l; l = l->next) {
		file_test_t *f = l->data;

		if (tick % f->update == 0) {
			update_file_test(f);
		}
	}
}

/******************************************************************************/

static double
get_file_test(void *arg)
{
	file_test_t *f = (file_test_t *) arg;
	return f->value;
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

	filename = xsg_conf_read_string();

	if (xsg_conf_find_command("exists")) {
		*arg = add_file_test(filename, update, XSG_FILE_TEST_EXISTS);
		*num = get_file_test;
	} else if (xsg_conf_find_command("is_regular")) {
		*arg = add_file_test(filename, update, XSG_FILE_TEST_IS_REGULAR);
		*num = get_file_test;
	} else if (xsg_conf_find_command("is_symlink")) {
		*arg = add_file_test(filename, update, XSG_FILE_TEST_IS_SYMLINK);
		*num = get_file_test;
	} else if (xsg_conf_find_command("is_dir")) {
		*arg = add_file_test(filename, update, XSG_FILE_TEST_IS_DIR);
		*num = get_file_test;
	} else if (xsg_conf_find_command("is_executable")) {
		*arg = add_file_test(filename, update, XSG_FILE_TEST_IS_EXECUTABLE);
		*num = get_file_test;
	} else {
		xsg_buffer_t *buffer;

		buffer = find_file_buffer(filename, update);
		xsg_buffer_parse(buffer, NULL, num, str, arg);
	}

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

	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME,
			"<filename>", "exists");
	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME,
			"<filename>", "is_regular");
	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME,
			"<filename>", "is_symlink");
	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME,
			"<filename>", "is_dir");
	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME,
			"<filename>", "is_executable");

	xsg_buffer_help(string, XSG_MODULE_NAME, "<filename>");

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_file, help_file, "read regular files");

