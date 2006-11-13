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
#include "modules.h"
#include "var.h"

/******************************************************************************/

typedef struct _var_t var_t;
typedef struct _val_t val_t;

struct _var_t {
	char op;
	void *(*op_func)(void *var_a, void *var_b, xsg_string_t *buffer);
	uint8_t type;
	void *(*func)(void *args);
	void *args;
	uint16_t widget_id;
	uint16_t var_id;
	val_t *val;
};

struct _val_t {
	uint64_t update;
	uint8_t type;
	xsg_list_t *var_list;
	xsg_string_t *buffer;
	uint16_t widget_id;
	uint16_t val_id;
};

/******************************************************************************/

static xsg_list_t *var_list = NULL;
static xsg_list_t *val_list = NULL;

/******************************************************************************/

static var_t *get_var(uint16_t var_id) {
	xsg_list_t *l;
	var_t *var;

	for (l = var_list; l; l = l->next) {
		var = l->data;
		if (var->var_id == var_id)
			return var;
	}

	g_error("Invalid variable id: %" PRIu16, var_id);

	return NULL;
}

static val_t *get_val(uint16_t val_id) {
	xsg_list_t *l;
	val_t *val;

	for (l = val_list; l; l = l->next) {
		val = l->data;
		if (val->val_id == val_id)
			return val;
	}

	g_error("Invalid value id: %" PRIu16, val_id);

	return NULL;
}

/******************************************************************************/

void xsg_var_init() {
	return;
}

/******************************************************************************/

static void *get_int_as_double(void *value) {
	static double d;
	int64_t i;

	i = * (int64_t *) value;
	d = (double) i;
	g_message("Conversion (INT->DOUBLE) %" PRId64 " -> %g", i, d);
	return (void *) &d;
}

static void *get_string_as_double(void *value) {
	static double d;
	char *s;

	s = (char *) value;
	d = g_ascii_strtod(s, NULL);
	g_message("Conversion (STRING->DOUBLE) %s -> %g", s, d);
	return (void *) &d;
}

static void *get_double_as_int(void *value) {
	static int64_t i;
	double d;
	double r;

	d = * (double *) value;
	i = (int64_t) d;
	r = d - (double) i;
	if (r >= 0.5)
		i++;
	g_message("Conversion (DOUBLE->INT) %g -> %" PRId64, d, i);
	return (void *) &i;
}

static void *get_string_as_int(void *value) {
	static int64_t i;
	char *s;

	s = (char *) value;
	i = g_ascii_strtoll(s, NULL, 0);
	g_message("Conversion (STRING->INT) %s -> %" PRId64, s, i);
	return (void *) &i;
}

static char *get_double_as_string(void *value, xsg_string_t *buffer) {
	double d;

	d = * (double *) value;
	xsg_string_printf(buffer, "%g", d);
	g_message("Conversion (DOUBLE->STRING) %g -> %s", d, buffer->str);
	return buffer->str;
}

static char *get_int_as_string(void *value, xsg_string_t *buffer) {
	int64_t i;

	i = * (int64_t *) value;
	xsg_string_printf(buffer, "%" PRId64, i);
	g_message("Conversion (INT->STRING) %" PRId64 " -> %s", i, buffer->str);
	return buffer->str;
}

/******************************************************************************/

static void *set(void *var_a, void *var_b, xsg_string_t *buffer) {
	return var_b;
}

/******************************************************************************/

static const uint8_t add_int_int_type = XSG_INT;
static void *add_int_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) var_b;

	res = *a + *b;
	return &res;
}

static const uint8_t add_int_double_type = XSG_DOUBLE;
static void *add_int_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_int_as_double(var_a);
	double *b = (double *) var_b;

	res = *a + *b;
	return &res;
}

static const uint8_t add_int_string_type = XSG_INT;
static void *add_int_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a + *b;
	return &res;
}

static const uint8_t add_double_int_type = XSG_DOUBLE;
static void *add_double_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) get_int_as_double(var_b);

	res = *a + *b;
	return &res;
}

static const uint8_t add_double_double_type = XSG_DOUBLE;
static void *add_double_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) var_b;

	res = *a + *b;
	return &res;
}

static const uint8_t add_double_string_type = XSG_DOUBLE;
static void *add_double_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) get_string_as_double(var_b);

	res = *a + *b;
	return &res;
}

