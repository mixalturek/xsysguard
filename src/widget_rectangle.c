/* widget_rectangle.c
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
 * Rectangle <x> <y> <width> <height> <color> [Angle <angle>] [ColorRange <angle> <count> <distance> <color> ...] [Filled]
 *
 */

/******************************************************************************/

#include "widgets.h"
#include "angle.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	xsg_angle_t *angle;
	Imlib_Color color;
	Imlib_Color_Range range;
	double range_angle;
	bool filled;
} rectangle_t;

/******************************************************************************/

static void render_rectangle(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	rectangle_t *rectangle;
	int xoffset, yoffset;
	unsigned int width, height;
	double angle;

	xsg_debug("render_rectangle");

	rectangle = (rectangle_t *) widget->data;

	if (rectangle->angle == NULL)
		angle = 0.0;
	else
		angle = rectangle->angle->angle;

	if ((angle == 0.0) || (angle == 90.0) || (angle == 180.0) || (angle == 270.0)) {
		xoffset = widget->xoffset - up_x;
		yoffset = widget->yoffset - up_y;
		width = widget->width;
		height = widget->height;

		imlib_context_set_image(buffer);

		if (rectangle->range) {
			double range_angle = rectangle->range_angle + angle;
			imlib_context_set_color_range(rectangle->range);
			T(imlib_image_fill_color_range_rectangle(xoffset, yoffset, width, height, range_angle));
		} else if (rectangle->filled) {
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			T(imlib_image_fill_rectangle(xoffset, yoffset, width, height));
		} else {
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			T(imlib_image_draw_rectangle(xoffset, yoffset, width, height));
		}
	} else {
		Imlib_Image tmp;

		xoffset = 0;
		yoffset = 0;
		width = rectangle->angle->width;
		height = rectangle->angle->height;

		tmp = imlib_create_image(width, height);

		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		T(imlib_image_clear());

		if (rectangle->range) {
			double range_angle = rectangle->range_angle;
			imlib_context_set_color_range(rectangle->range);
			T(imlib_image_fill_color_range_rectangle(xoffset, yoffset, width, height, range_angle));
		} else if (rectangle->filled) {
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			T(imlib_image_fill_rectangle(xoffset, yoffset, width, height));
		} else {
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			T(imlib_image_draw_rectangle(xoffset, yoffset, width, height));
		}

		imlib_context_set_image(buffer);

		T(imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0,
				rectangle->angle->width, rectangle->angle->height,
				rectangle->angle->xoffset - up_x, rectangle->angle->yoffset - up_y,
				rectangle->angle->angle_x, rectangle->angle->angle_y));

		imlib_context_set_image(tmp);
		imlib_free_image();
	}
}

static void update_rectangle(xsg_widget_t *widget, uint32_t var_id) {
	return;
}

static void scroll_rectangle(xsg_widget_t *widget) {
	return;
}

void xsg_widget_rectangle_parse() {
	xsg_widget_t *widget;
	rectangle_t *rectangle;
	double angle = 0.0;
	uint32_t widget_id;

	widget = xsg_new0(xsg_widget_t, 1);
	rectangle = xsg_new(rectangle_t, 1);

	widget_id = xsg_widgets_add(widget);

	widget->update = xsg_conf_read_uint();;
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->show_var_id = 0xffffffff;
	widget->show = TRUE;
	widget->render_func = render_rectangle;
	widget->update_func = update_rectangle;
	widget->scroll_func = scroll_rectangle;
	widget->data = (void *) rectangle;

	rectangle->angle = NULL;
	rectangle->color = xsg_imlib_uint2color(xsg_conf_read_color());
	rectangle->range = NULL;
	rectangle->range_angle = 0.0;
	rectangle->filled = FALSE;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Show")) {
			widget->show_var_id = xsg_var_parse(widget_id, widget->update);
		} else if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;

			if (rectangle->range != NULL) {
				imlib_context_set_color_range(rectangle->range);
				imlib_free_color_range();
			}

			rectangle->range = imlib_create_color_range();
			imlib_context_set_color_range(rectangle->range);
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			imlib_add_color_to_color_range(0);
			rectangle->range_angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = xsg_imlib_uint2color(xsg_conf_read_color());
				imlib_context_set_color(color.red, color.green,
						color.blue, color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Filled")) {
			rectangle->filled = TRUE;
		} else {
			xsg_conf_error("Show, Angle, ColorRange or Filled");
		}
	}

	if (angle != 0.0)
		rectangle->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, &widget->width, &widget->height);
}


