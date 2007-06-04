/* widget_ellipse.c
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

/*
 *
 * Ellipse <xc> <yc> <a> <b> <color> [Filled]
 *
 */

/******************************************************************************/

#include "widgets.h"
#include "widget.h"
#include "window.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	int xc;
	int yc;
	int a;
	int b;
	Imlib_Color color;
	bool filled;
} ellipse_t;

/******************************************************************************/

static void render_ellipse(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	ellipse_t *ellipse;

	ellipse = (ellipse_t *) widget->data;

	xsg_debug("%s: Render Ellipse: xc=%d, yc=%d, a=%d, b=%d, red=%d, green=%d, blue=%d, alpha=%d",
			xsg_window_get_config_name(widget->window_id),
			ellipse->xc, ellipse->yc, ellipse->a, ellipse->b,
			ellipse->color.red, ellipse->color.green, ellipse->color.blue, ellipse->color.alpha);

	imlib_context_set_image(buffer);

	imlib_context_set_color(ellipse->color.red, ellipse->color.green,
			ellipse->color.blue, ellipse->color.alpha);

	if (ellipse->filled)
		imlib_image_fill_ellipse(ellipse->xc - up_x, ellipse->yc - up_y,
				ellipse->a, ellipse->b);
	else
		imlib_image_draw_ellipse(ellipse->xc - up_x, ellipse->yc - up_y,
				ellipse->a, ellipse->b);
}

static void update_ellipse(xsg_widget_t *widget, uint32_t var_id) {
	return;
}

static void scroll_ellipse(xsg_widget_t *widget) {
	return;
}

void xsg_widget_ellipse_parse(uint32_t window_id) {
	xsg_widget_t *widget;
	ellipse_t *ellipse;

	widget = xsg_widgets_new(window_id);;

	ellipse = xsg_new(ellipse_t, 1);

	ellipse->xc = xsg_conf_read_int();
	ellipse->yc = xsg_conf_read_int();
	ellipse->a = xsg_conf_read_uint();
	ellipse->b = xsg_conf_read_uint();
	ellipse->color = xsg_imlib_uint2color(xsg_conf_read_color());
	ellipse->filled = FALSE;

	widget->xoffset = ellipse->xc - ellipse->a;
	widget->yoffset = ellipse->yc - ellipse->b;
	widget->width = ellipse->a * 2;
	widget->height = ellipse->b * 2;
	widget->render_func = render_ellipse;
	widget->update_func = update_ellipse;
	widget->scroll_func = scroll_ellipse;
	widget->data = (void *) ellipse;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_uint();
			widget->visible_var_id = xsg_var_parse(window_id, widget->id, widget->visible_update);
		} else if (xsg_conf_find_command("Filled")) {
			ellipse->filled = TRUE;
		} else {
			xsg_conf_error("Visible or Filled");
		}
	}

}


