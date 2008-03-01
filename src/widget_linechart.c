/* widget_linechart.c
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
#include <math.h>
#include <float.h>

#include "widgets.h"
#include "widget.h"
#include "window.h"
#include "angle.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	xsg_var_t *var;
	Imlib_Color color;
	double *values;
} linechart_var_t;

/******************************************************************************/

typedef struct {
	xsg_angle_t *angle;
	double min;
	double max;
	xsg_var_t *min_var;
	xsg_var_t *max_var;
	char *background;
	xsg_list_t *var_list;
	unsigned int value_index;
} linechart_t;

/******************************************************************************/

static void
render_linechart(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y)
{
	linechart_t *linechart;
	xsg_list_t *l;

	linechart = widget->data;

	xsg_debug("%s: render LineChart: x=%d, y=%d, w=%u, h=%u",
			xsg_window_get_config_name(widget->window),
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);

	if ((linechart->angle == NULL) || (linechart->angle->angle == 0.0)) {
		int xoffset, yoffset;
		double pixel_mult;

		if (linechart->background) {
			xsg_imlib_blend_background(linechart->background,
					widget->xoffset - up_x,
					widget->yoffset - up_y,
					widget->width, widget->height,
					0, widget->update);
		}

		pixel_mult = ((double) widget->height - 1.0)
			/ (linechart->max - linechart->min);

		imlib_context_set_image(buffer);
		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var_t *linechart_var = l->data;
			ImlibPolygon poly = imlib_polygon_new();
			unsigned int point_count = 0;

			imlib_context_set_color(linechart_var->color.red,
					linechart_var->color.green,
					linechart_var->color.blue,
					linechart_var->color.alpha);

			for (xoffset = 0; xoffset < widget->width; xoffset++) {
				int value_index;
				double value;

				value_index = (xoffset + linechart->value_index + 1)
						% widget->width;
				value = linechart_var->values[value_index];

				if (isnan(value)) {
					if (point_count > 0) {
						imlib_image_draw_polygon(poly, 0);
						imlib_polygon_free(poly);
						poly = imlib_polygon_new();
						point_count = 0;
					}
					continue;
				}

				yoffset = lround(pixel_mult *
						(linechart->max - value));

				imlib_polygon_add_point(poly,
						widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y + yoffset);
				point_count++;
			}

			if (point_count > 0) {
				imlib_image_draw_polygon(poly, 0);
			}

			imlib_polygon_free(poly);
		}

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (linechart->angle->angle == 90.0) {
		int xoffset, yoffset;
		double pixel_mult;

		if (linechart->background) {
			xsg_imlib_blend_background(linechart->background,
					widget->xoffset - up_x,
					widget->yoffset - up_y,
					widget->width, widget->height,
					1, widget->update);
		}

		pixel_mult = ((double) widget->width - 1.0)
			/ (linechart->max - linechart->min);

		imlib_context_set_image(buffer);
		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var_t *linechart_var = l->data;
			ImlibPolygon poly = imlib_polygon_new();
			unsigned int point_count = 0;

			imlib_context_set_color(linechart_var->color.red,
					linechart_var->color.green,
					linechart_var->color.blue,
					linechart_var->color.alpha);

			for (yoffset = 0; yoffset < widget->height; yoffset++) {
				int value_index;
				double value;

				value_index = (yoffset + linechart->value_index + 1)
						% widget->height;
				value = linechart_var->values[value_index];

				if (isnan(value)) {
					if (point_count > 0) {
						imlib_image_draw_polygon(poly, 0);
						imlib_polygon_free(poly);
						poly = imlib_polygon_new();
						point_count = 0;
					}
					continue;
				}

				xoffset = widget->width - lround(pixel_mult
						* (linechart->max - value)) - 1;

				imlib_polygon_add_point(poly,
						widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y + yoffset);
				point_count++;
			}

			if (point_count > 0) {
				imlib_image_draw_polygon(poly, 0);
			}

			imlib_polygon_free(poly);
		}

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (linechart->angle->angle == 180.0) {
		int xoffset, yoffset;
		double pixel_mult;

		if (linechart->background) {
			xsg_imlib_blend_background(linechart->background,
					widget->xoffset - up_x,
					widget->yoffset - up_y,
					widget->width, widget->height,
					2, widget->update);
		}

		pixel_mult = ((double) widget->height - 1.0)
			/ (linechart->max - linechart->min);

		imlib_context_set_image(buffer);
		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var_t *linechart_var = l->data;
			ImlibPolygon poly = imlib_polygon_new();
			unsigned int point_count = 0;

			imlib_context_set_color(linechart_var->color.red,
					linechart_var->color.green,
					linechart_var->color.blue,
					linechart_var->color.alpha);

			for (xoffset = 0; xoffset < widget->width; xoffset++) {
				int value_index;
				double value;

				value_index = (linechart->value_index - xoffset + widget->width)
						% widget->width;
				value = linechart_var->values[value_index];

				if (isnan(value)) {
					if (point_count > 0) {
						imlib_image_draw_polygon(poly, 0);
						imlib_polygon_free(poly);
						poly = imlib_polygon_new();
						point_count = 0;
					}
					continue;
				}

				yoffset = widget->height - lround(pixel_mult
						* (linechart->max - value)) - 1;

				imlib_polygon_add_point(poly,
						widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y + yoffset);
				point_count++;
			}

			if (point_count > 0) {
				imlib_image_draw_polygon(poly, 0);
			}

			imlib_polygon_free(poly);
		}

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (linechart->angle->angle == 270.0) {
		int xoffset, yoffset;
		double pixel_mult;

		if (linechart->background) {
			xsg_imlib_blend_background(linechart->background,
					widget->xoffset - up_x,
					widget->yoffset - up_y,
					widget->width, widget->height,
					3, widget->update);
		}

		pixel_mult = ((double) widget->width - 1.0)
			/ (linechart->max - linechart->min);

		imlib_context_set_image(buffer);
		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var_t *linechart_var = l->data;
			ImlibPolygon poly = imlib_polygon_new();
			unsigned int point_count = 0;

			imlib_context_set_color(linechart_var->color.red,
					linechart_var->color.green,
					linechart_var->color.blue,
					linechart_var->color.alpha);

			for (yoffset = 0; yoffset < widget->height; yoffset++) {
				int value_index;
				double value;

				value_index = (linechart->value_index - yoffset + widget->height)
						% widget->height;
				value = linechart_var->values[value_index];

				if (isnan(value)) {
					if (point_count > 0) {
						imlib_image_draw_polygon(poly, 0);
						imlib_polygon_free(poly);
						poly = imlib_polygon_new();
						point_count = 0;
					}
					continue;
				}

				xoffset = lround(pixel_mult
						* (linechart->max - value));

				imlib_polygon_add_point(poly,
						widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y + yoffset);
				point_count++;
			}

			if (point_count > 0) {
				imlib_image_draw_polygon(poly, 0);
			}

			imlib_polygon_free(poly);
		}

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else {
		int xoffset, yoffset;
		double pixel_mult;
		unsigned int chart_width, chart_height;
		Imlib_Image tmp;

		chart_width = linechart->angle->width;
		chart_height = linechart->angle->height;

		tmp = imlib_create_image(chart_width, chart_height);
		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear();

		if (linechart->background) {
			xsg_imlib_blend_background(linechart->background,
					0, 0, chart_width, chart_height,
					0, widget->update);
		}

		pixel_mult = ((double) chart_height - 1.0)
			/ (linechart->max - linechart->min);

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var_t *linechart_var = l->data;
			ImlibPolygon poly = imlib_polygon_new();
			unsigned int point_count = 0;

			imlib_context_set_color(linechart_var->color.red,
					linechart_var->color.green,
					linechart_var->color.blue,
					linechart_var->color.alpha);

			for (xoffset = 0; xoffset < chart_width; xoffset++) {
				int value_index;
				double value;

				value_index = (xoffset + linechart->value_index + 1)
						% chart_width;
				value = linechart_var->values[value_index];

				if (isnan(value)) {
					if (point_count > 0) {
						imlib_image_draw_polygon(poly, 0);
						imlib_polygon_free(poly);
						poly = imlib_polygon_new();
						point_count = 0;
					}
					continue;
				}

				yoffset = lround(pixel_mult
						* (linechart->max - value));

				imlib_polygon_add_point(poly, xoffset, yoffset);
				point_count++;
			}

			if (point_count > 0) {
				imlib_image_draw_polygon(poly, 0);
			}

			imlib_polygon_free(poly);
		}

		imlib_context_set_image(buffer);
		imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0,
				chart_width, chart_height,
				linechart->angle->xoffset - up_x,
				linechart->angle->yoffset - up_y,
				linechart->angle->angle_x,
				linechart->angle->angle_y);

		imlib_context_set_image(tmp);
		imlib_free_image();
	}
}

