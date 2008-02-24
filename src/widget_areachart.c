/* widget_areachart.c
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
#include <alloca.h>

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
	Imlib_Color_Range range;
	double range_angle;
	unsigned int top_height;
	Imlib_Color *top_colors;
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
	char *background;
	xsg_list_t *var_list;
	unsigned int value_index;
} areachart_t;

/******************************************************************************/

static void
render_areachart(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y)
{
	areachart_t *areachart;
	xsg_list_t *l;

	areachart = widget->data;

	xsg_debug("%s: render AreaChart: x=%d, y=%d, w=%u, h=%u",
			xsg_window_get_config_name(widget->window),
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);

	if ((areachart->angle == NULL) || (areachart->angle->angle == 0.0)) {
		int xoffset, yoffset;
		int *prev_pos_h, *prev_neg_h;
		double *prev_pos, *prev_neg;
		double pixel_mult;

		if (areachart->background)
			xsg_imlib_blend_background(areachart->background, widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height, 0, widget->update);

		pixel_mult = ((double) widget->height) / (areachart->max - areachart->min);

		prev_pos_h = alloca(widget->width * sizeof(int));
		prev_neg_h = alloca(widget->width * sizeof(int));
		prev_pos = alloca(widget->width * sizeof(double));
		prev_neg = alloca(widget->width * sizeof(double));

		for (xoffset = 0; xoffset < widget->width; xoffset++) {
			prev_pos_h[xoffset] = 0;
			prev_neg_h[xoffset] = 0;
			prev_pos[xoffset] = 0.0;
			prev_neg[xoffset] = 0.0;
		}

		for (l = areachart->var_list; l; l = l->next) {
			areachart_var_t *areachart_var = l->data;
			Imlib_Image range_img = NULL;

			if (areachart_var->range != NULL)
				range_img = xsg_imlib_create_color_range_image(widget->width, widget->height,
						areachart_var->range, areachart_var->range_angle);

			imlib_context_set_image(buffer);
			imlib_context_set_cliprect(widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);

			for (xoffset = 0; xoffset < widget->width; xoffset++) {
				int value_index, height;
				double value;
				unsigned int i;

				value_index = ((areachart->value_index - xoffset + widget->width) % widget->width);
				value = areachart_var->values[value_index];

				if (value > 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (value + prev_pos[value_index]))
								- prev_pos_h[value_index];
						yoffset = (int) (areachart->max * pixel_mult) - height
								- prev_pos_h[value_index];
						prev_pos_h[value_index] += height;
						prev_pos[value_index] += value;
					} else {
						height = (int) (pixel_mult * value);
						yoffset = (int) (areachart->max * pixel_mult) - height;
						prev_pos_h[value_index] = height;
						prev_pos[value_index] = value;
					}
				} else if (value < 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (-value + prev_neg[value_index]))
								- prev_neg_h[value_index];
						yoffset = (int) (areachart->max * pixel_mult)
								+ prev_neg_h[value_index];
						prev_neg_h[value_index] += height;
						prev_neg[value_index] += -value;
					} else {
						height = (int) (pixel_mult * -value);
						yoffset = (int) (areachart->max * pixel_mult);
						prev_neg_h[value_index] = height;
						prev_neg[value_index] = -value;
					}
				} else {
					if (!areachart_var->add_prev) {
						prev_pos_h[value_index] = 0;
						prev_neg_h[value_index] = 0;
						prev_pos[value_index] = 0.0;
						prev_neg[value_index] = 0.0;
					}
					continue;
				}

				for (i = 0; i < areachart_var->top_height && height > 0; i++) {
					Imlib_Color *color = &areachart_var->top_colors[i];
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_pixel(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, 0);
					yoffset++;
					height--;
				}

				if (height <= 0)
					continue;

				if (range_img == NULL) {
					Imlib_Color *color = &areachart_var->color;
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, 1, height);
				} else {
					imlib_blend_image_onto_image(range_img, 1, xoffset, yoffset, 1, height,
							widget->xoffset - up_x + xoffset, widget->yoffset - up_y + yoffset,
							1, height);
				}
			}

			imlib_context_set_cliprect(0, 0, 0, 0);

			if (range_img != NULL) {
				imlib_context_set_image(range_img);
				imlib_free_image();
			}
		}
	} else if (areachart->angle->angle == 90.0) {
		int xoffset, yoffset;
		int *prev_pos_w, *prev_neg_w;
		double *prev_pos, *prev_neg;
		double pixel_mult;

		if (areachart->background)
			xsg_imlib_blend_background(areachart->background, widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, 1, widget->update);

		pixel_mult = ((double) widget->width) / (areachart->max - areachart->min);

		prev_pos_w = alloca(widget->height * sizeof(int));
		prev_neg_w = alloca(widget->height * sizeof(int));
		prev_pos = alloca(widget->height * sizeof(double));
		prev_neg = alloca(widget->height * sizeof(double));

		for (yoffset = 0; yoffset < widget->height; yoffset++) {
			prev_pos_w[yoffset] = 0;
			prev_neg_w[yoffset] = 0;
			prev_pos[yoffset] = 0.0;
			prev_neg[yoffset] = 0.0;
		}

		for (l = areachart->var_list; l; l = l->next) {
			areachart_var_t *areachart_var = l->data;
			Imlib_Image range_img = NULL;

			if (areachart_var->range)
				range_img = xsg_imlib_create_color_range_image(widget->width, widget->height,
						areachart_var->range, areachart_var->range_angle + 90.0);

			imlib_context_set_image(buffer);
			imlib_context_set_cliprect(widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);

			for (yoffset = 0; yoffset < widget->height; yoffset++) {
				int value_index, width;
				double value;
				unsigned int i;

				value_index = ((areachart->value_index - yoffset + widget->height) % widget->height);
				value = areachart_var->values[value_index];

				if (value > 0.0) {
					if (areachart_var->add_prev) {
						width = (int) (pixel_mult * (value + prev_pos[value_index]))
								- prev_pos_w[value_index];
						xoffset = widget->width - (int) (areachart->max * pixel_mult)
								+ prev_pos_w[value_index];
						prev_pos_w[value_index] += width;
						prev_pos[value_index] += value;
					} else {
						width = (int) (pixel_mult * value);
						xoffset = widget->width - (int) (areachart->max * pixel_mult);
						prev_pos_w[value_index] = width;
						prev_pos[value_index] = value;
					}
				} else if (value < 0.0) {
					if (areachart_var->add_prev) {
						width = (int) (pixel_mult * (-value + prev_neg[value_index]))
								- prev_neg_w[value_index];
						xoffset = widget->width - (int) (areachart->max * pixel_mult) - width
								- prev_neg_w[value_index];
						prev_neg_w[value_index] += width;
						prev_neg[value_index] += -value;
					} else {
						width = (int) (pixel_mult * -value);
						xoffset = widget->width - (int) (areachart->max * pixel_mult) - width;
						prev_neg_w[value_index] = width;
						prev_neg[value_index] = -value;
					}
				} else {
					if (!areachart_var->add_prev) {
						prev_pos_w[value_index] = 0;
						prev_neg_w[value_index] = 0;
						prev_pos[value_index] = 0.0;
						prev_neg[value_index] = 0.0;
					}
					continue;
				}

				for (i = 0; i < areachart_var->top_height && width > 0; i++) {
					Imlib_Color *color = &areachart_var->top_colors[i];
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_pixel(widget->xoffset - up_x + xoffset + width - 1,
							widget->yoffset - up_y + yoffset, 0);
					width--;
				}

				if (width <= 0)
					continue;

				if (range_img == NULL) {
					Imlib_Color *color = &areachart_var->color;
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, width, 1);
				} else {
					imlib_blend_image_onto_image(range_img, 1, xoffset, yoffset, width, 1,
							widget->xoffset - up_x + xoffset, widget->yoffset - up_y + yoffset,
							width, 1);
				}
			}

			imlib_context_set_cliprect(0, 0, 0, 0);

			if (range_img != NULL) {
				imlib_context_set_image(range_img);
				imlib_free_image();
			}
		}
	} else if (areachart->angle->angle == 180.0) {
		int xoffset, yoffset;
		int *prev_pos_h, *prev_neg_h;
		double *prev_pos, *prev_neg;
		double pixel_mult;

		if (areachart->background)
			xsg_imlib_blend_background(areachart->background, widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, 2, widget->update);

		pixel_mult = ((double) widget->height) / (areachart->max - areachart->min);

		prev_pos_h = alloca(widget->width * sizeof(int));
		prev_neg_h = alloca(widget->width * sizeof(int));
		prev_pos = alloca(widget->width * sizeof(double));
		prev_neg = alloca(widget->width * sizeof(double));

		for (xoffset = 0; xoffset < widget->width; xoffset++) {
			prev_pos_h[xoffset] = 0;
			prev_neg_h[xoffset] = 0;
			prev_pos[xoffset] = 0.0;
			prev_neg[xoffset] = 0.0;
		}

		for (l = areachart->var_list; l; l = l->next) {
			areachart_var_t *areachart_var = l->data;
			Imlib_Image range_img = NULL;

			if (areachart_var->range != NULL)
				range_img = xsg_imlib_create_color_range_image(widget->width, widget->height,
						areachart_var->range, areachart_var->range_angle + 180.0);

			imlib_context_set_image(buffer);
			imlib_context_set_cliprect(widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);

			for (xoffset = 0; xoffset < widget->width; xoffset++) {
				int value_index, height;
				double value;
				unsigned int i;

				value_index = ((xoffset + areachart->value_index + 1) % widget->width);
				value = areachart_var->values[value_index];

				if (value > 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (value + prev_pos[value_index]))
								- prev_pos_h[value_index];
						yoffset = widget->height - (int) (areachart->max * pixel_mult)
								+ prev_pos_h[value_index];
						prev_pos_h[value_index] += height;
						prev_pos[value_index] += value;
					} else {
						height = (int) (pixel_mult * value);
						yoffset = widget->height - (int) (areachart->max * pixel_mult);
						prev_pos_h[value_index] = height;
						prev_pos[value_index] = value;
					}
				} else if (value < 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (-value + prev_neg[value_index]))
								- prev_neg_h[value_index];
						yoffset = widget->height - (int) (areachart->max * pixel_mult) - height
								- prev_neg_h[value_index];
						prev_neg_h[value_index] += height;
						prev_neg[value_index] += -value;
					} else {
						height = (int) (pixel_mult * -value);
						yoffset = widget->height - (int) (areachart->max * pixel_mult) - height;
						prev_neg_h[value_index] = height;
						prev_neg[value_index] = -value;
					}
				} else {
					if (!areachart_var->add_prev) {
						prev_pos_h[value_index] = 0;
						prev_neg_h[value_index] = 0;
						prev_pos[value_index] = 0.0;
						prev_neg[value_index] = 0.0;
					}
					continue;
				}

				for (i = 0; i < areachart_var->top_height && height > 0; i++) {
					Imlib_Color *color = &areachart_var->top_colors[i];
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_pixel(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset + height - 1, 0);
					height--;
				}

				if (height <= 0)
					continue;

				if (range_img == NULL) {
					Imlib_Color *color = &areachart_var->color;
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, 1, height);
				} else {
					imlib_blend_image_onto_image(range_img, 1, xoffset, yoffset, 1, height,
							widget->xoffset - up_x + xoffset, widget->yoffset - up_y + yoffset,
							1, height);
				}
			}

			imlib_context_set_cliprect(0, 0, 0, 0);

			if (range_img != NULL) {
				imlib_context_set_image(range_img);
				imlib_free_image();
			}
		}
	} else if (areachart->angle->angle == 270.0) {
		int xoffset, yoffset;
		int *prev_pos_w, *prev_neg_w;
		double *prev_pos, *prev_neg;
		double pixel_mult;

		if (areachart->background)
			xsg_imlib_blend_background(areachart->background, widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, 3, widget->update);

		pixel_mult = ((double) widget->width) / (areachart->max - areachart->min);

		prev_pos_w = alloca(widget->height * sizeof(int));
		prev_neg_w = alloca(widget->height * sizeof(int));
		prev_pos = alloca(widget->height * sizeof(double));
		prev_neg = alloca(widget->height * sizeof(double));

		for (yoffset = 0; yoffset < widget->height; yoffset++) {
			prev_pos_w[yoffset] = 0;
			prev_neg_w[yoffset] = 0;
			prev_pos[yoffset] = 0.0;
			prev_neg[yoffset] = 0.0;
		}

		for (l = areachart->var_list; l; l = l->next) {
			areachart_var_t *areachart_var = l->data;
			Imlib_Image range_img = NULL;

			if (areachart_var->range != NULL)
				range_img = xsg_imlib_create_color_range_image(widget->width, widget->height,
						areachart_var->range, areachart_var->range_angle + 270.0);

			imlib_context_set_image(buffer);
			imlib_context_set_cliprect(widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);

			for (yoffset = 0; yoffset < widget->height; yoffset++) {
				int value_index, width;
				double value;
				unsigned int i;

				value_index = ((yoffset + areachart->value_index + 1) % widget->height);
				value = areachart_var->values[value_index];

				if (value > 0.0) {
					if (areachart_var->add_prev) {
						width = (int) (pixel_mult * (value + prev_pos[value_index]))
							- prev_pos_w[value_index];
						xoffset = (int) (areachart->max * pixel_mult) - width
							- prev_pos_w[value_index];
						prev_pos_w[value_index] += width;
						prev_pos[value_index] += value;
					} else {
						width = (int) (pixel_mult * value);
						xoffset = (int) (areachart->max * pixel_mult) - width;
						prev_pos_w[value_index] = width;
						prev_pos[value_index] = value;
					}
				} else if (value < 0.0) {
					if (areachart_var->add_prev) {
						width = (int) (pixel_mult * (-value + prev_neg[value_index]))
								- prev_neg_w[value_index];
						xoffset = (int) (areachart->max * pixel_mult)
								+ prev_neg_w[value_index];
						prev_neg_w[value_index] += width;
						prev_neg[value_index] += -value;
					} else {
						width = (int) (pixel_mult * -value);
						xoffset = (int) (areachart->max * pixel_mult);
						prev_neg_w[value_index] = width;
						prev_neg[value_index] = -value;
					}
				} else {
					if (!areachart_var->add_prev) {
						prev_pos_w[value_index] = 0;
						prev_neg_w[value_index] = 0;
						prev_pos[value_index] = 0.0;
						prev_neg[value_index] = 0.0;
					}
					continue;
				}

				for (i = 0; i < areachart_var->top_height && width > 0; i++) {
					Imlib_Color *color = &areachart_var->top_colors[i];
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_pixel(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, 0);
					xoffset++;
					width--;
				}

				if (width <= 0)
					continue;

				if (range_img == NULL) {
					Imlib_Color *color = &areachart_var->color;
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y + yoffset, width, 1);
				} else {
					imlib_blend_image_onto_image(range_img, 1, xoffset, yoffset, width, 1,
							widget->xoffset - up_x + xoffset, widget->yoffset - up_y + yoffset,
							width, 1);
				}
			}

			imlib_context_set_cliprect(0, 0, 0, 0);

			if (range_img != NULL) {
				imlib_context_set_image(range_img);
				imlib_free_image();
			}
		}
	} else {
		int xoffset, yoffset;
		int *prev_pos_h, *prev_neg_h;
		double *prev_pos, *prev_neg;
		double pixel_mult;
		unsigned int chart_width, chart_height;
		Imlib_Image tmp;

		chart_width = areachart->angle->width;
		chart_height = areachart->angle->height;

		tmp = imlib_create_image(chart_width, chart_height);
		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear();

		if (areachart->background)
			xsg_imlib_blend_background(areachart->background, 0, 0, chart_width, chart_height, 0, widget->update);

		pixel_mult = ((double) chart_height) / (areachart->max - areachart->min);

		prev_pos_h = alloca(chart_width * sizeof(int));
		prev_neg_h = alloca(chart_width * sizeof(int));
		prev_pos = alloca(chart_width * sizeof(double));
		prev_neg = alloca(chart_width * sizeof(double));

		for (xoffset = 0; xoffset < chart_width; xoffset++) {
			prev_pos_h[xoffset] = 0;
			prev_neg_h[xoffset] = 0;
			prev_pos[xoffset] = 0.0;
			prev_neg[xoffset] = 0.0;
		}

		for (l = areachart->var_list; l; l = l->next) {
			areachart_var_t *areachart_var = l->data;
			Imlib_Image range_img = NULL;

			if (areachart_var->range != NULL)
				range_img = xsg_imlib_create_color_range_image(chart_width, chart_height,
						areachart_var->range, areachart_var->range_angle);

			imlib_context_set_image(tmp);

			for (xoffset = 0; xoffset < chart_width; xoffset++) {
				int value_index, height;
				double value;
				unsigned int i;

				value_index = ((areachart->value_index - xoffset + chart_width) % chart_width);
				value = areachart_var->values[value_index];

				if (value > 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (value + prev_pos[value_index]))
								- prev_pos_h[value_index];
						yoffset = (int) (areachart->max * pixel_mult) - height
								- prev_pos_h[value_index];
						prev_pos_h[value_index] += height;
						prev_pos[value_index] += value;
					} else {
						height = (int) (pixel_mult * value);
						yoffset = (int) (areachart->max * pixel_mult) - height;
						prev_pos_h[value_index] = height;
						prev_pos[value_index] = value;
					}
				} else if (value < 0.0) {
					if (areachart_var->add_prev) {
						height = (int) (pixel_mult * (-value + prev_neg[value_index]))
								- prev_neg_h[value_index];
						yoffset = (int) (areachart->max * pixel_mult)
								+ prev_neg_h[value_index];
						prev_neg_h[value_index] += height;
						prev_neg[value_index] += -value;
					} else {
						height = (int) (pixel_mult * -value);
						yoffset = (int) (areachart->max * pixel_mult);
						prev_neg_h[value_index] = height;
						prev_neg[value_index] = -value;
					}
				} else {
					if (!areachart_var->add_prev) {
						prev_pos_h[value_index] = 0;
						prev_neg_h[value_index] = 0;
						prev_pos[value_index] = 0.0;
						prev_neg[value_index] = 0.0;
					}
					continue;
				}

				for (i = 0; i < areachart_var->top_height && height > 0; i++) {
					Imlib_Color *color = &areachart_var->top_colors[i];
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_pixel(xoffset, yoffset, 0);
					yoffset++;
					height--;
				}

				if (height <= 0)
					continue;

				if (range_img == NULL) {
					Imlib_Color *color = &areachart_var->color;
					imlib_context_set_color(color->red, color->green, color->blue, color->alpha);
					imlib_image_draw_rectangle(xoffset, yoffset, 1, height);
				} else {
					imlib_blend_image_onto_image(range_img, 1, xoffset, yoffset, 1, height,
							xoffset, yoffset, 1, height);
				}
			}

			if (range_img != NULL) {
				imlib_context_set_image(range_img);
				imlib_free_image();
			}
		}

		imlib_context_set_image(buffer);
		imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0, chart_width, chart_height,
				areachart->angle->xoffset - up_x, areachart->angle->yoffset - up_y,
				areachart->angle->angle_x, areachart->angle->angle_y);

		imlib_context_set_image(tmp);
		imlib_free_image();
	}
}

