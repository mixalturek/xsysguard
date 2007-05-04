/* widget_image.c
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
 * Image <x> <y> <image> [Angle <angle>] [Scale <width> <height>]
 *
 */

/******************************************************************************/

#include "widgets.h"
#include "conf.h"

/******************************************************************************/

typedef struct {
	xsg_widget_angle_t *angle;
	char *filename;
	Imlib_Image image;
	bool scale;
} image_t;

/******************************************************************************/

static void render_image(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	image_t *image;

	xsg_debug("render_image");

	image = (image_t *) widget->data;

	imlib_context_set_image(buffer);

	if (image->angle)
		imlib_blend_image_onto_image_at_angle(image->image, 1, 0, 0,
				image->angle->width, image->angle->height,
				image->angle->xoffset - up_x, image->angle->yoffset - up_y,
				image->angle->angle_x, image->angle->angle_y);
	else
		imlib_blend_image_onto_image(image->image, 1, 0, 0,
				widget->width, widget->height, widget->xoffset - up_x,
				widget->yoffset - up_y, widget->width, widget->height);
}

static void update_image(xsg_widget_t *widget, uint32_t var_id) {
	return;
}

static void scroll_image(xsg_widget_t *widget) {
	return;
}

void xsg_widget_image_parse() {
	xsg_widget_t *widget;
	image_t *image;
	double angle = 0.0;

	widget = xsg_new0(xsg_widget_t, 1);
	image = xsg_new0(image_t, 1);

	widget->update = 0;
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = 0;
	widget->height = 0;
	widget->render_func = render_image;
	widget->update_func = update_image;
	widget->scroll_func = scroll_image;
	widget->data = (void *) image;

	image->angle = NULL;
	image->filename = xsg_conf_read_string();
	image->image = NULL;
	image->scale = FALSE;

	image->image = xsg_widgets_load_image(image->filename);
	if (unlikely(image->image == NULL))
		xsg_error("Cannot load image \"%s\"", image->filename);

	imlib_context_set_image(image->image);

	widget->width = imlib_image_get_width();
	widget->height = imlib_image_get_height();

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Scale")) {
			image->scale = TRUE;
			widget->width = xsg_conf_read_uint();
			widget->height = xsg_conf_read_uint();
		} else {
			xsg_conf_error("Angle or Scale");
		}
	}

	if (image->scale) {
		image->image = imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(),
				imlib_image_get_height(), widget->width, widget->height);
		imlib_free_image();
	}

	if (angle != 0.0) {
		image->angle = parse_angle(angle, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);

		if (image->angle->angle == 90.0) {
			imlib_image_orientate(1);
			image->angle = NULL;
		} else if (image->angle->angle == 180.0) {
			imlib_image_orientate(2);
			image->angle = NULL;
		} else if (image->angle->angle == 270.0) {
			imlib_image_orientate(3);
			image->angle = NULL;
		}
	}

	xsg_widgets_add(widget);
}


