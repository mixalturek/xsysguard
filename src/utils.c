/* utils.c
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

/* most stuff copied from glib */

#include <xsysguard.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/******************************************************************************
 *
 * gmem.c
 *
 ******************************************************************************/

void *xsg_malloc(size_t size) {
	if (size) {
		void *mem;

		xsg_debug("malloc %lu bytes", size);

		mem = malloc(size);
		if (mem)
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_malloc0(size_t size) {
	if (size) {
		void *mem;

		xsg_debug("malloc0 %lu bytes", size);

		mem = calloc(1, size);
		if (mem)
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_realloc(void *mem, size_t size) {
	if (size) {

		xsg_debug("realloc %lu bytes", size);

		if (!mem)
			mem = malloc(size);
		else
			mem = realloc(mem, size);

		if (mem)
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	if (mem)
		free(mem);

	return NULL;
}

void xsg_free(void *mem) {
	free(mem);
}

/******************************************************************************
 *
 * glist.c
 *
 ******************************************************************************/

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

/******************************************************************************
 *
 * gstring.c
 *
 ******************************************************************************/

#define MY_MAXSIZE ((size_t)-1)

static size_t nearest_power(size_t base, size_t num) {
	if (num > MY_MAXSIZE / 2) {
		return MY_MAXSIZE;
	} else {
		size_t n = base;

		while (n < num)
			n <<= 1;

		return n;
	}
}

static void xsg_string_maybe_expand(xsg_string_t *string, size_t len) {
	if (string->len + len >= string->allocated_len) {
		string->allocated_len = nearest_power(1, string->len + len + 1);
		string->str = xsg_realloc(string->str, string->allocated_len);
	}
}

xsg_string_t *xsg_string_new(const char *init) {
	xsg_string_t *string;

	if (init == NULL || *init == '\0') {
		string = xsg_string_sized_new(2);
	} else {
		int len;

		len = strlen(init);
		string = xsg_string_sized_new(len + 2);
		xsg_string_append_len(string, init, len);
	}
	return string;
}

xsg_string_t *xsg_string_sized_new(size_t dfl_size) {
	xsg_string_t *string = xsg_new0(xsg_string_t, 1);

	string->allocated_len = 0;
	string->len = 0;
	string->str = NULL;

	xsg_string_maybe_expand(string, MAX(dfl_size, 2));
	string->str[0] = 0;

	return string;
}

xsg_string_t *xsg_string_assign(xsg_string_t *string, const char *rval) {
	if (string == NULL)
		return NULL;
	if (rval == NULL)
		return string;

	if (string->str != rval) {
		xsg_string_truncate(string, 0);
		xsg_string_append(string, rval);
	}

	return string;
}

xsg_string_t *xsg_string_truncate(xsg_string_t *string, size_t len) {
	if (string == NULL)
		return NULL;

	string->len = MIN(len, string->len);
	string->str[string->len] = 0;

	return string;
}

xsg_string_t *xsg_string_set_size(xsg_string_t *string, ssize_t len) {
	if (string == NULL)
		return NULL;

	if (len >= string->allocated_len)
		xsg_string_maybe_expand(string, len - string->len);

	string->len = len;
	string->str[len] = 0;

	return string;
}

xsg_string_t *xsg_string_append(xsg_string_t *string, const char *val) {
	if (string == NULL)
		return NULL;
	if (val == NULL)
		return string;

	return xsg_string_insert_len(string, -1, val, -1);
}

xsg_string_t *xsg_string_append_len(xsg_string_t *string, const char *val, ssize_t len) {
	if (string == NULL)
		return NULL;
	if (val == NULL)
		return string;
	return xsg_string_insert_len(string, -1, val, len);
}

xsg_string_t *xsg_string_insert_len(xsg_string_t *string, ssize_t pos, const char *val, ssize_t len) {
	if (string == NULL)
		return NULL;
	if (val == NULL)
		return string;
	if (len < 0)
		len = strlen(val);
	if (pos < 0)
		pos = string->len;
	else if (pos > string->len)
		return string;

	if (val >= string->str && val <= string->str + string->len) {
		size_t offset = val - string->str;
		size_t precount = 0;

		xsg_string_maybe_expand(string, len);
		val = string->str + offset;

		if (pos < string->len)
			  memmove(string->str + pos + len, string->str + pos, string->len - pos);

		if (offset < pos) {
			precount = MIN(len, pos - offset);
			memcpy(string->str + pos, val, precount);
		}

		if (len > precount)
			memcpy(string->str + pos + precount, val + precount + len, len - precount);
	} else {
		xsg_string_maybe_expand(string, len);

		if (pos < string->len)
			memmove(string->str + pos + len, string->str + pos, string->len - pos);

		if (len == 1)
			string->str[pos] = *val;
		else
			memcpy(string->str + pos, val, len);
	}

	string->len += len;

	string->str[string->len] = 0;

	return string;
}

static void xsg_string_append_printf_internal(xsg_string_t *string, const char *fmt, va_list args) {
	char *buffer;
	int length;

	length = vasprintf(&buffer, fmt, args);
	xsg_string_append_len(string, buffer, length);
	xsg_free(buffer);
}

void xsg_string_printf(xsg_string_t *string, const char *format, ...) {
	va_list args;

	xsg_string_truncate(string, 0);

	va_start(args, format);
	xsg_string_append_printf_internal(string, format, args);
	va_end(args);
}

char *xsg_string_free(xsg_string_t *string, bool free_segment) {
	char *segment;

	if (string == NULL)
		return NULL;

	if (free_segment) {
		xsg_free(string->str);
		segment = NULL;
	} else {
		segment = string->str;
	}

	xsg_free(string);

	return segment;
}


