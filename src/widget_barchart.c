/* widget_barchart.c
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
 * BarChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Mask <image>]
 * + <variable> <color> [ColorRange <angle> <count> <distance> <color> ...] [AddPrev]
 *
 */

/******************************************************************************/

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
	Imlib_Color_Range range;
	double range_angle;
	bool add_prev;
	double value;
} barchart_var_t;

/******************************************************************************/

typedef struct {
	xsg_angle_t *angle;
	double min;
	double max;
	bool const_min;
	bool const_max;
	Imlib_Image mask;
	xsg_list_t *var_list;
} barchart_t;

/******************************************************************************/

static void render_barchart(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y) {
	barchart_t *barchart;
	xsg_list_t *l;
	Imlib_Image mask_img;

	xsg_debug("%s: Render BarChart", xsg_window_get_config_name(widget->window));

	barchart = (barchart_t *) widget->data;

	if (barchart->min >= barchart->max)
		return;

	mask_img = NULL;
	if (barchart->mask) {
		mask_img = xsg_imlib_load_image(barchart->mask);

		if (unlikely(mask_img == NULL))
			xsg_warning("Cannot load image \"%s\"", barchart->mask);
	}

	if ((mask_img == NULL) && ((barchart->angle == NULL) || (barchart->angle->angle == 0.0))) {
		int yoffset;
		int prev_pos_h, prev_neg_h;
		double prev_pos, prev_neg;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->height) / (barchart->max - barchart->min);

		prev_pos_h = 0;
		prev_neg_h = 0;
		prev_pos = 0.0;
		prev_neg = 0.0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;
			double value;
			int height;

			barchart_var = l->data;

			value = barchart_var->value;

			if (value > 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (value + prev_pos)) - prev_pos_h;
					yoffset = (int) (barchart->max * pixel_mult) - height - prev_pos_h;
					prev_pos_h += height;
					prev_pos += value;
				} else {
					height = (int) (pixel_mult * value);
					yoffset = (int) (barchart->max * pixel_mult) - height;
					prev_pos_h = height;
					prev_pos = value;
				}
			} else if (value < 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (-value + prev_neg)) - prev_neg_h;
					yoffset = (int) (barchart->max * pixel_mult) + prev_neg_h;
					prev_neg_h += height;
					prev_neg += -value;
				} else {
					height = (int) (pixel_mult * -value);
					yoffset = (int) (barchart->max * pixel_mult);
					prev_neg_h = height;
					prev_neg = -value;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
					prev_pos = 0.0;
					prev_neg = 0.0;
				}
				continue;
			}

			if (yoffset < 0) {
				height += yoffset;
				yoffset = 0;
			}

			if ((yoffset + height) > widget->height)
				height -= (yoffset + height) - widget->height;

			if (height <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle;
				imlib_context_set_cliprect(widget->xoffset - up_x,
						widget->yoffset - up_y + yoffset, widget->width, height);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(widget->xoffset - up_x,
							widget->yoffset - up_y + yoffset, widget->width, height));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 90.0)) {
		int xoffset;
		int prev_pos_w, prev_neg_w;
		double prev_pos, prev_neg;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->width) / (barchart->max - barchart->min);

		prev_pos_w = 0;
		prev_neg_w = 0;
		prev_pos = 0.0;
		prev_neg = 0.0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;
			double value;
			int width;

			barchart_var = l->data;

			value = barchart_var->value;

			if (value > 0.0) {
				if (barchart_var->add_prev) {
					width = (int) (pixel_mult * (value + prev_pos)) - prev_pos_w;
					xoffset = widget->width - (int) (barchart->max * pixel_mult) + prev_pos_w;
					prev_pos_w += width;
					prev_pos += value;
				} else {
					width = (int) (pixel_mult * value);
					xoffset = widget->width - (int) (barchart->max * pixel_mult);
					prev_pos_w = width;
					prev_pos = value;
				}
			} else if (value < 0.0) {
				if (barchart_var->add_prev) {
					width = (int) (pixel_mult * (-value + prev_neg)) - prev_neg_w;
					xoffset = widget->width - (int) (barchart->max * pixel_mult) - width - prev_neg_w;
					prev_neg_w += width;
					prev_neg += -value;
				} else {
					width = (int) (pixel_mult * -value);
					xoffset = widget->width - (int) (barchart->max * pixel_mult) - width;
					prev_neg_w = width;
					prev_neg = -value;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_w = 0;
					prev_neg_w = 0;
					prev_pos = 0.0;
					prev_neg = 0.0;
				}
				continue;
			}

			if (xoffset < 0) {
				width += xoffset;
				xoffset = 0;
			}

			if ((xoffset + width) > widget->width)
				width -= (xoffset + width) - widget->width;

			if (width <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 90.0;
				imlib_context_set_cliprect(widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y, width, widget->height);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
							widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y, width, widget->height));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 180.0)) {
		int yoffset;
		int prev_pos_h, prev_neg_h;
		double prev_pos, prev_neg;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->height) / (barchart->max - barchart->min);

		prev_pos_h = 0;
		prev_neg_h = 0;
		prev_pos = 0.0;
		prev_neg = 0.0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;
			double value;
			int height;

			barchart_var = l->data;

			value = barchart_var->value;

			if (value > 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (value + prev_pos)) - prev_pos_h;
					yoffset = widget->height - (int) (barchart->max * pixel_mult) + prev_pos_h;
					prev_pos_h += height;
					prev_pos += value;
				} else {
					height = (int) (pixel_mult * value);
					yoffset = widget->height - (int) (barchart->max * pixel_mult);
					prev_pos_h = height;
					prev_pos = value;
				}
			} else if (value < 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (-value + prev_neg)) - prev_neg_h;
					yoffset = widget->height - (int) (barchart->max * pixel_mult) - height - prev_neg_h;
					prev_neg_h += height;
					prev_neg += -value;
				} else {
					height = (int) (pixel_mult * -value);
					yoffset = widget->height - (int) (barchart->max * pixel_mult) - height;
					prev_neg_h = height;
					prev_neg = -value;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
					prev_pos = 0.0;
					prev_neg = 0.0;
				}
				continue;
			}

			if (yoffset < 0) {
				height += yoffset;
				yoffset = 0;
			}

			if ((yoffset + height) > widget->height)
				height -= (yoffset + height) - widget->height;

			if (height <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 180.0;
				imlib_context_set_cliprect(widget->xoffset - up_x,
						widget->yoffset - up_y + yoffset, widget->width, height);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(widget->xoffset - up_x,
							widget->yoffset - up_y + yoffset, widget->width, height));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 270.0)) {
		int xoffset;
		int prev_pos_w, prev_neg_w;
		double prev_pos, prev_neg;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->width) / (barchart->max - barchart->min);

		prev_pos_w = 0;
		prev_neg_w = 0;
		prev_pos = 0.0;
		prev_neg = 0.0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;
			double value;
			int width;

			barchart_var = l->data;

			value = barchart_var->value;

			if (value > 0.0) {
				if (barchart_var->add_prev) {
					width = (int) (pixel_mult * (value + prev_pos)) - prev_pos_w;
					xoffset = (int) (barchart->max * pixel_mult) - width - prev_pos_w;
					prev_pos_w += width;
					prev_pos += value;
				} else {
					width = (int) (pixel_mult * value);
					xoffset = (int) (barchart->max * pixel_mult) - width;
					prev_pos_w = width;
					prev_pos = value;
				}
			} else if (value < 0.0) {
				if (barchart_var->add_prev) {
					width = (int) (pixel_mult * (-value + prev_neg)) - prev_neg_w;
					xoffset = (int) (barchart->max * pixel_mult) + prev_neg_w;
					prev_neg_w += width;
					prev_neg += -value;
				} else {
					width = (int) (pixel_mult * -value);
					xoffset = (int) (barchart->max * pixel_mult);
					prev_neg_w = width;
					prev_neg = -value;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_w = 0;
					prev_neg_w = 0;
					prev_pos = 0.0;
					prev_neg = 0.0;
				}
				continue;
			}

			if (xoffset < 0) {
				width += xoffset;
				xoffset = 0;
			}

			if ((xoffset + width) > widget->width)
				width -= (xoffset + width) - widget->width;

			if (width <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 270.0;
				imlib_context_set_cliprect(widget->xoffset - up_x + xoffset,
						widget->yoffset - up_y, width, widget->height);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
							widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(widget->xoffset - up_x + xoffset,
							widget->yoffset - up_y, width, widget->height));
			}
		}
	} else {
		int yoffset;
		int prev_pos_h, prev_neg_h;
		double prev_pos, prev_neg;
		double pixel_mult;
		unsigned int chart_width, chart_height;
		Imlib_Image tmp;

		if (barchart->angle) {
			chart_width = barchart->angle->width;
			chart_height = barchart->angle->height;
		} else {
			chart_width = widget->width;
			chart_height = widget->height;
		}

		tmp = imlib_create_image(chart_width, chart_height);
		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear();

		pixel_mult = ((double) chart_height) / (barchart->max - barchart->min);

		prev_pos_h = 0;
		prev_neg_h = 0;
		prev_pos = 0.0;
		prev_neg = 0.0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;
			double value;
			int height;

			barchart_var = l->data;

			value = barchart_var->value;

			if (value > 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (value + prev_pos)) - prev_pos_h;
					yoffset = (int) (barchart->max * pixel_mult) - height - prev_pos_h;
					prev_pos_h += height;
					prev_pos += value;
				} else {
					height = (int) (pixel_mult * value);
					yoffset = (int) (barchart->max * pixel_mult) - height;
					prev_pos_h = height;
					prev_pos = value;
				}
			} else if (value < 0.0) {
				if (barchart_var->add_prev) {
					height = (int) (pixel_mult * (-value + prev_neg)) - prev_neg_h;
					yoffset = (int) (barchart->max * pixel_mult) + prev_neg_h;
					prev_neg_h += height;
					prev_neg += -value;
				} else {
					height = (int) (pixel_mult * -value);
					yoffset = (int) (barchart->max * pixel_mult);
					prev_neg_h = height;
					prev_neg = -value;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
					prev_pos = 0.0;
					prev_neg = 0.0;
				}
				continue;
			}

			if (yoffset < 0) {
				height += yoffset;
				yoffset = 0;
			}

			if (yoffset + height > chart_height)
				height -= (yoffset + height) - chart_height;

			if (height <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle;
				imlib_context_set_cliprect(0, yoffset, chart_width, height);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(0, 0, chart_width, chart_height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(0, yoffset, chart_width, height));
			}
		}

		if (mask_img)
			T(xsg_imlib_blend_mask(mask_img));

		imlib_context_set_image(buffer);
		if (barchart->angle) {
			T(imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0, chart_width, chart_height,
					barchart->angle->xoffset - up_x, barchart->angle->yoffset - up_y,
					barchart->angle->angle_x, barchart->angle->angle_y));
		} else {
			T(imlib_blend_image_onto_image(tmp, 1, 0, 0, chart_width, chart_height,
					widget->xoffset - up_x, widget->yoffset - up_y,
					chart_width, chart_height));
		}

		imlib_context_set_image(tmp);
		imlib_free_image();
	}

	if (mask_img) {
		imlib_context_set_image(mask_img);
		imlib_free_image();
	}
}

