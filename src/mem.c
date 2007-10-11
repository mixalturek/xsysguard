/* mem.c
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

#include "mem.h"

/******************************************************************************/

typedef struct _mem_t {
	char *data;
	size_t size;
	unsigned bitmask;
} mem_t;

/******************************************************************************/

static mem_t *mem = NULL;
static unsigned mem_len = 0;

/******************************************************************************/

void *xsg_mem_alloc(size_t size) {
	unsigned i;
	mem_t *m;

	if (unlikely(size == 0))
		return NULL;

	if (size % sizeof(void *) != 0)
		size += sizeof(void *) - (size % sizeof(void *));

	for (i = 0; i < mem_len; i++) {
		m = mem + i;
		if (m->size == size && m->bitmask != 0) {
			unsigned bit;

			for (bit = 0; bit < sizeof(unsigned) * 8; bit++) {
				if (m->bitmask & (1 << bit)) {
					m->bitmask &= ~(1 << bit);
					return (void *) (m->data + bit * size);
				}
			}
		}
	}

	mem_len++;

	mem = xsg_realloc(mem, sizeof(mem_t) * mem_len);

	m = mem + mem_len - 1;

	m->data = (char *) xsg_malloc(size * sizeof(unsigned) * 8);
	m->size = size;
	m->bitmask = (unsigned)-1;

	m->bitmask &= ~1;

	return (void *) (m->data);
}

void xsg_mem_free(void *data) {
	char *d = (char *) data;
	unsigned i;

	for (i = 0; i < mem_len; i++) {
		mem_t *m = mem + i;

		if ((d >= m->data) && ((d - m->data) % m->size == 0)) {
			if ((d - m->data) / m->size < sizeof(unsigned) * 8) {
				m->bitmask |= 1 << (d - m->data) / m->size;
				return;
			}
		}
	}

	xsg_error("xsg_mem_free: cannot free %p", data);
}