static void
update_areachart(xsg_widget_t *widget, xsg_var_t *var)
{
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	xsg_list_t *l;
	unsigned int i, count;

	/* TODO: dirty flag + smaller update rect */
	xsg_window_update_append_rect(widget->window, widget->xoffset, widget->yoffset, widget->width, widget->height);

	areachart = (areachart_t *) widget->data;

	i = areachart->value_index;
	for (l = areachart->var_list; l; l = l->next) {
		areachart_var = l->data;

		if ((var == 0) || (areachart_var->var == var)) {
			areachart_var->values[i] = xsg_var_get_num(areachart_var->var);
		}
	}

	if (areachart->const_min && areachart->const_max) {
		return;
	}

	if (areachart->angle) {
		count = areachart->angle->width;
	} else {
		count = widget->width;
	}

	if (!areachart->const_min && !areachart->const_max) {
		areachart->min = DBL_MAX;
		areachart->max = DBL_MIN;

		for (i = 0; i < count; i++) {
			double pos = 0.0;
			double neg = 0.0;

			for (l = areachart->var_list; l; l = l->next) {
				areachart_var = l->data;

				if (areachart_var->values[i] > 0.0) {
					if (areachart_var->add_prev)
						pos += areachart_var->values[i];
					else
						pos = areachart_var->values[i];
					areachart->min = MIN(areachart->min, pos);
					areachart->max = MAX(areachart->max, pos);
				} else if (areachart_var->values[i] < 0.0) {
					if (areachart_var->add_prev)
						neg += areachart_var->values[i];
					else
						neg = areachart_var->values[i];
					areachart->min = MIN(areachart->min, neg);
					areachart->max = MAX(areachart->max, neg);
				} else {
					if (!areachart_var->add_prev) {
						pos = 0.0;
						neg = 0.0;
					}
					areachart->min = MIN(areachart->min, 0.0);
					areachart->max = MAX(areachart->max, 0.0);
				}
			}
		}
	} else if (areachart->const_min) {
		areachart->max = DBL_MIN;

		for (i = 0;  i < count; i++) {
			double pos = 0.0;
			double neg = 0.0;

			for (l = areachart->var_list; l; l = l->next) {
				areachart_var = l->data;

				if (areachart_var->values[i] > 0.0) {
					if (areachart_var->add_prev)
						pos += areachart_var->values[i];
					else
						pos = areachart_var->values[i];
					areachart->max = MAX(areachart->max, pos);
				} else if (areachart_var->values[i] < 0.0) {
					if (areachart_var->add_prev)
						neg += areachart_var->values[i];
					else
						neg = areachart_var->values[i];
					areachart->max = MAX(areachart->max, neg);
				} else {
					if (!areachart_var->add_prev) {
						pos = 0.0;
						neg = 0.0;
					}
					areachart->max = MAX(areachart->max, 0.0);
				}
			}
		}
	} else if (areachart->const_max) {
		areachart->min = DBL_MAX;

		for (i = 0; i < count; i++) {
			double pos = 0.0;
			double neg = 0.0;

			for (l = areachart->var_list; l; l = l->next) {
				areachart_var = l->data;

				if (areachart_var->values[i] > 0.0) {
					if (areachart_var->add_prev)
						pos += areachart_var->values[i];
					else
						pos = areachart_var->values[i];
					areachart->min = MIN(areachart->min, pos);
				} else if (areachart_var->values[i] < 0.0) {
					if (areachart_var->add_prev)
						neg += areachart_var->values[i];
					else
						neg = areachart_var->values[i];
					areachart->min = MIN(areachart->min, neg);
				} else {
					if (!areachart_var->add_prev) {
						pos = 0.0;
						neg = 0.0;
					}
					areachart->min = MIN(areachart->min, 0.0);
				}
			}
		}
	}
}

