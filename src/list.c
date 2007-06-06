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

/*
 * This code is based on code from GLIB released under the GNU Lesser
 * General Public License.
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * ftp://ftp.gtk.org/pub/gtk/
 *
 */

#include <xsysguard.h>

/******************************************************************************/

typedef unsigned bitmask_t;

typedef struct _mem_t {
	xsg_list_t *nodes;
	bitmask_t free_nodes;
} mem_t;

/******************************************************************************/

static mem_t *mem = NULL;
static unsigned mem_len = 0;

/******************************************************************************/

static xsg_list_t *new_node(void) {
	unsigned i;
	mem_t *m;

	for (i = 0; i < mem_len; i++) {
		m = mem + i;
		if (m->free_nodes != 0) {
			unsigned bit;

			for (bit = 0; bit < sizeof(bitmask_t) * 8; bit++) {
				if (m->free_nodes & (1 << bit)) {
					m->free_nodes &= ~(1 << bit);
					return m->nodes + bit;
				}
			}
		}
	}

	mem_len++;
	mem = xsg_realloc(mem, sizeof(mem_t) * mem_len);

	m = mem + mem_len - 1;

	m->nodes = xsg_new(xsg_list_t, sizeof(bitmask_t) * 8);
	m->free_nodes = (bitmask_t)-1;

	m->free_nodes &= ~1;
	return m->nodes;
}

static void free_node(xsg_list_t *node) {
	unsigned i;
	mem_t *m;

	for (i = 0; i < mem_len; i++) {
		m = mem + i;
		if ((m->nodes <= node) && (node <= (m->nodes + sizeof(bitmask_t) * 8))) {
			m->free_nodes |= (1 << (node - m->nodes));
			return;
		}
	}

	xsg_error("free_node: cannot free node");
}

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

	new_list = new_node();
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

	new_list = new_node();
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

xsg_list_t *xsg_list_remove(xsg_list_t *list, const void *data) {
	xsg_list_t *tmp;

	tmp = list;
	while (tmp) {
		if (tmp->data != data) {
			tmp = tmp->next;
		} else {
			if (tmp->prev)
				tmp->prev->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = tmp->prev;
			if (list == tmp)
				list = list->next;
			free_node(tmp);
			break;
		}
	}
	return list;
}

void xsg_list_free(xsg_list_t *list) {
	xsg_list_t *next;

	while (list != NULL) {
		next = list->next;
		free_node(list);
		list = next;
	}
}

xsg_list_t *xsg_list_delete_link(xsg_list_t *list, xsg_list_t *link) {
	if (link) {
		if (link->prev)
			link->prev->next = link->next;
		if (link->next)
			link->next->prev = link->prev;

		if (link == list)
			list = list->next;

		free_node(link);
	}

	return list;
}

