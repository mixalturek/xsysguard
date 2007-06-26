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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dump.h"

/******************************************************************************/

typedef struct _dump_t {
	const char *filename;
	const char *name;
	uint64_t update;
	unsigned count;
	double *values;
	unsigned *index;
} dump_t;

/******************************************************************************/

typedef struct _dump_header_t {
	uint32_t last_tv_sec;
	uint32_t last_tv_usec;
	uint32_t diff_tv_sec;
	uint32_t diff_tv_usec;
} dump_header_t;

/******************************************************************************/

static xsg_list_t *dump_list = NULL;

/******************************************************************************/

void xsg_dump_register(const char *name, uint64_t update, unsigned count, double *values, unsigned *index) {
	dump_t *d;

	d = xsg_new(dump_t, 1);

	d->filename = xsg_build_filename(xsg_get_home_dir(), ".xsysguard", "dumps", name, NULL);
	d->name = name;
	d->update = update;
	d->count = count;
	d->values = values;
	d->index = index;

	dump_list = xsg_list_append(dump_list, d);
}

/******************************************************************************/

static void dump(void) {
	dump_header_t header;
	struct timeval now;
	xsg_list_t *l;

	xsg_gettimeofday(&now, NULL);

	header.last_tv_sec  = xsg_uint32_be(now.tv_sec);
	header.last_tv_usec = xsg_uint32_be(now.tv_usec);

	for (l = dump_list; l; l = l->next) {
		dump_t *d = l->data;;
		int fd;
		unsigned i;
		size_t index, count;
		uint64_t diff;

		diff = d->update * xsg_main_get_interval();

		header.diff_tv_sec = xsg_uint32_be(diff / 1000);
		header.diff_tv_usec = xsg_uint32_be(diff % 1000 * 1000);

		// overwrite existing value array in network byte order
		for (i = 0; i < d->count; i++)
			d->values[i] = xsg_double_be(d->values[i]);

		fd = open(d->filename, O_WRONLY | O_CREAT, 00666);

		if (fd < 0)
			continue;

		if (write(fd, &header, sizeof(dump_header_t)) < 0) {
			close(fd);
			continue;
		}

		index = *d->index + 1;
		count = d->count - index;

		if (count > 0) {
			if (write(fd, d->values + index, count * sizeof(double)) < 0) {
				close(fd);
				continue;
			}
		}

		write(fd, d->values, index * sizeof(double));

		close(fd);
	}
}

void xsg_dump_atexit(void) {
	if (atexit(dump) != 0)
		xsg_warning("Cannot set exit function: dump");
}

