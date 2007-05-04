/* widget_polygon.c
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
 * Polygon <color> <count> <x> <y> <x> <y> ... [Filled|Closed]
 *
 */

/******************************************************************************/

#include "widgets.h"
#include "imlib.h"
#include "conf.h"

/******************************************************************************/

typedef struct {
	Imlib_Color color;
	ImlibPolygon polygon;
	bool filled;
	bool closed;
} polygon_t;

/******************************************************************************/

typedef struct {
	int x, y;
} point_t;

typedef struct {
	point_t *points;
	int pointcount;
	int lx, rx;
	int ty, by;
} poly_t;

static ImlibPolygon poly_copy(ImlibPolygon polygon, int xoffset, int yoffset) {
	poly_t *poly;
	poly_t *new_poly;
	int i;

	poly = (poly_t *) polygon;
	new_poly = (poly_t *) xsg_malloc(sizeof(poly_t));
	new_poly->points = (point_t *) xsg_malloc(sizeof(point_t) * poly->pointcount);
	new_poly->pointcount = poly->pointcount;
	new_poly->lx = poly->lx + xoffset;
	new_poly->rx = poly->rx + xoffset;
	new_poly->ty = poly->ty + yoffset;
	new_poly->by = poly->by + yoffset;

	for (i = 0; i < poly->pointcount; i++) {
		new_poly->points[i].x = poly->points[i].x + xoffset;
		new_poly->points[i].y = poly->points[i].y + yoffset;
	}

	return (ImlibPolygon) new_poly;
}

/******************************************************************************/

static void render_polygon(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	polygon_t *polygon;
	ImlibPolygon poly;

	xsg_debug("render_polygon");

	polygon = (polygon_t *) widget->data;

	imlib_context_set_image(buffer);

	imlib_context_set_color(polygon->color.red, polygon->color.green,
			polygon->color.blue, polygon->color.alpha);

	poly = poly_copy(polygon->polygon, -up_x, -up_y);

	if (polygon->filled)
		imlib_image_fill_polygon(poly);
	else
		imlib_image_draw_polygon(poly, polygon->closed);

	imlib_polygon_free(poly);
}

static void update_polygon(xsg_widget_t *widget, uint32_t var_id) {
	return;
}

static void scroll_polygon(xsg_widget_t *widget) {
	return;
}

void xsg_widget_polygon_parse() {
	xsg_widget_t *widget;
	polygon_t *polygon;
	unsigned int count, i;
	int x1, y1, x2, y2;

	widget = xsg_new0(xsg_widget_t, 1);
	polygon = xsg_new0(polygon_t, 1);

	polygon->color = xsg_imlib_uint2color(xsg_conf_read_color());
	polygon->polygon = imlib_polygon_new();
	polygon->filled = FALSE;
	polygon->closed = FALSE;

	count = xsg_conf_read_uint();
	for (i = 0; i < count; i++) {
		int x, y;
		x = xsg_conf_read_int();
		y = xsg_conf_read_int();
		imlib_polygon_add_point(polygon->polygon, x, y);
	}

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Filled"))
			polygon->filled = TRUE;
		else if (xsg_conf_find_command("Closed"))
			polygon->closed = TRUE;
		else
			xsg_conf_error("Filled or Closed");
	}

	imlib_polygon_get_bounds(polygon->polygon, &x1, &y1, &x2, &y2);

	widget->update = 0;
	widget->xoffset = x1;
	widget->yoffset = y1;
	widget->width = x2 - x1;
	widget->height = y2 - y1;
	widget->render_func = render_polygon;
	widget->update_func = update_polygon;
	widget->scroll_func = scroll_polygon;
	widget->data = (void *) polygon;

	xsg_widgets_add(widget);
}


