/* tail.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
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

typedef struct _tail_t {
	char *filename;
	xsg_buffer_t *buffer;
	dev_t dev;
	ino_t ino;
	off_t size;
	time_t mtime;
} tail_t;

/******************************************************************************/

static xsg_list_t *tail_list = NULL;

/******************************************************************************/

static xsg_buffer_t *find_buffer(const char *filename) {
	xsg_list_t *l;
	tail_t *t;
	int fd;

	for (l = tail_list; l; l = l->next) {
		t = l->data;

		if (strcmp(t->filename, filename) == 0)
			return t->buffer;
	}

	t = xsg_new(tail_t, 1);
	t->filename = xsg_strdup(filename);
	t->buffer = xsg_buffer_new();

	t->dev = 0;
	t->ino = 0;
	t->size = 0;
	t->mtime = 0;

	if ((fd = open(t->filename, O_RDONLY | O_NONBLOCK)) > 0) {
		struct stat stats;

		if ((fstat(fd, &stats)) == 0) {
			t->dev = stats.st_dev;
			t->ino = stats.st_ino;
			t->size = 0;
			t->mtime = stats.st_mtime;
		}
		close(fd);
	}

	tail_list = xsg_list_append(tail_list, (void *) t);

	return t->buffer;
}

/******************************************************************************/

static void update_buffer(tail_t *t) {
	struct stat stats;
	char buf[4096];
	int fd;

	fd = open(t->filename, O_RDONLY | O_NONBLOCK);

	if (fd < 0) {
		xsg_warning("Cannot open file: \"%s\": %s", t->filename, strerror(errno));
		xsg_buffer_clear(t->buffer);
		return;
	}

	if (fstat(fd, &stats) != 0) {
		xsg_warning("Cannot stat file: \"%s\": %s", t->filename, strerror(errno));
		xsg_buffer_clear(t->buffer);
		close(fd);
		return;
	}

	if (!S_ISREG(stats.st_mode)) {
		xsg_warning("Not a regualr file: \"%s\"", t->filename);
		xsg_buffer_clear(t->buffer);
		close(fd);
		return;
	}

	if (t->dev != stats.st_dev) {
		xsg_message("ID of device containing file changed: \"%s\"", t->filename);
		xsg_buffer_clear(t->buffer);
		t->dev = stats.st_dev;
		t->ino = stats.st_ino;
		t->size = 0;
	} else if (t->ino != stats.st_ino) {
		xsg_message("Inode number changed: \"%s\"", t->filename);
		xsg_buffer_clear(t->buffer);
		t->ino = stats.st_ino;
		t->size = 0;
	} else if (t->mtime != stats.st_mtime && t->size == stats.st_size) {
		xsg_message("Time of last modification changed, but total size not: \"%s\"", t->filename);
		xsg_buffer_clear(t->buffer);
		t->size = 0;
	} else if (t->mtime != stats.st_mtime && t->size > stats.st_size) {
		xsg_message("File truncated: \"%s\"", t->filename);
		xsg_buffer_clear(t->buffer);
		t->size = 0;
	}

	t->mtime = stats.st_mtime;

	if (t->size == stats.st_size) {
		close(fd);
		return;
	}

	if (lseek(fd, t->size, SEEK_SET) < 0) {
		xsg_warning("Cannot seek file: \"%s\": %s", t->filename, strerror(errno));
		close(fd);
		return;
	}

	while (TRUE) {
		ssize_t n = read(fd, buf, sizeof(buf));

		if (n < 0 && errno == EINTR) {
			continue;
		} else if (n < 0) {
			xsg_warning("Cannot read from file \"%s\": %s", t->filename, strerror(errno));
			break;
		} else if (n > 0) {
			xsg_buffer_add(t->buffer, buf, n);
			t->size += n;
			continue;
		} else {
			// EOF
			break;
		}
	}

	close(fd);
}

static void update_tails(uint64_t tick) {
	xsg_list_t *l;

	for (l = tail_list; l; l = l->next)
		update_buffer((tail_t *) l->data);
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	char *filename;
	xsg_buffer_t *buffer;

	if (n > 1)
		xsg_conf_error("Past values not supported");

	filename = xsg_conf_read_string();

	buffer = find_buffer(filename);

	xsg_free(filename);

	xsg_buffer_parse(buffer, NULL, num, str, arg);

	xsg_main_add_update_func(update_tails);
}

static const char *help(void) {
	static xsg_string_t *string = NULL;

	if (string == NULL)
		string = xsg_string_new(NULL);
	else
		string = xsg_string_truncate(string, 0);

	xsg_buffer_help(string, xsg_module.name, "<filename>");

	return string->str;
}

xsg_module_t xsg_module = {
	parse, help, "read files like `tail -F`"
};

