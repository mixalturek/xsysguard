/* var.c
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
#include <math.h>

#include "widgets.h"
#include "var.h"

/******************************************************************************/

typedef struct _var_t {
	uint64_t update;
	uint8_t type;
	void *(*func)(void *args);
	void *args;
	GString *buffer;
	uint16_t var_id;
	uint16_t widget_id;
} var_t;

/******************************************************************************/

static GList *var_list = NULL;

/******************************************************************************/

static var_t *get_var(uint16_t var_id) {
	GList *l;
	var_t *var;

	for (l = var_list; l; l = l->next) {
		var = l->data;
		if (var->var_id == var_id)
			return var;
	}

	g_error("Invalid variable id: %u", var_id);

	return NULL;
}

/******************************************************************************/

void xsg_var_set_type(uint16_t var_id, uint8_t type) {
	var_t *var;

	var = get_var(var_id);
	var->type = type;
}

void xsg_var_update(uint16_t var_id) {
	var_t *var;

	var = get_var(var_id);
	xsg_widgets_update(var->widget_id, var_id);
}

/******************************************************************************/

uint16_t xsg_var_add(xsg_var *var, uint64_t update, uint16_t widget_id) {
	static uint16_t var_id = 0;
	var_t *v;

	v = g_new0(var_t, 1);

	v->update = update;
	v->type = var->type;
	v->func = var->func;
	v->args = var->args;
	v->buffer = NULL;
	v->var_id = var_id;
	v->widget_id = widget_id;

	var_list = g_list_append(var_list, v);

	var_id++;

	return g_list_length(var_list) - 1;
}

void xsg_var_init() {
	return;
}

/******************************************************************************/

static double get_int_as_double(void *value) {
	double d;
	int64_t i;

	i = * (int64_t *) value;
	d = (double) i;
	g_message("Conversion(INT->DOUBLE) %" G_GINT64_FORMAT " -> %lg", i, d);
	return d;
}

static double get_string_as_double(void *value) {
	double d;
	char *s;

	s = (char *) value;
	d = g_ascii_strtod(s, NULL);
	g_message("Conversion(STRING->DOUBLE) %s -> %lg", s, d);
	return d;
}

static int64_t get_double_as_int(void *value) {
	int64_t i;
	double d;
	double r;

	d = * (double *) value;
	i = (int64_t) d;
	r = d - (double) i;
	if (r >= 0.5)
		i++;
	g_message("Conversion(DOUBLE->INT) %lg -> %" G_GINT64_FORMAT, d, i);
	return i;
}

static int64_t get_string_as_int(void *value) {
	int64_t i;
	char *s;

	s = (char *) value;
	i = g_ascii_strtoll(s, NULL, 0);
	g_message("Conversion(STRING->INT) %s -> %" G_GINT64_FORMAT, s, i);
	return i;
}

static char *get_double_as_string(void *value, GString *buffer) {
	double d;

	d = * (double *) value;
	g_string_printf(buffer, "%lg", d);
	g_message("Conversion(DOUBLE->STRING) %lg -> %s", d, buffer->str);
	return buffer->str;
}

static char *get_int_as_string(void *value, GString *buffer) {
	int64_t i;

	i = * (int64_t *) value;
	g_string_printf(buffer, "%" G_GINT64_FORMAT, i);
	g_message("Conversion(INT->STRING) %" G_GINT64_FORMAT " -> %s", i, buffer->str);
	return buffer->str;
}

/******************************************************************************/

double xsg_var_as_double(uint16_t var_id) {
	var_t *var;
	void *value;

	var = get_var(var_id);
	value = (var->func)(var->args);

	switch (var->type) {
		case XSG_DOUBLE:
			return * (double *) value;
		case XSG_INT:
			return get_int_as_double(value);
		case XSG_STRING:
			return get_string_as_double(value);
		default:
			return 0.0;
	}
}

int64_t xsg_var_as_int(uint16_t var_id) {
	var_t *var;
	void *value;

	var = get_var(var_id);
	value = (var->func)(var->args);

	switch (var->type) {
		case XSG_INT:
			return * (int64_t *) value;
		case XSG_DOUBLE:
			return get_double_as_int(value);
		case XSG_STRING:
			return get_string_as_int(value);
		default:
			return 0;
	}
}

char *xsg_var_as_string(uint16_t var_id) {
	var_t *var;
	void *value;

	var = get_var(var_id);
	value = (var->func)(var->args);

	if (var->buffer == NULL)
		var->buffer = g_string_new("");

	switch (var->type) {
		case XSG_STRING:
			g_string_assign(var->buffer, (char *) value);
		case XSG_INT:
			get_int_as_string(value, var->buffer);
		case XSG_DOUBLE:
			get_double_as_string(value, var->buffer);
		default:
			;
	}
	return var->buffer->str;
}

/******************************************************************************/

