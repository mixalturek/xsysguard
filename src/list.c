/* list.c
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

/******************************************************************************/

xsg_list_t *xsg_list_last(xsg_list_t *list) {
	if (list)
		while (list->next)
			list = list->next;
	return list;
}

void *xsg_list_nth_data(xsg_list_t *list, unsigned int n) {
	while ((n-- > 0) && list)
		list = list->next;

	return list ? list->data : NULL;
}

unsigned int xsg_list_length(xsg_list_t *list) {
	unsigned int length;

	length = 0;
	while (list) {
		length++;
		list = list->next;
	}

	return length;
}

xsg_list_t *xsg_list_append(xsg_list_t *list, void *data) {
	xsg_list_t *new_list;
	xsg_list_t *last;

	new_list = xsg_new0(xsg_list_t, 1);
	new_list->data = data;
	new_list->next = NULL;

	if (list) {
		last = xsg_list_last(list);
		last->next = new_list;
		new_list->prev = last;
		return list;
	} else {
		new_list->prev = NULL;
		return new_list;
	}
}

xsg_list_t *xsg_list_prepend(xsg_list_t *list, void *data) {
	xsg_list_t *new_list;

	new_list = xsg_new0(xsg_list_t, 1);
	new_list->data = data;
	new_list->next = list;

	if (list) {
		new_list->prev = list->prev;
		if (list->prev)
			list->prev->next = new_list;
		list->prev = new_list;
	} else {
		new_list->prev = NULL;
	}

	return new_list;
}

void xsg_list_free(xsg_list_t *list) {
	xsg_list_t *next;

	while (list != NULL) {
		next = list->next;
		xsg_free(list);
		list = next;
	}
}


