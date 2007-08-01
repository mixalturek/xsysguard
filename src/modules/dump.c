/* dump.c
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

typedef struct _dump_file_header_t {
	uint32_t last_tv_sec;
	uint32_t last_tv_usec;
	uint32_t diff_tv_sec;
	uint32_t diff_tv_usec;
} dump_file_header_t;

/******************************************************************************/

typedef struct _dump_file_t {
	char *name;
	dump_file_header_t *header;
	uint32_t data_count;
	double *data;
} dump_file_t;

/******************************************************************************/

typedef struct _dump_t {
	dump_file_t *dump_file;
	uint32_t back;
} dump_t;

/******************************************************************************/

static double get_dump(void *arg) {
	dump_t *dump = (dump_t *) arg;
	dump_file_t *dump_file = dump->dump_file;
	struct timeval now_tv;
	uint64_t now_ms, last_ms, diff_ms, search_ms;
	double d;

	xsg_gettimeofday(&now_tv, NULL);

	now_ms = (uint64_t) now_tv.tv_sec * (uint64_t) 1000 + (uint64_t) now_tv.tv_usec / (uint64_t) 1000;
	last_ms = (uint64_t) dump_file->header->last_tv_sec * (uint64_t) 1000 + (uint64_t) dump_file->header->last_tv_usec / (uint64_t) 1000;
	diff_ms = (uint64_t) dump_file->header->diff_tv_sec * (uint64_t) 1000 + (uint64_t) dump_file->header->diff_tv_usec / (uint64_t) 1000;

	if (diff_ms * (uint64_t) dump->back + last_ms < now_ms) {
		d = DNAN;
	} else {
		search_ms = diff_ms * (uint64_t) dump->back + last_ms - now_ms;

		if (dump->back < (uint32_t) (search_ms / diff_ms))
			d = DNAN;
		else
			d = dump_file->data[search_ms / diff_ms];
	}

	xsg_debug("%s: get_dump(%"PRIu32"): %f", dump_file->name, dump->back, d);

	return d;
}

static double get_nan(void *arg) {
	return DNAN;
}

/******************************************************************************/

dump_file_t *read_dump_file(char *name, char *filename) {
	dump_file_t *dump_file;
	struct stat stat_buf;
	char *buffer;
	size_t size, bytes_read;
	uint32_t i;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		xsg_warning("Cannot read dump file: %s: %s", filename, strerror(errno));
		return NULL;
	}

	if (fstat(fd, &stat_buf) < 0) {
		xsg_warning("Cannot read dump file: %s: fstat failed", filename);
		return NULL;
	}

	size = stat_buf.st_size;

	if (size <= 0) {
		xsg_warning("Cannot read dump file: %s: null byte size", filename);
		return NULL;
	}

	if (size < sizeof(dump_file_header_t) + sizeof(double) || (size - sizeof(dump_file_header_t)) % sizeof(double) != 0) {
		xsg_warning("Cannot read dump file: %s: invalid file size", filename);
		return NULL;
	}

	if (!S_ISREG(stat_buf.st_mode)) {
		xsg_warning("Cannot read dump file: %s: not a regular file", filename);
		return NULL;
	}

	buffer = xsg_malloc(size);

	bytes_read = 0;
	while (bytes_read < size) {
		ssize_t rc;

		rc = read(fd, buffer + bytes_read, size - bytes_read);

		if (rc < 0) {
			if (errno != EINTR) {
				xsg_warning("Cannot read dump file: %s: %s", filename, strerror(errno));
				xsg_free(buffer);
				return NULL;
			}
		} else if (rc == 0) {
			break;
		} else {
			bytes_read += rc;
		}
	}

	close(fd);

	dump_file = xsg_new(dump_file_t, 1);

	dump_file->name = xsg_strdup(name);
	dump_file->header = (dump_file_header_t *) buffer;
	dump_file->data_count = (size - sizeof(dump_file_header_t)) / sizeof(double);
	dump_file->data = (double *) (buffer + sizeof(dump_file_header_t));

	dump_file->header->last_tv_sec = xsg_uint32_be(dump_file->header->last_tv_sec);
	dump_file->header->last_tv_usec = xsg_uint32_be(dump_file->header->last_tv_usec);
	dump_file->header->diff_tv_sec = xsg_uint32_be(dump_file->header->diff_tv_sec);
	dump_file->header->diff_tv_usec = xsg_uint32_be(dump_file->header->diff_tv_usec);

	for (i = 0; i < dump_file->data_count; i++)
		dump_file->data[i] = xsg_double_be(dump_file->data[i]);

	return dump_file;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	char *name, *filename;
	dump_file_t *dump_file;
	uint32_t i;

	name = xsg_conf_read_string();

	filename = xsg_build_filename(xsg_get_home_dir(), ".xsysguard", "dumps", name, NULL);

	dump_file = read_dump_file(name, filename);

	xsg_free(filename);
	xsg_free(name);

	if (dump_file == NULL) {
		for (i = 0; i < n; i++)
			num[i] = get_nan;
	} else {
		dump_t *dumps;

		dumps = xsg_new(dump_t, n);

		for (i = 0; i < n; i++) {
			dumps[i].dump_file = dump_file;
			dumps[i].back = i;
			arg[i] = (void *) (dumps + i);
			num[i] = get_dump;
		}

		xsg_var_dirty(var, n);
	}
}

char *info(void) {
	return "reads files created with the Dump command";
}