static void update_barchart(xsg_widget_t *widget, xsg_var_t *var) {
	barchart_t *barchart;
	barchart_var_t *barchart_var;
	xsg_list_t *l;
	bool dirty = FALSE;

	barchart = (barchart_t *) widget->data;

	if (barchart->const_min && barchart->const_max) {
		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var == NULL) || (barchart_var->var == var)) {
				double value = barchart_var->value;

				barchart_var->value = xsg_var_get_num(barchart_var->var);
				if (value != barchart_var->value)
					dirty = TRUE;
			}
		}
	} else if (barchart->const_min) {
		double pos = 0.0;
		double neg = 0.0;

		barchart->max = DBL_MIN;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var == NULL) || (barchart_var->var == var)) {
				double value = barchart_var->value;

				barchart_var->value = xsg_var_get_num(barchart_var->var);
				if (value != barchart_var->value)
					dirty = TRUE;
			}

			if (barchart_var->value > 0.0) {
				if (barchart_var->add_prev)
					pos += barchart_var->value;
				else
					pos = barchart_var->value;
				barchart->max = MAX(barchart->max, pos);
			} else if (barchart_var->value < 0.0) {
				if (barchart_var->add_prev)
					neg += barchart_var->value;
				else
					neg = barchart_var->value;
				barchart->max = MAX(barchart->max, neg);
			} else {
				if (!barchart_var->add_prev) {
					pos = 0.0;
					neg = 0.0;
				}
				barchart->max = MAX(barchart->max, 0.0);
			}
		}
	} else if (barchart->const_max) {
		double pos = 0.0;
		double neg = 0.0;

		barchart->min = DBL_MAX;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var == NULL) || (barchart_var->var == var)) {
				double value = barchart_var->value;

				barchart_var->value = xsg_var_get_num(barchart_var->var);
				if (value != barchart_var->value)
					dirty = TRUE;
			}

			if (barchart_var->value > 0.0) {
				if (barchart_var->add_prev)
					pos += barchart_var->value;
				else
					pos = barchart_var->value;
				barchart->min = MIN(barchart->min, pos);
			} else if (barchart_var->value < 0.0) {
				if (barchart_var->add_prev)
					neg += barchart_var->value;
				else
					neg = barchart_var->value;
				barchart->min = MIN(barchart->min, neg);
			} else {
				if (!barchart_var->add_prev) {
					pos = 0.0;
					neg = 0.0;
				}
				barchart->min = MIN(barchart->min, 0.0);
			}
		}
	} else {
		double pos = 0.0;
		double neg = 0.0;

		barchart->min = DBL_MAX;
		barchart->max = DBL_MIN;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var == NULL) || (barchart_var->var == var)) {
				double value = barchart_var->value;

				barchart_var->value = xsg_var_get_num(barchart_var->var);
				if (value != barchart_var->value)
					dirty = TRUE;
			}

			if (barchart_var->value > 0.0) {
				if (barchart_var->add_prev)
					pos += barchart_var->value;
				else
					pos = barchart_var->value;
				barchart->min = MIN(barchart->min, pos);
				barchart->max = MAX(barchart->max, pos);
			} else if (barchart_var->value < 0.0) {
				if (barchart_var->add_prev)
					neg += barchart_var->value;
				else
					neg = barchart_var->value;
				barchart->min = MIN(barchart->min, neg);
				barchart->max = MAX(barchart->max, neg);
			} else {
				if (!barchart_var->add_prev) {
					pos = 0.0;
					neg = 0.0;
				}
				barchart->min = MIN(barchart->min, 0.0);
				barchart->max = MAX(barchart->max, 0.0);
			}
		}
	}

	if (dirty)
		xsg_window_update_append_rect(widget->window, widget->xoffset, widget->yoffset,
				widget->width, widget->height);
}

