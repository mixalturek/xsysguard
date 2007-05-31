/* printf.c
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
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "printf.h"
#include "var.h"

/******************************************************************************/

typedef struct _var_t {
	uint32_t var_id;
	enum {
		VAR_INT     = 1,
		VAR_UINT    = 2,
		VAR_DOUBLE  = 3,
		VAR_CHAR    = 4,
		VAR_STRING  = 5,
		VAR_POINTER = 6,
	} type;
	double num;
	xsg_string_t *str;
	char *format;
} var_t;

typedef struct _printf_t {
	char *begin;
	xsg_list_t *var_list;
	xsg_list_t *next_var;
	xsg_string_t *buffer;
} printf_t;

/******************************************************************************/

static uint32_t printf_count = 0;
static xsg_list_t *printf_list = NULL;
static printf_t **printf_array = NULL;

/******************************************************************************/

static printf_t *get_printf(uint32_t printf_id) {
	if (unlikely(printf_array == NULL)) {
		printf_t *p;

		xsg_debug("printf_array is NULL, using printf_list...");
		p = xsg_list_nth_data(printf_list, printf_id);
		if (p == NULL)
			xsg_error("invalid printf_id: %"PRIu32, printf_id);
		else
			return p;
	}

	if (unlikely(printf_id >= printf_count))
		xsg_error("invalid printf_id: %"PRIu32, printf_id);

	return printf_array[printf_id];
}

static void build_printf_array(void) {
	xsg_list_t *l;
	uint32_t printf_id = 0;

	printf_array = xsg_new0(printf_t *, printf_count);

	for (l = printf_list; l; l = l->next) {
		printf_array[printf_id] = l->data;
		printf_id++;
	}
}

/******************************************************************************/

void xsg_printf_init(void) {
	build_printf_array();
}

uint32_t xsg_printf_new(const char *format) {
	printf_t *p;
	xsg_string_t *buf;
	const char *f;
	int type = 0;

	if (unlikely(format == NULL))
		xsg_error("printf_new: format is null");

	p = xsg_new0(printf_t, 1);

	p->begin = NULL;
	p->var_list = NULL;
	p->next_var = NULL;
	p->buffer = xsg_string_new(NULL);

	buf = xsg_string_new(NULL);

	f = format;

	while (*f != '\0') {
		if (f[0] != '%') {
			buf = xsg_string_append_len(buf, f, 1);
			f++;
		} else if (f[0] == '%' && f[1] == '%') {
			buf = xsg_string_append_len(buf, f, 2);
			f += 2;
		} else {
			// found % -> flush buf
			if (p->begin == NULL) {
				p->begin = xsg_strdup(buf->str);
				xsg_debug("printf_new: begin: \"%s\"", buf->str);
			} else {
				var_t *var = xsg_new0(var_t, 1);
				var->var_id = 0;
				var->type = type;
				var->num = DNAN;
				var->str = xsg_string_new(NULL);
				var->format = xsg_strdup(buf->str);

				p->var_list = xsg_list_append(p->var_list, var);
				xsg_debug("printf_new: format: \"%s\"", buf->str);
			}
			buf = xsg_string_truncate(buf, 0);
			// The character %
			buf = xsg_string_append_len(buf, f, 1);
			f++;
			// The flag character
			while (*f == '#' || *f == '0' || *f == '-' || *f == ' ' || *f == '+') {
				buf = xsg_string_append_len(buf, f, 1);
				f++;
			}
			if (*f == '\0')
				xsg_error("printf_new: cannot parse format: conversion specifier expected after flag character");
			// The field width
			if (isdigit(*f) || *f == '*') {
				buf = xsg_string_append_len(buf, f, 1);
				f++;
				while (isdigit(*f)) {
					buf = xsg_string_append_len(buf, f, 1);
					f++;
				}
			}
			if (*f == '\0')
				xsg_error("printf_new: cannot parse format: conversion specifier expected after field width");
			// The precision
			if (*f == '.') {
				buf = xsg_string_append_len(buf, f, 1);
				f++;
				while (isdigit(*f)) {
					buf = xsg_string_append_len(buf, f, 1);
					f++;
				}
			}
			if (*f == '\0')
				xsg_error("printf_new: cannot parse format: conversion specifier expected after precision");
			// The length modifier
			if (f[0] == 'h' && f[1] == 'h')
				f += 2;
			else if (f[0] == 'l' && f[1] == 'l')
				f += 2;
			else if (*f == 'h' || *f == 'l' || *f == 'L' || *f == 'q' || *f == 'j' || *f == 'z' || *f == 't')
				f++;
			// The conversion specifier
			switch (*f) {
				case 'd':
					type = VAR_INT;
					buf = xsg_string_append(buf, PRId64);
					break;
				case 'i':
					type = VAR_INT;
					buf = xsg_string_append(buf, PRIi64);
					break;
				case 'o':
					type = VAR_UINT;
					buf = xsg_string_append(buf, PRIo64);
					break;
				case 'u':
					type = VAR_UINT;
					buf = xsg_string_append(buf, PRIu64);
					break;
				case 'x':
					type = VAR_UINT;
					buf = xsg_string_append(buf, PRIx64);
					break;
				case 'X':
					type = VAR_UINT;
					buf = xsg_string_append(buf, PRIX64);
					break;
				case 'e':
				case 'E':
				case 'f':
				case 'F':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					type = VAR_DOUBLE;
					buf = xsg_string_append_len(buf, f, 1);
					break;
				case 'c':
					type = VAR_CHAR;
					buf = xsg_string_append_len(buf, f, 1);
					break;
				case 's':
					type = VAR_STRING;
					buf = xsg_string_append_len(buf, f, 1);
					break;
				case 'p':
					type = VAR_POINTER;
					buf = xsg_string_append_len(buf, f, 1);
					break;
				default:
					xsg_error("printf_new: cannot parse format: conversion specifier expected: d,i,o,u,x,X,e,E,f,F,g,G,a,A,c,s,p");
			}
			f++;
		}
	}
	// flush buf
	if (p->begin == NULL) {
		p->begin = xsg_strdup(buf->str);
		xsg_debug("printf_new: begin: \"%s\"", buf->str);
	} else {
		var_t *var = xsg_new0(var_t, 1);
		var->var_id = 0;
		var->type = type;
		var->num = DNAN;
		var->str = xsg_string_new(NULL);
		var->format = xsg_strdup(buf->str);

		p->var_list = xsg_list_append(p->var_list, var);
		xsg_debug("printf_new: format: \"%s\"", buf->str);
	}

	xsg_string_free(buf, TRUE);

	p->next_var = p->var_list;

	printf_list = xsg_list_append(printf_list, p);

	return printf_count++;
}

