/* widget_line.c
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
 * Line <x1> <y1> <x2> <y2> <color>
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
	int x1;
	int y1;
	int x2;
	int y2;
	Imlib_Color color;
} line_t;

/******************************************************************************/

static void render_line(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	line_t *line;

	line = (line_t *) widget->data;

	xsg_debug("%s: Render Line: x1=%d, y1=%d, x2=%d, y2=%d, red=0x%x, green=0x%x, blue=0x%x, alpha=0x%x",
			xsg_window_get_config_name(widget->window),
			line->x1, line->y1, line->x2, line->y2,
			line->color.red, line->color.green, line->color.blue, line->color.alpha);

	imlib_context_set_image(buffer);

	imlib_context_set_color(line->color.red, line->color.green,
			line->color.blue, line->color.alpha);

	imlib_image_draw_line(line->x1 - up_x, line->y1 - up_y,
			line->x2 - up_x, line->y2 - up_y, 0);
}

static void update_line(xsg_widget_t *widget, xsg_var_t *var) {
	return;
}

static void scroll_line(xsg_widget_t *widget) {
	return;
}

void xsg_widget_line_parse(xsg_window_t *window) {
	xsg_widget_t *widget;
	line_t *line;

	widget = xsg_widgets_new(window);

	line = xsg_new(line_t, 1);

	line->x1 = xsg_conf_read_int();
	line->y1 = xsg_conf_read_int();
	line->x2 = xsg_conf_read_int();
	line->y2 = xsg_conf_read_int();
	line->color = xsg_imlib_uint2color(xsg_conf_read_color());
	xsg_conf_read_newline();

	widget->xoffset = MIN(line->x1, line->x2);
	widget->yoffset = MIN(line->y1, line->y2);
	widget->width = MAX(line->x1, line->x2) - widget->xoffset + 1;
	widget->height = MAX(line->y1, line->y2) - widget->yoffset + 1;
	widget->render_func = render_line;
	widget->update_func = update_line;
	widget->scroll_func = scroll_line;
	widget->data = (void *) line;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_uint();
			widget->visible_var = xsg_var_parse(window, widget, widget->visible_update);
		} else {
			xsg_conf_error("Visible");
		}
	}
}


