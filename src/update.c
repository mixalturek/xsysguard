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
#include "mem.h"

/******************************************************************************/

typedef struct _rect_t {
	int xoffset;
	int yoffset;
	int width;
	int height;
} rect_t;

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
			xsg_mem_free(rect);
			updates = xsg_list_delete_link(updates, l);
			x = MIN(x1_1, x1_2);
			y = MIN(y1_1, y1_2);
			w = MAX(x2_1, x2_2) - x;
			h = MAX(y2_1, y2_2) - y;
			return xsg_update_append_rect(updates, x, y, w, h);
		}
	}

	rect = xsg_mem_new(rect_t);
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
		xsg_mem_free(l->data);
	}

	xsg_list_free(updates);
}