void xsg_printf_add_var(uint32_t printf_id, uint32_t var_id) {
	printf_t *p;
	var_t *var;

	p = get_printf(printf_id);

	if (p->next_var == NULL)
		xsg_conf_error("no more variables");

	var = p->next_var->data;

	var->var_id = var_id;

	p->next_var = p->next_var->next;
}

const char *xsg_printf(uint32_t printf_id, uint32_t var_id) {
	xsg_list_t *l;
	printf_t *p;

	p = get_printf(printf_id);

	p->buffer = xsg_string_assign(p->buffer, p->begin);

	for (l = p->var_list; l; l = l->next) {
		var_t *var = l->data;

		if (var->type == VAR_INT) {
			// d, i
			if ((var_id == 0xffffffff) || (var_id == var->var_id)) {
				var->num = xsg_var_get_num(var->var_id);
				if (isnan(var->num) || isinf(var->num))
					var->num = 0.0;
			}
			xsg_string_append_printf(p->buffer, var->format, (int64_t) var->num);
		} else if (var->type == VAR_UINT) {
			// o, u, x, X
			if ((var_id == 0xffffffff) || (var_id == var->var_id)) {
				var->num = xsg_var_get_num(var->var_id);
				if (isnan(var->num) || isinf(var->num))
					var->num = 0.0;
			}
			xsg_string_append_printf(p->buffer, var->format, (uint64_t) var->num);
		} else if (var->type == VAR_DOUBLE) {
			// e, E, f, F, g, G, a, A
			if ((var_id == 0xffffffff) || (var_id == var->var_id))
				var->num = xsg_var_get_num(var->var_id);
			xsg_string_append_printf(p->buffer, var->format, var->num);
		} else if (var->type == VAR_CHAR) {
			// c
			if ((var_id == 0xffffffff) || (var_id == var->var_id)) {
				var->num = xsg_var_get_num(var->var_id);
				if (isnan(var->num) || isinf(var->num))
					var->num = (double) ' ';
			}
			xsg_string_append_printf(p->buffer, var->format, (int) var->num);
		} else if (var->type == VAR_STRING) {
			// s
			if ((var_id == 0xffffffff) || (var_id == var->var_id))
				var->str = xsg_string_assign(var->str, xsg_var_get_str(var->var_id));
			xsg_string_append_printf(p->buffer, var->format, var->str->str);
		} else if (var->type == VAR_POINTER) {
			// p
			if ((var_id == 0xffffffff) || (var_id == var->var_id)) {
				var->num = xsg_var_get_num(var->var_id);
				if (isnan(var->num) || isinf(var->num))
					var->num = 0.0;
			}
			xsg_string_append_printf(p->buffer, var->format, (unsigned) var->num);
		} else {
			xsg_error("printf: unknown var type");
		}
	}

	xsg_debug("printf(%u): \"%s\"", printf_id, p->buffer->str);

	return p->buffer->str;
}