static void
scroll_areachart(xsg_widget_t *widget)
{
	areachart_t *areachart;
	unsigned int width;

	areachart = (areachart_t *) widget->data;

	if (areachart->angle) {
		width = areachart->angle->width;
	} else {
		width = widget->width;
	}

	areachart->value_index = (areachart->value_index + 1) % width;

	xsg_window_update_append_rect(widget->window, widget->xoffset,
			widget->yoffset, widget->width, widget->height);
}

/******************************************************************************/

xsg_widget_t *
xsg_widget_areachart_parse(xsg_window_t *window, uint64_t *update)
{
	xsg_widget_t *widget;
	areachart_t *areachart;
	double angle = 0.0;

	widget = xsg_widgets_new(window);

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

	*update = widget->update;

	areachart->angle = NULL;
	areachart->min = 0.0;
	areachart->max = 0.0;
	areachart->const_min = FALSE;
	areachart->const_max = FALSE;
	areachart->background = NULL;
	areachart->var_list = NULL;
	areachart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_uint();
			widget->visible_var = xsg_var_parse(widget->visible_update, window, widget);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Min")) {
			areachart->min = xsg_conf_read_double();
			areachart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			areachart->max = xsg_conf_read_double();
			areachart->const_max = TRUE;
		} else if (xsg_conf_find_command("Background")) {
			if (areachart->background != NULL)
				xsg_free(areachart->background);
			areachart->background = xsg_conf_read_string();
		} else {
			xsg_conf_error("Visible, Angle, Min, Max or Background expected");
		}
	}

	if (angle != 0.0) {
		areachart->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, widget->width, widget->height);
	}

	return widget;
}