static void
update_linechart(xsg_widget_t *widget, xsg_var_t *var)
{
	linechart_t *linechart;
	linechart_var_t *linechart_var;
	xsg_list_t *l;
	unsigned int i, count;
	bool dirty = FALSE;

	linechart = (linechart_t *) widget->data;

	if (linechart->min_var) {
		if ((var == NULL) || (linechart->min_var == var)) {
			double value = linechart->min;

			linechart->min = xsg_var_get_num(linechart->min_var);
			if (value != linechart->min) {
				dirty = TRUE;
			}
		}
	}

	if (linechart->max_var) {
		if ((var == NULL) || (linechart->max_var == var)) {
			double value = linechart->max;

			linechart->max = xsg_var_get_num(linechart->max_var);
			if (value != linechart->max) {
				dirty = TRUE;
			}
		}
	}

	i = linechart->value_index;
	for (l = linechart->var_list; l; l = l->next) {
		linechart_var = l->data;

		if ((var == NULL) || (linechart_var->var == var)) {
			double value = linechart_var->values[i];

			linechart_var->values[i] = xsg_var_get_num(
					linechart_var->var);
			if (value != linechart_var->values[i]) {
				dirty = TRUE;
			}
		}
	}

	if (!dirty) {
		return;
	}

	xsg_window_update_append_rect(widget->window,
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);

	if (linechart->min_var && linechart->max_var) {
		return;
	}

	if (linechart->angle) {
		count = linechart->angle->width;
	} else {
		count = widget->width;
	}

	if (linechart->min_var) {
		linechart->max = - DBL_MAX;

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;

			for (i = 0; i < count; i++) {
				if (!isnan(linechart_var->values[i])) {
					linechart->max = MAX(linechart->max,
						linechart_var->values[i]);
				}
			}
		}
	} else if (linechart->max_var) {
		linechart->min = DBL_MAX;

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;

			for (i = 0; i < count; i++) {
				if (!isnan(linechart_var->values[i])) {
					linechart->min = MIN(linechart->min,
						linechart_var->values[i]);
				}
			}
		}
	} else {
		linechart->min = DBL_MAX;
		linechart->max = - DBL_MAX;

		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;

			for (i = 0; i < count; i++) {
				if (!isnan(linechart_var->values[i])) {
					linechart->min = MIN(linechart->min,
						linechart_var->values[i]);
					linechart->max = MAX(linechart->max,
						linechart_var->values[i]);
				}
			}
		}
	}
}