static const uint8_t add_string_int_type = XSG_INT;
static void *add_string_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a + *b;
	return &res;
}

static const uint8_t add_string_double_type = XSG_DOUBLE;
static void *add_string_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_string_as_double(var_a);
	double *b = (double *) var_b;

	res = *a + *b;
	return &res;
}

static const uint8_t add_string_string_type = XSG_DOUBLE;
static void *add_string_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_string_as_double(var_a);
	double *b = (double *) get_string_as_double(var_b);

	res = *a + *b;
	return &res;
}

/******************************************************************************/

static const uint8_t mult_int_int_type = XSG_INT;
static void *mult_int_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) var_b;

	res = *a * *b;
	return &res;
}

static const uint8_t mult_int_double_type = XSG_DOUBLE;
static void *mult_int_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_int_as_double(var_a);
	double *b = (double *) var_b;

	res = *a * *b;
	return &res;
}

static const uint8_t mult_int_string_type = XSG_INT;
static void *mult_int_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a * *b;
	return &res;
}

static const uint8_t mult_double_int_type = XSG_DOUBLE;
static void *mult_double_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) get_int_as_double(var_b);

	res = *a * *b;
	return &res;
}

static const uint8_t mult_double_double_type = XSG_DOUBLE;
static void *mult_double_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) var_b;

	res = *a * *b;
	return &res;
}

static const uint8_t mult_double_string_type = XSG_DOUBLE;
static void *mult_double_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) var_a;
	double *b = (double *) get_string_as_double(var_b);

	res = *a * *b;
	return &res;
}

static const uint8_t mult_string_int_type = XSG_INT;
static void *mult_string_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a * *b;
	return &res;
}

static const uint8_t mult_string_double_type = XSG_DOUBLE;
static void *mult_string_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_string_as_double(var_a);
	double *b = (double *) var_b;

	res = *a * *b;
	return &res;
}

static const uint8_t mult_string_string_type = XSG_DOUBLE;
static void *mult_string_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static double res;
	double *a = (double *) get_string_as_double(var_a);
	double *b = (double *) get_string_as_double(var_b);

	res = *a * *b;
	return &res;
}

/******************************************************************************/

static const uint8_t div_int_int_type = XSG_INT;
static void *div_int_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) var_b;

	res = *a / *b;
	return &res;
}

static const uint8_t div_int_double_type = XSG_INT;
static void *div_int_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a / *b;
	return &res;
}

static const uint8_t div_int_string_type = XSG_INT;
static void *div_int_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a / *b;
	return &res;
}

static const uint8_t div_double_int_type = XSG_INT;
static void *div_double_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a / *b;
	return &res;
}

static const uint8_t div_double_double_type = XSG_INT;
static void *div_double_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a / *b;
	return &res;
}

static const uint8_t div_double_string_type = XSG_INT;
static void *div_double_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a / *b;
	return &res;
}

static const uint8_t div_string_int_type = XSG_INT;
static void *div_string_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a / *b;
	return &res;
}

static const uint8_t div_string_double_type = XSG_INT;
static void *div_string_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a / *b;
	return &res;
}

static const uint8_t div_string_string_type = XSG_INT;
static void *div_string_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a / *b;
	return &res;
}

/******************************************************************************/

static const uint8_t mod_int_int_type = XSG_INT;
static void *mod_int_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) var_b;

	res = *a % *b;
	return &res;
}

static const uint8_t mod_int_double_type = XSG_INT;
static void *mod_int_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a % *b;
	return &res;
}

static const uint8_t mod_int_string_type = XSG_INT;
static void *mod_int_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) var_a;
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a % *b;
	return &res;
}

static const uint8_t mod_double_int_type = XSG_INT;
static void *mod_double_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a % *b;
	return &res;
}

static const uint8_t mod_double_double_type = XSG_INT;
static void *mod_double_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a % *b;
	return &res;
}

static const uint8_t mod_double_string_type = XSG_INT;
static void *mod_double_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_double_as_int(var_a);
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a % *b;
	return &res;
}

static const uint8_t mod_string_int_type = XSG_INT;
static void *mod_string_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) var_b;

	res = *a % *b;
	return &res;
}

