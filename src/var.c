/* var.c
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
#include <math.h>

#include "window.h"
#include "modules.h"
#include "var.h"
#include "rpn.h"

/******************************************************************************/

struct _xsg_var_t {
	xsg_window_t *window;
	xsg_widget_t *widget;
	xsg_rpn_t *rpn;
	bool dirty;
};

/******************************************************************************/

static xsg_list_t *var_list = NULL;

static bool dirty = FALSE;

/******************************************************************************/

void xsg_var_init(void) {
	xsg_rpn_init();
}

/******************************************************************************/

void xsg_var_dirty(xsg_var_t **var, uint32_t n) {
	uint32_t i;

	if (unlikely(var == NULL))
		return;

	for (i = 0; i < n; i++)
		if (likely(var[i] != NULL))
			var[i]->dirty = TRUE;

	dirty = TRUE;
}

void xsg_var_flush_dirty(void) {
	if (dirty) {
		xsg_list_t *l;

		for (l = var_list; l; l = l->next) {
			xsg_var_t *var = l->data;

			if (unlikely(var->dirty)) {
				xsg_window_update(var->window, var->widget, var);
				var->dirty = FALSE;
			}
		}
		xsg_window_render();
		dirty = FALSE;
	}
}

/******************************************************************************/

xsg_var_t **xsg_var_parse_past(uint64_t update, xsg_window_t *window, xsg_widget_t *widget, uint32_t n) {
	xsg_rpn_t **rpn;
	xsg_var_t **var;
	xsg_var_t *mem;
	uint32_t i;

	rpn = alloca(sizeof(xsg_rpn_t **) * n);

	var = xsg_new(xsg_var_t *, n);
	mem = xsg_new(xsg_var_t, n);

	for (i = 0; i < n; i++)
		var[i] = mem + i;

	xsg_rpn_parse(update, var, rpn, n);

	for (i = 0; i < n; i++) {
		var[i]->window = window;
		var[i]->widget = widget;
		var[i]->rpn = rpn[i];
		var[i]->dirty = FALSE;

		var_list = xsg_list_append(var_list, var[i]);
	}

	return var;
}

xsg_var_t *xsg_var_parse(uint64_t update, xsg_window_t *window, xsg_widget_t *widget) {
	xsg_var_t *var;
	xsg_rpn_t *rpn;

	var = xsg_new(xsg_var_t, 1);

	xsg_rpn_parse(update, &var, &rpn, 1);

	var->window = window;
	var->widget = widget;
	var->rpn = rpn;
	var->dirty = FALSE;

	var_list = xsg_list_append(var_list, var);

	return var;
}

double xsg_var_get_num(xsg_var_t *var) {
	return xsg_rpn_get_num(var->rpn);
}

char *xsg_var_get_str(xsg_var_t *var) {
	return xsg_rpn_get_str(var->rpn);
}