static void
scroll_linechart(xsg_widget_t *widget)
{
	linechart_t *linechart;
	unsigned int width;

	linechart = (linechart_t *) widget->data;

	if (linechart->angle) {
		width = linechart->angle->width;
	} else {
		width = widget->width;
	}

	linechart->value_index = (linechart->value_index + 1) % width;

	xsg_window_update_append_rect(widget->window,
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);
}

/******************************************************************************/

static void
parse_var(xsg_widget_t *widget, xsg_var_t *var)
{
	linechart_t *linechart;
	linechart_var_t * linechart_var;
	unsigned int width, i;

	linechart = widget->data;
	linechart_var = xsg_new(linechart_var_t, 1);
	linechart->var_list = xsg_list_append(linechart->var_list, linechart_var);

	if (linechart->angle) {
		width = linechart->angle->width;
	} else {
		width = widget->width;
	}

	linechart_var->var = var;
	linechart_var->color = xsg_imlib_uint2color(xsg_conf_read_color());
	linechart_var->values = xsg_new(double, width);

	for (i = 0; i < width; i++) {
		linechart_var->values[i] = DNAN;
	}

	xsg_conf_read_newline();
}

void
xsg_widget_linechart_parse(xsg_window_t *window)
{
	xsg_widget_t *widget;
	linechart_t *linechart;
	double angle = 0.0;

	widget = xsg_widgets_new(window);

	linechart = xsg_new(linechart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_linechart;
	widget->update_func = update_linechart;
	widget->scroll_func = scroll_linechart;
	widget->data = (void *) linechart;

	linechart->angle = NULL;
	linechart->min = 0.0;
	linechart->max = 0.0;
	linechart->min_var = NULL;
	linechart->max_var = NULL;
	linechart->background = NULL;
	linechart->var_list = NULL;
	linechart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_uint();
			widget->visible_var = xsg_var_parse(
					widget->visible_update, window, widget);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Min")) {
			linechart->min_var = xsg_var_parse(widget->update,
					window, widget);
		} else if (xsg_conf_find_command("Max")) {
			linechart->max_var = xsg_var_parse(widget->update,
					window, widget);
		} else if (xsg_conf_find_command("Background")) {
			if (linechart->background != NULL) {
				xsg_free(linechart->background);
			}
			linechart->background = xsg_conf_read_string();
		} else {
			xsg_conf_error("Visible, Angle, Min, Max or Background "
					"expected");
		}
	}

	if (angle != 0.0) {
		linechart->angle = xsg_angle_parse(angle,
				widget->xoffset, widget->yoffset,
				widget->width, widget->height);
	}

	while (xsg_conf_find_command("+")) {
		parse_var(widget, xsg_var_parse(widget->update, window, widget));
	}
}

