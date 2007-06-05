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
#include <errno.h>

#include "vard.h"
#include "rpn.h"
#include "modules.h"
#include "writebuffer.h"

/******************************************************************************/

struct _xsg_var_t {
	uint64_t update;
	uint32_t remote_id;

	bool dirty;

	enum {
		END = 0x00,
		NUM = 0x01,
		STR = 0x02
	} type;

	xsg_string_t *str;
	double num;

	xsg_rpn_t *rpn;
};

/******************************************************************************/

static xsg_list_t *var_list = NULL;

static bool dirty = FALSE;

/******************************************************************************/

void xsg_var_queue_vars(void) {
	xsg_list_t *l;

	if (!dirty)
		return;

	if (!xsg_writebuffer_ready())
		return;

	for (l = var_list; l; l = l->next) {
		xsg_var_t *var = l->data;

		if (!var->dirty)
			continue;

		if (var->type == NUM)
			xsg_writebuffer_queue_num(var->remote_id, var->num);
		else if (var->type == STR)
			xsg_writebuffer_queue_str(var->remote_id, var->str);
		else
			xsg_error("invalid var type");

		var->dirty = FALSE;
	}

	dirty = FALSE;

	xsg_writebuffer_flush();
}

/******************************************************************************/

static void update_var(xsg_var_t *var) {
	if (var->type == NUM) {
		double num;

		num = xsg_rpn_get_num(var->rpn);

		if (num != var->num) {
			var->num = num;
			var->dirty = TRUE;
			dirty = TRUE;
		}
	} else if (var->type == STR) {
		char *str;

		str = xsg_rpn_get_str(var->rpn);

		if (strcmp(str, var->str->str) != 0) {
			var->str = xsg_string_assign(var->str, str);
			var->dirty = TRUE;
			dirty = TRUE;
		}
	} else {
		xsg_error("invalid var type");
	}
}

/******************************************************************************/

static void update(uint64_t tick) {
	xsg_list_t *l;

	xsg_writebuffer_queue_alive();

	dirty = TRUE;

	for (l = var_list; l; l = l->next) {
		xsg_var_t *var = l->data;
		if ((var->update != 0) && (tick % var->update) == 0)
			update_var(var);
	}

	xsg_var_queue_vars();
}

/******************************************************************************/

void xsg_var_dirty(xsg_var_t *var) {
	update_var(var);
}

void xsg_var_flush_dirty(void) {
	xsg_var_queue_vars();
}

/******************************************************************************/

void xsg_var_parse(uint32_t remote_id, uint64_t update, uint8_t type) {
	xsg_var_t *var;

	var = xsg_new(xsg_var_t, 1);

	var->update = update;
	var->remote_id = remote_id;
	var->dirty = FALSE;

	var->type = type;

	if (var->type == STR)
		var->str = xsg_string_new(NULL);
	else
		var->str = NULL;

	var->num = DNAN;

	var->rpn = xsg_rpn_parse(var, update);

	var_list = xsg_list_append(var_list, var);
}

void xsg_var_init() {
	xsg_main_add_update_func(update);
}