static void scroll_barchart(xsg_widget_t *widget) {
	return;
}

xsg_widget_t *xsg_widget_barchart_parse(xsg_window_t *window, uint64_t *update) {
	xsg_widget_t *widget;
	barchart_t *barchart;
	double angle = 0.0;

	widget = xsg_widgets_new(window);

	barchart = xsg_new(barchart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_barchart;
	widget->update_func = update_barchart;
	widget->scroll_func = scroll_barchart;
	widget->data = (void *) barchart;

	*update = widget->update;

	barchart->angle = NULL;
	barchart->min = 0.0;
	barchart->max = 0.0;
	barchart->const_min = FALSE;
	barchart->const_max = FALSE;
	barchart->mask = NULL;
	barchart->var_list = NULL;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_uint();
			widget->visible_var = xsg_var_parse(window, widget, widget->visible_update);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Min")) {
			barchart->min = xsg_conf_read_double();
			barchart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			barchart->max = xsg_conf_read_double();
			barchart->const_max = TRUE;
		} else if (xsg_conf_find_command("Mask")) {
			if (barchart->mask != NULL)
				xsg_free(barchart->mask);
			barchart->mask = xsg_conf_read_string();
		} else {
			xsg_conf_error("Visible, Angle, Min, Max or Mask expected");
		}
	}

	if (angle != 0.0)
		barchart->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, &widget->width, &widget->height);

	return widget;
}

