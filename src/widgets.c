/* widgets.c
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

#include "widgets.h"
#include "widget.h"
#include "window.h"
#include "imlib.h"
#include "var.h"

/******************************************************************************/

static xsg_list_t *widget_list = NULL;

/******************************************************************************/

xsg_widget_t *xsg_widgets_new(xsg_window_t *window) {
	xsg_widget_t *widget;

	widget = xsg_new(xsg_widget_t, 1);

	widget->window = window;
	widget->update = 0;
	widget->xoffset = 0;
	widget->yoffset = 0;
	widget->width = 0;
	widget->height = 0;
	widget->visible_update = 0;
	widget->visible_var = NULL;
	widget->visible = TRUE;
	widget->render_func = NULL;
	widget->update_func = NULL;
	widget->scroll_func = NULL;
	widget->data = NULL;

	widget_list = xsg_list_append(widget_list, widget);

	xsg_window_add_widget(window, widget);

	return widget;
}

/******************************************************************************/

xsg_widget_t *xsg_widgets_last() {
	xsg_list_t *l;

	l = xsg_list_last(widget_list);

	if (unlikely(l == NULL))
		xsg_error("No widgets available");

	return l->data;
}

/******************************************************************************
 *
 * render
 *
 ******************************************************************************/

static bool widget_rect(xsg_widget_t *widget, int x, int y, unsigned int w, unsigned int h) {
	int x1_1, x2_1, y1_1, y2_1, x1_2, x2_2, y1_2, y2_2;
	bool x_overlap, y_overlap;

	x1_1 = widget->xoffset;
	x2_1 = widget->xoffset + widget->width;
	y1_1 = widget->yoffset;
	y2_1 = widget->yoffset + widget->height;

	x1_2 = x;
	x2_2 = x + w;
	y1_2 = y;
	y2_2 = y + h;

	x_overlap = !((x2_2 <= x1_1) || (x2_1 <= x1_2));
	y_overlap = !((y2_2 <= y1_1) || (y2_1 <= y1_2));

	return x_overlap && y_overlap;
}

void xsg_widgets_render(xsg_widget_t *w, Imlib_Image buffer, int up_x, int up_y, int up_w, int up_h) {
	xsg_widget_t *widget = w;

	if (widget->visible && widget_rect(widget, up_x, up_y, up_w, up_h))
		(widget->render_func)(widget, buffer, up_x, up_y);
}

/******************************************************************************
 *
 * update
 *
 ******************************************************************************/

void xsg_widgets_update(xsg_widget_t *widget, xsg_var_t *var) {
	if (widget->visible_var == var) {
		bool visible = widget->visible;

		if (widget->visible_update != 0)
			widget->visible = (xsg_var_get_num(var) == 0.0) ? FALSE : TRUE;
		else
			widget->visible = TRUE;
		if (widget->visible != visible)
			xsg_window_update_append_rect(widget->window, widget->xoffset, widget->yoffset,
					widget->width, widget->height);
	} else {
		(widget->update_func)(widget, var);
	}
}

static void scroll_and_update(uint64_t tick) {
	xsg_list_t *l;

	for (l = widget_list; l; l = l->next) {
		xsg_widget_t *widget = l->data;

		if ((widget->visible_update != 0) && (tick % widget->visible_update) == 0) {
			bool visible = widget->visible;

			widget->visible = (xsg_var_get_num(widget->visible_var) == 0.0) ? FALSE : TRUE;

			if (visible != widget->visible)
				xsg_window_update_append_rect(widget->window, widget->xoffset, widget->yoffset,
						widget->width, widget->height);
		}

		if ((widget->update != 0) && (tick % widget->update) == 0) {
			(widget->scroll_func)(widget);
			(widget->update_func)(widget, NULL);
		}

		if (tick == 0)
			(widget->update_func)(widget, NULL);
	}
}

/******************************************************************************
 *
 * init
 *
 ******************************************************************************/

void xsg_widgets_init() {
	xsg_main_add_update_func(scroll_and_update);
}