static const uint8_t mod_string_double_type = XSG_INT;
static void *mod_string_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) get_double_as_int(var_b);

	res = *a % *b;
	return &res;
}

static const uint8_t mod_string_string_type = XSG_INT;
static void *mod_string_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	static int64_t res;
	int64_t *a = (int64_t *) get_string_as_int(var_a);
	int64_t *b = (int64_t *) get_string_as_int(var_b);

	res = *a % *b;
	return &res;
}

/******************************************************************************/

static const uint8_t cat_int_int_type = XSG_STRING;
static void *cat_int_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	get_int_as_string(var_a, buffer);
	get_int_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_int_double_type = XSG_STRING;
static void *cat_int_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	get_int_as_string(var_a, buffer);
	get_double_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_int_string_type = XSG_STRING;
static void *cat_int_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	get_int_as_string(var_a, buffer);
	xsg_string_append(buffer, (char *) var_b);
	return buffer->str;
}

static const uint8_t cat_double_int_type = XSG_STRING;
static void *cat_double_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	get_double_as_string(var_a, buffer);
	get_int_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_double_double_type = XSG_STRING;
static void *cat_double_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	get_double_as_string(var_a, buffer);
	get_double_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_double_string_type = XSG_STRING;
static void *cat_double_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	get_double_as_string(var_a, buffer);
	xsg_string_append(buffer, (char *) var_b);
	return buffer->str;
}

