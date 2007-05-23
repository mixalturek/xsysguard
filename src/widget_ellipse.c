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

	xsg_debug("render_ellipse");

	ellipse = (ellipse_t *) widget->data;

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

void xsg_widget_ellipse_parse() {
	xsg_widget_t *widget;
	ellipse_t *ellipse;
	uint64_t update;
	uint32_t widget_id;

	widget = xsg_new0(xsg_widget_t, 1);
	ellipse = xsg_new0(ellipse_t, 1);

	update = xsg_conf_read_uint();
	widget_id = xsg_widgets_add(widget);

	ellipse->xc = xsg_conf_read_int();
	ellipse->yc = xsg_conf_read_int();
	ellipse->a = xsg_conf_read_uint();
	ellipse->b = xsg_conf_read_uint();
	ellipse->color = xsg_imlib_uint2color(xsg_conf_read_color());
	ellipse->filled = FALSE;

	widget->update = update;
	widget->xoffset = ellipse->xc - ellipse->a;
	widget->yoffset = ellipse->yc - ellipse->b;
	widget->width = ellipse->a * 2;
	widget->height = ellipse->b * 2;
	widget->show_var_id = 0xffffffff;
	widget->show = TRUE;
	widget->render_func = render_ellipse;
	widget->update_func = update_ellipse;
	widget->scroll_func = scroll_ellipse;
	widget->data = (void *) ellipse;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Show")) {
			widget->show_var_id = xsg_var_parse(widget_id, update);
		} else if (xsg_conf_find_command("Filled")) {
			ellipse->filled = TRUE;
		} else {
			xsg_conf_error("Show or Filled");
		}
	}

}


