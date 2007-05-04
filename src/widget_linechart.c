/* widget_linechart.c
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
 * LineChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Background <image>]
 * + <variable> <color> [AddPrev]
 *
 */

/******************************************************************************/

#include <math.h>
#include <float.h>

#include "widgets.h"
#include "angle.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	uint32_t var_id;
	Imlib_Color color;
	bool add_prev;
	double *values;
} linechart_var_t;

/******************************************************************************/

typedef struct {
	xsg_angle_t *angle;
	double min;
	double max;
	bool const_min;
	bool const_max;
	Imlib_Image background;
	xsg_list_t *var_list;
	unsigned int value_index;
} linechart_t;

/******************************************************************************/

static void render_linechart(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	linechart_t *linechart;
	linechart_var_t *linechart_var;
	double min, max;
	xsg_list_t *l;
	Imlib_Image tmp;
	ImlibPolygon poly;
	double pixel_h;
	unsigned int width;
	unsigned int height;
	unsigned int i;

	xsg_debug("render_linechart");

	linechart = (linechart_t *) widget->data;

	if (linechart->angle) {
		width = linechart->angle->width;
		height = linechart->angle->height;
	} else {
		width = widget->width;
		height = widget->height;
	}

	if (linechart->const_min) {
		min = linechart->min;
	} else {
		min = DBL_MAX;
		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;
			for (i = 0; i < width; i++)
				min = MIN(min, linechart_var->values[i]);
		}
		if (min == DBL_MAX)
			return;
	}

	if (linechart->const_max) {
		max = linechart->max;
	} else {
		max = DBL_MIN;
		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;
			for (i = 0; i < width; i++)
				max = MAX(max, linechart_var->values[i]);
		}
		if (max == DBL_MIN)
			return;
	}

	tmp = imlib_create_image(width, height);
	imlib_context_set_image(tmp);
	imlib_image_set_has_alpha(1);
	imlib_image_clear();

	/* TODO background image */

	pixel_h = (max - min) / (double) height;

	for (l= linechart->var_list; l; l = l->next) {
		linechart_var = l->data;

		poly = imlib_polygon_new();

		for (i = 0; i < width; i++) {
			unsigned int j;
			int x, y;

			j = (i + linechart->value_index) % width;
			x = width - i;
			y = height - linechart_var->values[j] * pixel_h;

			imlib_polygon_add_point(poly, x, y); /* FIXME */
		}

		imlib_image_draw_polygon(poly, 0);

		imlib_polygon_free(poly);
	}

	imlib_context_set_image(buffer);

	if (linechart->angle)
		imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0,
				linechart->angle->width, linechart->angle->height,
				linechart->angle->xoffset - up_x, linechart->angle->yoffset - up_y,
				linechart->angle->angle_x, linechart->angle->angle_y);
	else
		imlib_blend_image_onto_image(tmp, 1, 0, 0, widget->width, widget->height,
				widget->xoffset - up_x, widget->yoffset - up_y,
				widget->width, widget->height);

	imlib_context_set_image(tmp);
	imlib_free_image();
}

static void update_linechart(xsg_widget_t *widget, uint32_t var_id) {
	linechart_t *linechart;
	linechart_var_t *linechart_var;
	xsg_list_t *l;
	double prev = 0.0;
	unsigned int i;

	linechart = (linechart_t *) widget->data;
	i = linechart->value_index;
	for (l = linechart->var_list; l; l = l->next) {
		linechart_var = l->data;

		if ((var_id == 0xffffffff) || (linechart_var->var_id == var_id)) {
			linechart_var->values[i] = xsg_var_get_double(linechart_var->var_id);
			if (linechart_var->add_prev)
				linechart_var->values[i] += prev;
		}
		prev = linechart_var->values[i];
	}
}

static void scroll_linechart(xsg_widget_t *widget) {
	linechart_t *linechart;
	unsigned int width;

	linechart = (linechart_t *) widget->data;

	if (linechart->angle)
		width = linechart->angle->width;
	else
		width = widget->width;

	linechart->value_index = (linechart->value_index + 1) % width;
}

void xsg_widget_linechart_parse(uint64_t *update, uint32_t *widget_id) {
	xsg_widget_t *widget;
	linechart_t *linechart;
	double angle = 0.0;

	widget = xsg_new0(xsg_widget_t, 1);
	linechart = xsg_new0(linechart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->show_var_id = 0xffffffff;
	widget->show = TRUE;
	widget->render_func = render_linechart;
	widget->update_func = update_linechart;
	widget->scroll_func = scroll_linechart;
	widget->data = (void *) linechart;

	*update = widget->update;
	*widget_id = xsg_widgets_add(widget);

	linechart->angle = NULL;
	linechart->min = 0.0;
	linechart->max = 0.0;
	linechart->const_min = FALSE;
	linechart->const_max = FALSE;
	linechart->background = NULL;
	linechart->var_list = NULL;
	linechart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Show")) {
			widget->show_var_id = xsg_var_parse_double(*widget_id, *update);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Min")) {
			linechart->min = xsg_conf_read_double();
			linechart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			linechart->max = xsg_conf_read_double();
			linechart->const_max = TRUE;
		} else if (xsg_conf_find_command("Background")) {
			char *filename = xsg_conf_read_string();
			linechart->background = xsg_imlib_load_image(filename);
			if (unlikely(linechart->background == NULL))
				xsg_error("Cannot load image \"%s\"", filename);
			xsg_free(filename);
		} else {
			xsg_conf_error("Show, Angle, Min, Max or Background");
		}
	}

	if (angle != 0.0)
		linechart->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, &widget->width, &widget->height);

}

void xsg_widget_linechart_parse_var(uint32_t var_id) {
	xsg_widget_t *widget;
	linechart_t *linechart;
	linechart_var_t * linechart_var;
	unsigned int width, i;

	if (!xsg_var_is_double(var_id))
		xsg_error("LineChart allows double vars only");

	widget = xsg_widgets_last();
	linechart = widget->data;
	linechart_var = xsg_new0(linechart_var_t, 1);
	linechart->var_list = xsg_list_append(linechart->var_list, linechart_var);

	if (linechart->angle)
		width = linechart->angle->width;
	else
		width = widget->width;

	linechart_var->var_id = var_id;
	linechart_var->color = xsg_imlib_uint2color(xsg_conf_read_color());
	linechart_var->add_prev = FALSE;
	linechart_var->values = xsg_new0(double, width);

	for (i = 0; i < width; i++)
		linechart_var->values[i] = DNAN;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("AddPrev")) {
			linechart_var->add_prev = TRUE;
		} else {
			xsg_conf_error("AddPrev");
		}
	}
}


