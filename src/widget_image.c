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
#include "angle.h"
#include "imlib.h"
#include "conf.h"

/******************************************************************************/

typedef struct {
	xsg_angle_t *angle;
	char *filename;
} image_t;

/******************************************************************************/

static void render_image(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	image_t *image;
	Imlib_Image img;

	xsg_debug("render_image");

	image = (image_t *) widget->data;

	img = xsg_imlib_load_image(image->filename);

	if (unlikely(img == NULL)) {
		xsg_warning("Cannot load image \"%s\"", image->filename);
		return;
	}

	imlib_context_set_image(buffer);

	if ((image->angle == NULL) || (image->angle->angle == 0.0)) {
		imlib_blend_image_onto_image(img, 1, 0, 0,
				widget->width, widget->height, widget->xoffset - up_x,
				widget->yoffset - up_y, widget->width, widget->height);
	} else {
		imlib_blend_image_onto_image_at_angle(img, 1, 0, 0,
				image->angle->width, image->angle->height,
				image->angle->xoffset - up_x, image->angle->yoffset - up_y,
				image->angle->angle_x, image->angle->angle_y);
	}

	imlib_context_set_image(img);
	imlib_free_image();
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
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_image;
	widget->update_func = update_image;
	widget->scroll_func = scroll_image;
	widget->data = (void *) image;

	image->angle = NULL;
	image->filename = xsg_conf_read_string();

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else {
			xsg_conf_error("Angle");
		}
	}

	if (angle != 0.0)
		image->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, &widget->width, &widget->height);

	xsg_widgets_add(widget);
}


