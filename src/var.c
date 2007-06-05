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

struct _xsg_var_t {
	xsg_window_t *window;
	xsg_widget_t *widget;
	xsg_rpn_t *rpn;
};

/******************************************************************************/

static xsg_list_t *var_list = NULL;

static bool dirty = FALSE;

/******************************************************************************/

void xsg_var_init(void) {
	xsg_rpn_init();
}

/******************************************************************************/

void xsg_var_dirty(xsg_var_t *var) {
	xsg_window_update(var->window, var->widget, var);
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

xsg_var_t *xsg_var_parse(xsg_window_t *window, xsg_widget_t *widget, uint64_t update) {
	xsg_var_t *var;

	var = xsg_new(xsg_var_t, 1);
	var->window = window;
	var->widget = widget;
	var->rpn = xsg_rpn_parse(var, update);

	var_list = xsg_list_append(var_list, var);

	return var;
}

double xsg_var_get_num(xsg_var_t *var) {
	return xsg_rpn_get_num(var->rpn);
}

char *xsg_var_get_str(xsg_var_t *var) {
	return xsg_rpn_get_str(var->rpn);
}