void xsg_widget_barchart_parse_var(xsg_var_t *var) {
	xsg_widget_t *widget;
	barchart_t *barchart;
	barchart_var_t *barchart_var;

	barchart_var = xsg_new(barchart_var_t, 1);

	barchart_var->var = var;
	barchart_var->color = xsg_imlib_uint2color(xsg_conf_read_color());
	barchart_var->range = NULL;
	barchart_var->range_angle = 0.0;
	barchart_var->add_prev = FALSE;
	barchart_var->value = DNAN;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;

			if (barchart_var->range != NULL) {
				imlib_context_set_color_range(barchart_var->range);
				imlib_free_color_range();
			}

			barchart_var->range = imlib_create_color_range();
			imlib_context_set_color_range(barchart_var->range);
			imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
					barchart_var->color.blue, barchart_var->color.alpha);
			imlib_add_color_to_color_range(0);
			barchart_var->range_angle = xsg_conf_read_double();
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
		} else if (xsg_conf_find_command("AddPrev")) {
			barchart_var->add_prev = TRUE;
		} else {
			xsg_conf_error("ColorRange or AddPrev expected");
		}
	}

	widget = xsg_widgets_last();
	barchart = widget->data;
	barchart->var_list = xsg_list_append(barchart->var_list, barchart_var);
}


