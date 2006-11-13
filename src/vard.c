/* vard.c
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
#include <string.h>
#include <stdio.h>

#include "var.h"
#include "modules.h"

/******************************************************************************/

typedef struct _var_t {
	uint64_t update;
	uint8_t type;
	void *(*func)(void *args);
	void *args;
	uint16_t var_id;
} var_t;

/******************************************************************************/

static xsg_list_t *var_list = NULL;

/******************************************************************************/

static void send_var(var_t *v) {

	fwrite(&(v->var_id), 2, 1, stdout);
	if (v->type == XSG_STRING) {
		uint16_t len;
		char *buf;

		buf = (char *)(v->func)(v->args);
		len = strlen(buf);
		fwrite(&len, 2, 1, stdout);
		fwrite(buf, len, 1, stdout);
	} else {
		void *value;
		value = (v->func)(v->args);
		fwrite(value, 8, 1, stdout);
	}
	fflush(stdout);
}

static void update(uint64_t count) {
	xsg_list_t *l;
	var_t *v;

	for (l = var_list; l; l = l->next) {
		v = l->data;
		if ((count % v->update) == 0)
			send_var(v);
	}
}

/******************************************************************************/

void xsg_var_set_type(uint16_t var_id, uint8_t type) {
	xsg_list_t *l;
	var_t *v;

	for (l = var_list; l; l = l->next) {
		v = l->data;
		v->type = type;
	}
}

void xsg_var_update(uint16_t var_id) {
	xsg_list_t *l;
	var_t *v;

	for (l = var_list; l; l = l->next) {
		v = l->data;
		if (v->var_id == var_id)
			send_var(v);
	}
}

/******************************************************************************/

uint16_t xsg_var_parse(uint64_t update, uint16_t var_id) {
	xsg_var_t var;
	var_t *v;

	xsg_modules_parse_var(&var, update, var_id);

	v = g_new0(var_t, 1);

	v->update = update;
	v->type = var.type;
	v->func = var.func;
	v->args = var.args;
	v->var_id = var_id;

	var_list = xsg_list_append(var_list, v);

	return var_id;
}

void xsg_var_init() {
	xsg_main_add_update_func(update);
}