void
xsg_widget_areachart_parse_var(xsg_var_t *var)
{
	xsg_widget_t *widget;
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	unsigned int width, i;

	widget = xsg_widgets_last();
	areachart = widget->data;
	areachart_var = xsg_new(areachart_var_t, 1);
	areachart->var_list = xsg_list_append(areachart->var_list, areachart_var);

	if (areachart->angle) {
		width = areachart->angle->width;
	} else {
		width = widget->width;
	}

	areachart_var->var = var;
	areachart_var->color = xsg_imlib_uint2color(xsg_conf_read_color());
	areachart_var->range = NULL;
	areachart_var->range_angle = 0.0;
	areachart_var->top_height = 0;
	areachart_var->top_colors = NULL;
	areachart_var->add_prev = FALSE;
	areachart_var->values = xsg_new(double, width);

	for (i = 0; i < width; i++) {
		areachart_var->values[i] = DNAN;
	}

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;

			if (areachart_var->range != NULL) {
				imlib_context_set_color_range(areachart_var->range);
				imlib_free_color_range();
			}

			areachart_var->range = imlib_create_color_range();
			imlib_context_set_color_range(areachart_var->range);
			imlib_context_set_color(areachart_var->color.red, areachart_var->color.green,
					areachart_var->color.blue, areachart_var->color.alpha);
			imlib_add_color_to_color_range(0);
			areachart_var->range_angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = xsg_imlib_uint2color(xsg_conf_read_color());
				imlib_context_set_color(color.red, color.green, color.blue, color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("Top")) {
			unsigned int count, i;

			if (areachart_var->top_colors != NULL)
				xsg_free(areachart_var->top_colors);

			count = xsg_conf_read_uint();
			areachart_var->top_height = count;
			areachart_var->top_colors = xsg_new(Imlib_Color, count);

			for (i = 0; i < count; i++)
				areachart_var->top_colors[i] = xsg_imlib_uint2color(xsg_conf_read_color());

		} else if (xsg_conf_find_command("AddPrev")) {
			areachart_var->add_prev = TRUE;
		} else {
			xsg_conf_error("ColorRange, Top or AddPrev expected");
		}
	}
}


