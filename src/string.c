/* string.c
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

#define MY_MAXSIZE ((size_t)-1)

/******************************************************************************/

static size_t
nearest_power(size_t base, size_t num)
{
	if (num > MY_MAXSIZE / 2) {
		return MY_MAXSIZE;
	} else {
		size_t n = base;

		while (n < num) {
			n <<= 1;
		}

		return n;
	}
}

static void
xsg_string_maybe_expand(xsg_string_t *string, size_t len)
{
	if (string->len + len >= string->allocated_len) {
		string->allocated_len = nearest_power(1, string->len + len + 1);
		string->str = xsg_realloc(string->str, string->allocated_len);
	}
}

/******************************************************************************/

xsg_string_t *
xsg_string_new(const char *init)
{
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

xsg_string_t *
xsg_string_sized_new(size_t len)
{
	xsg_string_t *string = xsg_new(xsg_string_t, 1);

	string->allocated_len = 0;
	string->len = 0;
	string->str = NULL;

	xsg_string_maybe_expand(string, MAX(len, 2));
	string->str[0] = 0;

	return string;
}

/******************************************************************************/

void
xsg_string_assign(xsg_string_t *string, const char *val)
{
	if (unlikely(string == NULL)) {
		return;
	}
	if (unlikely(val == NULL)) {
		return;
	}

	if (likely(string->str != val)) {
		xsg_string_truncate(string, 0);
		xsg_string_append(string, val);
	}
}

void
xsg_string_truncate(xsg_string_t *string, size_t len)
{
	if (unlikely(string == NULL)) {
		return;
	}

	string->len = MIN(len, string->len);
	string->str[string->len] = 0;
}

void
xsg_string_set_size(xsg_string_t *string, size_t len)
{
	if (unlikely(string == NULL)) {
		return;
	}

	if (len >= string->allocated_len) {
		xsg_string_maybe_expand(string, len - string->len);
	}

	string->len = len;
	string->str[len] = 0;
}

void
xsg_string_append(xsg_string_t *string, const char *val)
{
	if (unlikely(string == NULL)) {
		return;
	}
	if (unlikely(val == NULL)) {
		return;
	}

	xsg_string_insert_len(string, -1, val, -1);
}

void
xsg_string_append_c(xsg_string_t *string, char c)
{
	if (unlikely(string == NULL)) {
		return;
	}

	xsg_string_insert_c(string, -1, c);
}

void
xsg_string_append_len(xsg_string_t *string, const char *val, ssize_t len)
{
	if (unlikely(string == NULL)) {
		return;
	}
	if (unlikely(val == NULL)) {
		return;
	}

	xsg_string_insert_len(string, -1, val, len);
}

void
xsg_string_insert_len(
	xsg_string_t *string,
	ssize_t pos,
	const char *val,
	ssize_t len
)
{
	if (unlikely(string == NULL)) {
		return;
	}
	if (unlikely(val == NULL)) {
		return;
	}
	if (len < 0) {
		len = strlen(val);
	}
	if (pos < 0) {
		pos = string->len;
	} else if (pos > string->len) {
		return;
	}

	if (val >= string->str && val <= string->str + string->len) {
		size_t offset = val - string->str;
		size_t precount = 0;

		xsg_string_maybe_expand(string, len);
		val = string->str + offset;

		if (pos < string->len) {
			  memmove(string->str + pos + len, string->str + pos,
					  string->len - pos);
		}

		if (offset < pos) {
			precount = MIN(len, pos - offset);
			memcpy(string->str + pos, val, precount);
		}

		if (len > precount) {
			memcpy(string->str + pos + precount,
					val + precount + len, len - precount);
		}
	} else {
		xsg_string_maybe_expand(string, len);

		if (pos < string->len) {
			memmove(string->str + pos + len, string->str + pos,
					string->len - pos);
		}

		if (len == 1) {
			string->str[pos] = *val;
		} else {
			memcpy(string->str + pos, val, len);
		}
	}

	string->len += len;
	string->str[string->len] = 0;
}

void
xsg_string_insert_c(xsg_string_t *string, ssize_t pos, char c)
{
	if (unlikely(string == NULL)) {
		return;
	}
	xsg_string_maybe_expand(string, 1);
	if (pos < 0) {
		pos = string->len;
	} else if (unlikely(pos > string->len)) {
			return;
	}
	if (pos < string->len) {
		memmove(string->str + pos + 1, string->str + pos,
				string->len - pos);
	}
	string->str[pos] = c;
	string->len += 1;
	string->str[string->len] = 0;
}

void
xsg_string_erase(xsg_string_t *string, ssize_t pos, ssize_t len)
{
	if (unlikely(string == NULL)) {
		return;
	}
	if (unlikely(pos < 0)) {
		return;
	}
	if (unlikely(pos > string->len)) {
		return;
	}
	if (len < 0) {
		len = string->len - pos;
	} else {
		if (unlikely(pos + len > string->len)) {
			return;
		}
		if (pos + len < string->len) {
			memmove(string->str + pos, string->str + pos + len,
					string->len - (pos + len));
		}
	}
	string->len -= len;
	string->str[string->len] = 0;
}

/******************************************************************************/

void
xsg_string_up(xsg_string_t *string)
{
	unsigned char *s;
	long n;

	if (unlikely(string == NULL)) {
		return;
	}

	n = string->len;
	s = (unsigned char *) string->str;

	while (n) {
		if (islower(*s)) {
			*s = toupper(*s);
		}
		s++;
		n--;
	}
}

void
xsg_string_down(xsg_string_t *string)
{
	unsigned char *s;
	long n;

	if (unlikely(string == NULL)) {
		return;
	}

	n = string->len;
	s = (unsigned char *) string->str;

	while (n) {
		if (isupper(*s)) {
			*s = tolower(*s);
		}
		s++;
		n--;
	}
}

/******************************************************************************/

static void
xsg_string_append_printf_internal(
	xsg_string_t *string,
	const char *fmt,
	va_list args
)
{
	char *buffer;
	int length;

	length = xsg_vasprintf(&buffer, fmt, args);
	xsg_string_append_len(string, buffer, length);
	xsg_free(buffer);
}

/******************************************************************************/

void
xsg_string_printf(xsg_string_t *string, const char *format, ...)
{
	va_list args;

	xsg_string_truncate(string, 0);

	va_start(args, format);
	xsg_string_append_printf_internal(string, format, args);
	va_end(args);
}

void
xsg_string_append_printf(xsg_string_t *string, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	xsg_string_append_printf_internal(string, format, args);
	va_end(args);
}

/******************************************************************************/

char *
xsg_string_free(xsg_string_t *string, bool free_segment)
{
	char *segment;

	if (unlikely(string == NULL)) {
		return NULL;
	}

	if (free_segment) {
		xsg_free(string->str);
		segment = NULL;
	} else {
		segment = string->str;
	}

	xsg_free(string);

	return segment;
}

