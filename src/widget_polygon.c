/* widget_polygon.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2008 Sascha Wessel <sawe@users.sf.net>
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
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	int x;
	int y;
} point_t;

typedef struct {
	Imlib_Color color;
	unsigned int point_count;
	point_t *points;
	bool filled;
	bool closed;
} polygon_t;

/******************************************************************************/

static void
render_polygon(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y)
{
	polygon_t *polygon;
	ImlibPolygon poly;
	unsigned int i;

	xsg_debug("%s: render Polygon",
			xsg_window_get_config_name(widget->window));

	polygon = (polygon_t *) widget->data;

	poly = imlib_polygon_new();

	for (i = 0; i < polygon->point_count; i++) {
		imlib_polygon_add_point(poly, polygon->points[i].x - up_x,
				polygon->points[i].y - up_y);
	}

	imlib_context_set_image(buffer);

	imlib_context_set_color(polygon->color.red, polygon->color.green,
			polygon->color.blue, polygon->color.alpha);

	if (polygon->filled) {
		imlib_image_fill_polygon(poly);
	} else {
		imlib_image_draw_polygon(poly, polygon->closed);
	}

	imlib_polygon_free(poly);
}

static void
update_polygon(xsg_widget_t *widget, xsg_var_t *var)
{
	return;
}

static void
scroll_polygon(xsg_widget_t *widget)
{
	return;
}

void
xsg_widget_polygon_parse(xsg_window_t *window)
{
	xsg_widget_t *widget;
	polygon_t *polygon;
	ImlibPolygon poly;
	unsigned int i;
	int x1, y1, x2, y2;

	widget = xsg_widgets_new(window);

	polygon = xsg_new(polygon_t, 1);

	xsg_imlib_uint2color(xsg_conf_read_color(), &polygon->color);
	polygon->point_count = xsg_conf_read_uint();
	polygon->points = xsg_new(point_t, polygon->point_count);;
	polygon->filled = FALSE;
	polygon->closed = FALSE;

	poly = imlib_polygon_new();

	for (i = 0; i < polygon->point_count; i++) {
		polygon->points[i].x = xsg_conf_read_int();
		polygon->points[i].y = xsg_conf_read_int();
		imlib_polygon_add_point(poly, polygon->points[i].x,
				polygon->points[i].y);
	}

	imlib_polygon_get_bounds(poly, &x1, &y1, &x2, &y2);
	imlib_polygon_free(poly);

	widget->xoffset = x1;
	widget->yoffset = y1;
	widget->width = x2 - x1;
	widget->height = y2 - y1;
	widget->render_func = render_polygon;
	widget->update_func = update_polygon;
	widget->scroll_func = scroll_polygon;
	widget->data = (void *) polygon;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_update();
			widget->visible_var = xsg_var_parse_num(
					widget->visible_update, window, widget);
		} else if (xsg_conf_find_command("Filled")) {
			polygon->filled = TRUE;
		} else if (xsg_conf_find_command("Closed")) {
			polygon->closed = TRUE;
		} else {
			xsg_conf_error("Visible, Filled or Closed expected");
		}
	}
}