static const uint8_t cat_string_int_type = XSG_STRING;
static void *cat_string_int(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	xsg_string_assign(buffer, var_a);
	get_int_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_string_double_type = XSG_STRING;
static void *cat_string_double(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_t *tmp = xsg_string_new(NULL);

	xsg_string_assign(buffer, var_a);
	get_double_as_string(var_b, tmp);
	xsg_string_append(buffer, tmp->str);
	xsg_string_free(tmp, TRUE);
	return buffer->str;
}

static const uint8_t cat_string_string_type = XSG_STRING;
static void *cat_string_string(void *var_a, void *var_b, xsg_string_t *buffer) {
	xsg_string_append(buffer, (char *) var_b);
	return buffer->str;
}

/******************************************************************************/

static void *calc(val_t *val) {
	xsg_list_t *l;
	var_t *var;
	void *value;
	void *reg = NULL;

	for (l = val->var_list; l; l = l->next) {
		var = l->data;
		value = (var->func)(var->args);

		reg = (var->op_func)(reg, value, val->buffer);
	}
	return reg;
}

/******************************************************************************/

double xsg_var_get_double(uint16_t val_id) {
	double d;
	val_t *val;
	void *value;

	val = get_val(val_id);
	value = calc(val);

	switch (val->type) {
		case XSG_DOUBLE:
			d = *(double *) value;
			break;
		case XSG_INT:
			d = *(double *) get_int_as_double(value);
			break;
		case XSG_STRING:
			d = *(double *) get_string_as_double(value);
			break;
		default:
			d = 0.0;
			break;
	}

	return d;
}

int64_t xsg_var_get_int(uint16_t val_id) {
	int64_t i;
	val_t *val;
	void *value;

	val = get_val(val_id);
	value = calc(val);

	switch (val->type) {
		case XSG_INT:
			i = *(int64_t *) value;
			break;
		case XSG_DOUBLE:
			i = *(int64_t *) get_double_as_int(value);
			break;
		case XSG_STRING:
			i = *(int64_t *) get_string_as_int(value);
			break;
		default:
			i = 0;
			break;
	}

	return i;
}

char *xsg_var_get_string(uint16_t val_id) {
	val_t *val;
	void *value;

	val = get_val(val_id);
	value = calc(val);

	if (val->buffer == NULL)
		val->buffer = xsg_string_new(0);

	switch (val->type) {
		case XSG_STRING:
			xsg_string_assign(val->buffer, (char *) value);
			break;
		case XSG_INT:
			get_int_as_string(value, val->buffer);
			break;
		case XSG_DOUBLE:
			get_double_as_string(value, val->buffer);
			break;
		default:
			break;
	}

	return val->buffer->str;
}

/******************************************************************************/

void update_op_funcs(val_t *val) {
	xsg_list_t *l;
	uint8_t type = 0;

	g_debug("Begin update_op_funcs");

	for (l = val->var_list; l; l = l->next) {
		void *(*op_func)(void*, void*, xsg_string_t*) = NULL;
		var_t *var = l->data;
		char op = var->op;
		uint8_t type_a = type;
		uint8_t type_b = var->type;

		g_debug("Updating op for op=%c type_a=%d type_b=%d", op, type_a, type_b);

		if (type == 0) {
			type = var->type;
			op_func = set;
		} else {
			switch (op) {
			case '+':
				switch (type_a) {
				case XSG_INT:
					switch (type_b) {
					case XSG_INT:
						type = add_int_int_type;
						op_func = add_int_int;
						break;
					case XSG_DOUBLE:
						type = add_int_double_type;
						op_func = add_int_double;
						break;
					case XSG_STRING:
						type = add_int_string_type;
						op_func = add_int_string;
						break;
					default:
						break;
					}
					break;
				case XSG_DOUBLE:
					switch (type_b) {
					case XSG_INT:
						type = add_double_int_type;
						op_func = add_double_int;
						break;
					case XSG_DOUBLE:
						type = add_double_double_type;
						op_func = add_double_double;
						break;
					case XSG_STRING:
						type = add_double_string_type;
						op_func = add_double_string;
						break;
					default:
						break;
					}
					break;
				case XSG_STRING:
					switch (type_b) {
					case XSG_INT:
						type = add_string_int_type;
						op_func = add_string_int;
						break;
					case XSG_DOUBLE:
						type = add_string_double_type;
						op_func = add_string_double;
						break;
					case XSG_STRING:
						type = add_string_string_type;
						op_func = add_string_string;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				break;
			case '*':
				switch (type_a) {
				case XSG_INT:
					switch (type_b) {
					case XSG_INT:
						type = mult_int_int_type;
						op_func = mult_int_int;
						break;
					case XSG_DOUBLE:
						type = mult_int_double_type;
						op_func = mult_int_double;
						break;
					case XSG_STRING:
						type = mult_int_string_type;
						op_func = mult_int_string;
						break;
					default:
						break;
					}
					break;
				case XSG_DOUBLE:
					switch (type_b) {
					case XSG_INT:
						type = mult_double_int_type;
						op_func = mult_double_int;
						break;
					case XSG_DOUBLE:
						type = mult_double_double_type;
						op_func = mult_double_double;
						break;
					case XSG_STRING:
						type = mult_double_string_type;
						op_func = mult_double_string;
						break;
					default:
						break;
					}
					break;
				case XSG_STRING:
					switch (type_b) {
					case XSG_INT:
						type = mult_string_int_type;
						op_func = mult_string_int;
						break;
					case XSG_DOUBLE:
						type = mult_string_double_type;
						op_func = mult_string_double;
						break;
					case XSG_STRING:
						type = mult_string_string_type;
						op_func = mult_string_string;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				break;
			case '/':
				switch (type_a) {
				case XSG_INT:
					switch (type_b) {
					case XSG_INT:
						type = div_int_int_type;
						op_func = div_int_int;
						break;
					case XSG_DOUBLE:
						type = div_int_double_type;
						op_func = div_int_double;
						break;
					case XSG_STRING:
						type = div_int_string_type;
						op_func = div_int_string;
						break;
					default:
						break;
					}
					break;
				case XSG_DOUBLE:
					switch (type_b) {
					case XSG_INT:
						type = div_double_int_type;
						op_func = div_double_int;
						break;
					case XSG_DOUBLE:
						type = div_double_double_type;
						op_func = div_double_double;
						break;
					case XSG_STRING:
						type = div_double_string_type;
						op_func = div_double_string;
						break;
					default:
						break;
					}
					break;
				case XSG_STRING:
					switch (type_b) {
					case XSG_INT:
						type = div_string_int_type;
						op_func = div_string_int;
						break;
					case XSG_DOUBLE:
						type = div_string_double_type;
						op_func = div_string_double;
						break;
					case XSG_STRING:
						type = div_string_string_type;
						op_func = div_string_string;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				break;
			case '%':
				switch (type_a) {
				case XSG_INT:
					switch (type_b) {
					case XSG_INT:
						type = mod_int_int_type;
						op_func = mod_int_int;
						break;
					case XSG_DOUBLE:
						type = mod_int_double_type;
						op_func = mod_int_double;
						break;
					case XSG_STRING:
						type = mod_int_string_type;
						op_func = mod_int_string;
						break;
					default:
						break;
					}
					break;
				case XSG_DOUBLE:
					switch (type_b) {
					case XSG_INT:
						type = mod_double_int_type;
						op_func = mod_double_int;
						break;
					case XSG_DOUBLE:
						type = mod_double_double_type;
						op_func = mod_double_double;
						break;
					case XSG_STRING:
						type = mod_double_string_type;
						op_func = mod_double_string;
						break;
					default:
						break;
					}
					break;
				case XSG_STRING:
					switch (type_b) {
					case XSG_INT:
						type = mod_string_int_type;
						op_func = mod_string_int;
						break;
					case XSG_DOUBLE:
						type = mod_string_double_type;
						op_func = mod_string_double;
						break;
					case XSG_STRING:
						type = mod_string_string_type;
						op_func = mod_string_string;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				break;
			case '.':
				switch (type_a) {
				case XSG_INT:
					switch (type_b) {
					case XSG_INT:
						type = cat_int_int_type;
						op_func = cat_int_int;
						break;
					case XSG_DOUBLE:
						type = cat_int_double_type;
						op_func = cat_int_double;
						break;
					case XSG_STRING:
						type = cat_int_string_type;
						op_func = cat_int_string;
						break;
					default:
						break;
					}
					break;
				case XSG_DOUBLE:
					switch (type_b) {
					case XSG_INT:
						type = cat_double_int_type;
						op_func = cat_double_int;
						break;
					case XSG_DOUBLE:
						type = cat_double_double_type;
						op_func = cat_double_double;
						break;
					case XSG_STRING:
						type = cat_double_string_type;
						op_func = cat_double_string;
						break;
					default:
						break;
					}
					break;
				case XSG_STRING:
					switch (type_b) {
					case XSG_INT:
						type = cat_string_int_type;
						op_func = cat_string_int;
						break;
					case XSG_DOUBLE:
						type = cat_string_double_type;
						op_func = cat_string_double;
						break;
					case XSG_STRING:
						type = cat_string_string_type;
						op_func = cat_string_string;
						break;
					default:
						break;
					}
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}

		if (op_func == NULL || type == 0)
			g_error("Unknown type or op_func");

		var->op_func = op_func;
	}
	val->type = type;

	g_debug("End update_op_funcs");
}

/******************************************************************************/

static char find_op() {
	if (xsg_conf_find_command("+"))
		return '+';
	if (xsg_conf_find_command("*"))
		return '*';
	if (xsg_conf_find_command("/"))
		return '/';
	if (xsg_conf_find_command("%"))
		return '%';
	if (xsg_conf_find_command("."))
		return '.';
	return 0;
}

uint16_t xsg_var_parse(uint64_t update, uint16_t widget_id) {
	static uint16_t var_id = 1;
	static uint16_t val_id = 1;
	val_t *val;
	char op;

	op = find_op();

	if (op == 0)
		return 0;

	val = g_new0(val_t, 1);
	val->update = update;
	val->var_list = NULL;
	val->buffer = NULL;
	val->widget_id = widget_id;
	val->val_id = val_id;

	val_id++;

	do {
		var_t *v;
		xsg_var_t var;

		xsg_modules_parse_var(&var, update, widget_id);

		v = g_new0(var_t, 1);
		v->op = op;
		v->type = var.type;
		v->func = var.func;
		v->args = var.args;
		v->var_id = var_id;
		v->val = val;

		var_id++;

		val->var_list = xsg_list_append(val->var_list, v);
		var_list = xsg_list_append(var_list, v);

		op = find_op();

	} while (op != 0);

	update_op_funcs(val);

	val_list = xsg_list_append(val_list, val);

	return val_id - 1;
}

/******************************************************************************/

void xsg_var_set_type(uint16_t var_id, uint8_t type) {
	var_t *var;

	var = get_var(var_id);
	var->type = type;
	update_op_funcs(var->val);
}

void xsg_var_update(uint16_t var_id) {
	var_t *var;

	var = get_var(var_id);
	xsg_widgets_update(var->val->widget_id, var->val->val_id);
}


