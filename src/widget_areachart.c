/* widget_areachart.c
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
 * AreaChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Background <image>]
 * + <variable> <color> [ColorRange <angle> <count> <distance> <color> ...] [Top <height> <color>] [AddPrev]
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
	Imlib_Color_Range range;
	double angle;
	unsigned int top_height;
	Imlib_Color top_color;
	bool add_prev;
	double *values;
} areachart_var_t;

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
} areachart_t;

/******************************************************************************/

static void render_areachart(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	xsg_debug("render_areachart");
	/* TODO */
}

static void update_areachart(xsg_widget_t *widget, uint32_t var_id) {
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	xsg_list_t *l;
	double prev = 0.0;
	unsigned int i;

	areachart = (areachart_t *) widget->data;
	i = areachart->value_index;
	for (l = areachart->var_list; l ; l = l->next) {
		areachart_var = l->data;

		if ((var_id == 0xffffffff) || (areachart_var->var_id == var_id)) {
			areachart_var->values[i] = xsg_var_get_double(areachart_var->var_id);
			if (areachart_var->add_prev)
				areachart_var->values[i] += prev;
		}
		prev = areachart_var->values[i];
	}
}

static void scroll_areachart(xsg_widget_t *widget) {
	areachart_t *areachart;
	unsigned int width;

	areachart = (areachart_t *) widget->data;

	if (areachart->angle)
		width = areachart->angle->width;
	else
		width = widget->width;

	areachart->value_index = (areachart->value_index + 1) % width;
}

void xsg_widget_areachart_parse(uint64_t *update, uint32_t *widget_id) {
	xsg_widget_t *widget;
	areachart_t *areachart;

	widget = xsg_new0(xsg_widget_t, 1);
	areachart = xsg_new(areachart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_areachart;
	widget->update_func = update_areachart;
	widget->scroll_func = scroll_areachart;
	widget->data = (void *) areachart;

	areachart->angle = NULL;
	areachart->min = 0.0;
	areachart->max = 0.0;
	areachart->const_min = FALSE;
	areachart->const_max = FALSE;
	areachart->background = NULL;
	areachart->var_list = NULL;
	areachart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			areachart->angle = xsg_angle_parse(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Min")) {
			areachart->min = xsg_conf_read_double();
			areachart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			areachart->max = xsg_conf_read_double();
			areachart->const_max = TRUE;
		} else if (xsg_conf_find_command("Background")) {
			char *filename = xsg_conf_read_string();
			areachart->background = xsg_imlib_load_image(filename);
			if (unlikely(areachart->background == NULL))
				xsg_error("Cannot load image \"%s\"", filename);
			xsg_free(filename);
		} else {
			xsg_conf_error("Angle, Min, Max or Background");
		}
	}

	*update = widget->update;
	*widget_id = xsg_widgets_add(widget);
}

void xsg_widget_areachart_parse_var(uint32_t var_id) {
	xsg_widget_t *widget;
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	unsigned int width, i;

	if (!xsg_var_is_double(var_id))
		xsg_error("AreaChart allows double vars only");

	widget = xsg_widgets_last();
	areachart = widget->data;
	areachart_var = xsg_new0(areachart_var_t, 1);
	areachart->var_list = xsg_list_append(areachart->var_list, areachart_var);

	if (areachart->angle)
		width = areachart->angle->width;
	else
		width = widget->width;

	areachart_var->var_id = var_id;
	areachart_var->color = xsg_imlib_uint2color(xsg_conf_read_color());
	areachart_var->range = NULL;
	areachart_var->angle = 0.0;
	areachart_var->top_height = 0;
	areachart_var->add_prev = FALSE;
	areachart_var->values = xsg_new0(double, width);

	for (i = 0; i < width; i++)
		areachart_var->values[i] = DNAN;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;
			areachart_var->range = imlib_create_color_range();
			imlib_context_set_color_range(areachart_var->range);
			imlib_context_set_color(
					areachart_var->color.red,
					areachart_var->color.green,
					areachart_var->color.blue,
					areachart_var->color.alpha);
			imlib_add_color_to_color_range(0);
			areachart_var->angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = xsg_imlib_uint2color(xsg_conf_read_color());
				imlib_context_set_color(
						color.red,
						color.green,
						color.blue,
						color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("Top")) {
			areachart_var->top_height = xsg_conf_read_uint();
			areachart_var->top_color = xsg_imlib_uint2color(xsg_conf_read_color());
		} else if (xsg_conf_find_command("AddPrev")) {
			areachart_var->add_prev = TRUE;
		} else {
			xsg_conf_error("ColorRange, Top or AddPrev");
		}
	}
}


