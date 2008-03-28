/* widget_image.c
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
#include <string.h>

#include "widgets.h"
#include "widget.h"
#include "window.h"
#include "angle.h"
#include "printf.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	xsg_printf_t *print;
	xsg_angle_t *angle;
	xsg_string_t *filename;
} image_t;

/******************************************************************************/

static void
render_image(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y)
{
	image_t *image;
	Imlib_Image img;

	image = (image_t *) widget->data;

	xsg_debug("%s: render Image: x=%d, y=%d, "
			"widht=%u, height=%u, filename=%s",
			xsg_window_get_config_name(widget->window),
			widget->xoffset, widget->yoffset,
			widget->width, widget->height, image->filename->str);

	img = xsg_imlib_load_image(image->filename->str);

	if (unlikely(img == NULL)) {
		xsg_warning("%s: render Image: cannot load image \"%s\"",
				xsg_window_get_config_name(widget->window),
				image->filename->str);
		return;
	}

	imlib_context_set_image(buffer);

	if ((image->angle == NULL) || (image->angle->angle == 0.0)) {
		imlib_blend_image_onto_image(img, 1, 0, 0,
				widget->width, widget->height,
				widget->xoffset - up_x, widget->yoffset - up_y,
				widget->width, widget->height);
	} else {
		imlib_blend_image_onto_image_at_angle(img, 1, 0, 0,
				image->angle->width, image->angle->height,
				image->angle->xoffset - up_x,
				image->angle->yoffset - up_y,
				image->angle->angle_x, image->angle->angle_y);
	}

	imlib_context_set_image(img);
	imlib_free_image();
}

static void
update_image(xsg_widget_t *widget, xsg_var_t *var)
{
	image_t *image;
	char *filename;

	image = widget->data;

	filename = xsg_printf(image->print, var);

	if (!filename) {
		return;
	}

	if (!strcmp(filename, image->filename->str)) {
		return;
	}

	xsg_string_assign(image->filename, filename);

	xsg_window_update_append_rect(widget->window, widget->xoffset,
				widget->yoffset, widget->width, widget->height);
}

static void
scroll_image(xsg_widget_t *widget)
{
	return;
}

/******************************************************************************/

void
xsg_widget_image_parse(xsg_window_t *window)
{
	xsg_widget_t *widget;
	image_t *image;
	double angle = 0.0;

	widget = xsg_widgets_new(window);

	image = xsg_new(image_t, 1);

	widget->update = xsg_conf_read_update();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_image;
	widget->update_func = update_image;
	widget->scroll_func = scroll_image;
	widget->data = (void *) image;

	image->print = xsg_printf_new(xsg_conf_read_string());
	image->angle = NULL;
	image->filename = xsg_string_new(NULL);

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_update();
			widget->visible_var = xsg_var_parse_num(
					widget->visible_update, window, widget);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else {
			xsg_conf_error("Visible or Angle expected");
		}
	}

	if (angle != 0.0) {
		image->angle = xsg_angle_parse(angle, widget->xoffset,
				widget->yoffset, widget->width, widget->height);
	}

	while (xsg_conf_find_command("+")) {
		xsg_var_t *var;

		if (xsg_printf_next_var_is_string(image->print)) {
			var = xsg_var_parse_str(widget->update, window, widget);
		} else {
			var = xsg_var_parse_num(widget->update, window, widget);
		}

		xsg_printf_add_var(image->print, var);
		xsg_conf_read_newline();
	}
}

