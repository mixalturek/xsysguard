/* update.c
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

#include "update.h"

/******************************************************************************/

typedef struct _rect_t {
	int xoffset;
	int yoffset;
	int width;
	int height;
} rect_t;

/******************************************************************************/

typedef unsigned bitmask_t;

typedef struct _mem_t {
	rect_t *rects;
	bitmask_t free_rects;
} mem_t;

/******************************************************************************/

static mem_t *mem = NULL;
static unsigned mem_len = 0;

/******************************************************************************/

static rect_t *new_rect(void) {
	unsigned i;
	mem_t *m;

	for (i = 0; i < mem_len; i++) {
		m = mem + i;
		if (m->free_rects != 0) {
			unsigned bit;

			for (bit = 0; bit < sizeof(bitmask_t) * 8; bit++) {
				if (m->free_rects & (1 << bit)) {
					m->free_rects &= ~(1 << bit);
					return m->rects + bit;
				}
			}
		}
	}

	mem_len++;
	mem = xsg_realloc(mem, sizeof(mem_t) * mem_len);

	m = mem + mem_len - 1;

	m->rects = xsg_new(rect_t, sizeof(bitmask_t) * 8);
	m->free_rects = (bitmask_t)-1;

	m->free_rects &= ~1;
	return m->rects;
}

static void free_rect(rect_t *rect) {
	unsigned i;
	mem_t *m;

	for (i = 0; i < mem_len; i++) {
		m = mem + i;
		if ((m->rects <= rect) && (rect <= (m->rects + sizeof(bitmask_t) * 8))) {
			m->free_rects |= (1 << (rect - m->rects));
			return;
		}
	}

	xsg_error("free_rect: cannot free rect");
}

/******************************************************************************/

xsg_list_t *xsg_update_append_rect(xsg_list_t *updates, int x, int y, int w, int h) {
	int x1_1, x2_1, y1_1, y2_1;
	xsg_list_t *l;
	rect_t *rect;

	if (w < 1 || h < 1)
		return updates;

	x1_1 = x;
	x2_1 = x + w;
	y1_1 = y;
	y2_1 = y + h;

	for (l = updates; l; l = l->next) {
		int x1_2, x2_2, y1_2, y2_2;
		bool x_overlap, y_overlap;

		rect = l->data;

		x1_2 = rect->xoffset;
		x2_2 = rect->xoffset + rect->width;
		y1_2 = rect->yoffset;
		y2_2 = rect->yoffset + rect->height;

		x_overlap = !((x2_2 <= x1_1) || (x2_1 <= x1_2));
		y_overlap = !((y2_2 <= y1_1) || (y2_1 <= y1_2));

		if (x_overlap && y_overlap) {
			free_rect(rect);
			updates = xsg_list_delete_link(updates, l);
			x = MIN(x1_1, x1_2);
			y = MIN(y1_1, y1_2);
			w = MAX(x2_1, x2_2) - x;
			h = MAX(y2_1, y2_2) - y;
			return xsg_update_append_rect(updates, x, y, w, h);
		}
	}

	rect = new_rect();
	rect->xoffset = x;
	rect->yoffset = y;
	rect->width = w;
	rect->height = h;

	updates = xsg_list_prepend(updates, rect);

	return updates;
}

void xsg_update_get_coordinates(xsg_list_t *updates, int *x, int *y, int *w, int *h) {
	rect_t *rect;

	if (unlikely(updates == NULL))
		return;

	rect = updates->data;

	if (unlikely(rect == NULL))
		return;

	if (likely(x != NULL))
		*x = rect->xoffset;
	if (likely(y != NULL))
		*y = rect->yoffset;
	if (likely(w != NULL))
		*w = rect->width;
	if (likely(h != NULL))
		*h = rect->height;
}

void xsg_update_free(xsg_list_t *updates) {
	xsg_list_t *l;

	for (l = updates; l; l = l->next) {
		free_rect(l->data);
	}

	xsg_list_free(updates);
}


