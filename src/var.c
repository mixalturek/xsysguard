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

#include "window.h"
#include "modules.h"
#include "var.h"
#include "rpn.h"

/******************************************************************************/

typedef struct _var_t var_t;

struct _var_t {
	uint32_t widget_id;
	uint32_t rpn_id;
};

/******************************************************************************/

static uint32_t var_count = 0;
static xsg_list_t *var_list = NULL;
static var_t **var_array = NULL;

static bool dirty = FALSE;

/******************************************************************************/

static var_t *get_var(uint32_t var_id) {
	if (unlikely(var_array == NULL)) {
		var_t *var;

		xsg_debug("var_array is NULL, using var_list...");
		var = xsg_list_nth_data(var_list, var_id);
		if (unlikely(var == NULL))
			xsg_error("invalid var_id: %"PRIu32, var_id);
		else
			return var;
	}

	if (unlikely(var_id >= var_count))
		xsg_error("invalid var_id: %"PRIu32, var_id);

	return var_array[var_id];
}

static void build_var_array(void) {
	xsg_list_t *l;
	uint32_t var_id = 0;

	var_array = xsg_new0(var_t *, var_count);

	for (l = var_list; l; l = l->next) {
		var_array[var_id] = l->data;
		var_id++;
	}
}

/******************************************************************************/

void xsg_var_init(void) {
	build_var_array();
	xsg_rpn_init();
}

/******************************************************************************/

void xsg_var_dirty(uint32_t var_id) {
	var_t *var;

	var = get_var(var_id);

	xsg_window_update_widget(var->widget_id, var_id);

	dirty = TRUE;
}

void xsg_var_flush_dirty(void) {
	if (dirty) {
		xsg_window_render();
		xsg_window_render_xshape();
		dirty = FALSE;
	}
}

/******************************************************************************/

uint32_t xsg_var_parse(uint32_t widget_id, uint64_t update) {
	var_t *var;

	var = xsg_new0(var_t, 1);
	var->widget_id = widget_id;
	var->rpn_id = xsg_rpn_parse(var_count, update);

	var_list = xsg_list_append(var_list, var);
	return var_count++;
}

double xsg_var_get_num(uint32_t var_id) {
	var_t *var;

	var = get_var(var_id);

	return xsg_rpn_get_num(var->rpn_id);
}

char *xsg_var_get_str(uint32_t var_id) {
	var_t *var;

	var = get_var(var_id);

	return xsg_rpn_get_str(var->rpn_id);
}

