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
#include "angle.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	uint32_t var_id;
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

	xsg_debug("render_barchart");

	barchart = (barchart_t *) widget->data;

	if (barchart->min >= barchart->max)
		return;

	// load mask image
	mask_img = NULL;
	if (barchart->mask) {
		mask_img = xsg_imlib_load_image(barchart->mask);

		if (unlikely(mask_img == NULL))
			xsg_warning("Cannot load image \"%s\"", barchart->mask);
	}

	if ((mask_img == NULL) && ((barchart->angle == NULL) || (barchart->angle->angle == 0.0))) {
		int clip_x, clip_y, clip_w, clip_h;
		int null_y, prev_pos_h, prev_neg_h;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->height) / (barchart->max - barchart->min);

		clip_x = widget->xoffset - up_x;
		clip_w = widget->width;

		null_y = widget->yoffset - up_y + ((int) (barchart->max * pixel_mult));

		prev_pos_h = 0;
		prev_neg_h = 0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;

			barchart_var = l->data;

			if (isnan(barchart_var->value))
				continue;

			if (barchart_var->value > 0.0) {
				clip_h = barchart_var->value * pixel_mult;
				if (barchart_var->add_prev) {
					clip_y = null_y - clip_h - prev_pos_h;
					prev_pos_h += clip_h;
				} else {
					clip_y = null_y - clip_h;
					prev_pos_h = clip_h;
				}
			} else if (barchart_var->value < 0.0) {
				clip_h = barchart_var->value * pixel_mult * (- 1.0);
				if (barchart_var->add_prev) {
					clip_y = null_y + prev_neg_h;
					prev_neg_h += clip_h;
				} else {
					clip_y = null_y;
					prev_neg_h = clip_h;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
				}
				continue;
			}

			if (clip_y < (widget->yoffset - up_y)) {
				clip_h -= widget->yoffset - up_y - clip_y;
				clip_y = widget->yoffset - up_y;
			}

			if ((clip_y + clip_h) > (widget->yoffset - up_y + widget->height))
				clip_h -= (clip_y + clip_h) - (widget->yoffset - up_y + widget->height);

			if (clip_h <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle;
				imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(clip_x, clip_y, clip_w, clip_h));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 90.0)) {
		int clip_x, clip_y, clip_w, clip_h;
		int null_x, prev_pos_w, prev_neg_w;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->width) / (barchart->max - barchart->min);

		clip_y = widget->yoffset - up_y;
		clip_h = widget->height;

		null_x = widget->xoffset - up_x + widget->width - ((int) (barchart->max * pixel_mult));

		prev_pos_w = 0;
		prev_neg_w = 0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;

			barchart_var = l->data;

			if (isnan(barchart_var->value))
				continue;

			if (barchart_var->value > 0.0) {
				clip_w = barchart_var->value * pixel_mult;
				if (barchart_var->add_prev) {
					clip_x = null_x + prev_pos_w;
					prev_pos_w += clip_w;
				} else {
					clip_x = null_x;
					prev_pos_w = clip_w;
				}
			} else if (barchart_var->value < 0.0) {
				clip_w = barchart_var->value * pixel_mult * (- 1.0);
				if (barchart_var->add_prev) {
					clip_x = null_x - clip_w - prev_neg_w;
					prev_neg_w += clip_w;
				} else {
					clip_x = null_x - clip_w;
					prev_neg_w = clip_w;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_w = 0;
					prev_neg_w = 0;
				}
				continue;
			}

			if (clip_x < (widget->xoffset - up_x)) {
				clip_w -= widget->xoffset - up_x - clip_x;
				clip_x = widget->xoffset - up_x;
			}

			if ((clip_x + clip_w) > (widget->xoffset - up_x + widget->width))
				clip_w -= (clip_x + clip_w) - (widget->xoffset - up_x + widget->width);

			if (clip_w <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 90.0;
				imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
							widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(clip_x, clip_y, clip_w, clip_h));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 180.0)) {
		int clip_x, clip_y, clip_w, clip_h;
		int null_y, prev_pos_h, prev_neg_h;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->height) / (barchart->max - barchart->min);

		clip_x = widget->xoffset - up_x;
		clip_w = widget->width;

		null_y = widget->yoffset - up_y + widget->height - ((int) (barchart->max * pixel_mult));

		prev_pos_h = 0;
		prev_neg_h = 0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;

			barchart_var = l->data;

			if (isnan(barchart_var->value))
				continue;

			if (barchart_var->value > 0.0) {
				clip_h = barchart_var->value * pixel_mult;
				if (barchart_var->add_prev) {
					clip_y = null_y + prev_pos_h;
					prev_pos_h += clip_h;
				} else {
					clip_y = null_y;
					prev_pos_h = clip_h;
				}
			} else if (barchart_var->value < 0.0) {
				clip_h = barchart_var->value * pixel_mult * -1;
				if (barchart_var->add_prev) {
					clip_y = null_y - clip_h - prev_neg_h;
					prev_neg_h += clip_h;
				} else {
					clip_y = null_y - clip_h;
					prev_neg_h = clip_h;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
				}
				continue;
			}

			if (clip_y < (widget->yoffset - up_y)) {
				clip_h -= widget->yoffset - up_y - clip_y;
				clip_y = widget->yoffset - up_y;
			}

			if ((clip_y + clip_h) > (widget->yoffset - up_y + widget->height))
				clip_h -= (clip_y + clip_h) - (widget->yoffset - up_y + widget->height);

			if (clip_h <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 180.0;
				imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
						widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(clip_x, clip_y, clip_w, clip_h));
			}
		}
	} else if ((mask_img == NULL) && (barchart->angle->angle == 270.0)) {
		int clip_x, clip_y, clip_w, clip_h;
		int null_x, prev_pos_w, prev_neg_w;
		double pixel_mult;

		imlib_context_set_image(buffer);

		pixel_mult = ((double) widget->width) / (barchart->max - barchart->min);

		clip_y = widget->yoffset - up_y;
		clip_h = widget->height;

		null_x = widget->xoffset - up_x + ((int) (barchart->max * pixel_mult));

		prev_pos_w = 0;
		prev_neg_w = 0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;

			barchart_var = l->data;

			if (isnan(barchart_var->value))
				continue;

			if (barchart_var->value > 0.0) {
				clip_w = barchart_var->value * pixel_mult;
				if (barchart_var->add_prev) {
					clip_x = null_x - clip_w - prev_pos_w;
					prev_pos_w += clip_w;
				} else {
					clip_x = null_x - clip_w;
					prev_pos_w = clip_w;
				}
			} else if (barchart_var->value < 0.0) {
				clip_w = barchart_var->value * pixel_mult * (- 1.0);
				if (barchart_var->add_prev) {
					clip_x = null_x + prev_neg_w;
					prev_neg_w += clip_w;
				} else {
					clip_x = null_x;
					prev_neg_w = clip_w;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_w = 0;
					prev_neg_w = 0;
				}
				continue;
			}

			if (clip_x < (widget->xoffset - up_x)) {
				clip_w -= widget->xoffset - up_x - clip_x;
				clip_x = widget->xoffset - up_x;
			}

			if ((clip_x + clip_w) > (widget->xoffset - up_x + widget->width))
				clip_w -= (clip_x + clip_h) - (widget->xoffset - up_x + widget->width);

			if (clip_w <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle + 270.0;
				imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(widget->xoffset - up_x, widget->yoffset - up_y,
							widget->width, widget->height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(clip_x, clip_y, clip_w, clip_h));
			}
		}
	} else {
		int clip_x, clip_y, clip_w, clip_h;
		int null_y, prev_pos_h, prev_neg_h;
		double pixel_mult;
		unsigned int width, height;
		Imlib_Image tmp;

		if (barchart->angle) {
			width = barchart->angle->width;
			height = barchart->angle->height;
		} else {
			width = widget->width;
			height = widget->height;
		}

		tmp = imlib_create_image(width, height);
		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear();

		pixel_mult = ((double) height) / (barchart->max - barchart->min);

		clip_x = 0;
		clip_w = width;

		null_y = (int) (barchart->max * pixel_mult);

		prev_pos_h = 0;
		prev_neg_h = 0;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var_t *barchart_var;

			barchart_var = l->data;

			if (isnan(barchart_var->value))
				continue;

			if (barchart_var->value > 0.0) {
				clip_h = barchart_var->value * pixel_mult;
				if (barchart_var->add_prev) {
					clip_y = null_y - clip_h - prev_pos_h;
					prev_pos_h += clip_h;
				} else {
					clip_y = null_y - clip_h;
					prev_pos_h = clip_h;
				}
			} else if (barchart_var->value < 0.0) {
				clip_h = barchart_var->value * pixel_mult * (- 1.0);
				if (barchart_var->add_prev) {
					clip_y = null_y + prev_neg_h;
					prev_neg_h += clip_h;
				} else {
					clip_y = null_y;
					prev_neg_h = clip_h;
				}
			} else {
				if (!barchart_var->add_prev) {
					prev_pos_h = 0;
					prev_neg_h = 0;
				}
				continue;
			}

			if (clip_y < 0) {
				clip_h -= -clip_y;
				clip_y = 0;
			}

			if (clip_y + clip_h > height)
				clip_h -= (clip_y + clip_h) - height;

			if (clip_h <= 0)
				continue;

			if (barchart_var->range != NULL) {
				double range_angle = barchart_var->range_angle;
				imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
				imlib_context_set_color_range(barchart_var->range);
				T(imlib_image_fill_color_range_rectangle(0, 0, width, height, range_angle));
				imlib_context_set_cliprect(0, 0, 0, 0);
			} else {
				imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
						barchart_var->color.blue, barchart_var->color.alpha);
				T(imlib_image_fill_rectangle(clip_x, clip_y, clip_w, clip_h));
			}
		}

		if (mask_img)
			T(xsg_imlib_blend_mask(mask_img));

		imlib_context_set_image(buffer);
		if (barchart->angle) {
			T(imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0, width, height,
					barchart->angle->xoffset - up_x, barchart->angle->yoffset - up_y,
					barchart->angle->angle_x, barchart->angle->angle_y));
		} else {
			T(imlib_blend_image_onto_image(tmp, 1, 0, 0, width, height,
					widget->xoffset - up_x, widget->yoffset - up_y,
					width, height));
		}

		imlib_context_set_image(tmp);
		imlib_free_image();
	}

	if (mask_img) {
		imlib_context_set_image(mask_img);
		imlib_free_image();
	}
}

static void update_barchart(xsg_widget_t *widget, uint32_t var_id) {
	barchart_t *barchart;
	barchart_var_t *barchart_var;
	xsg_list_t *l;

	barchart = (barchart_t *) widget->data;

	if (barchart->const_min && barchart->const_max) {
		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var_id == 0xffffffff) || (barchart_var->var_id == var_id))
				barchart_var->value = xsg_var_get_double(barchart_var->var_id);
		}
	} else if (barchart->const_min) {
		double pos = 0.0;
		double neg = 0.0;

		barchart->max = DBL_MIN;

		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;

			if ((var_id == 0xffffffff) || (barchart_var->var_id == var_id))
				barchart_var->value = xsg_var_get_double(barchart_var->var_id);

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

			if ((var_id == 0xffffffff) || (barchart_var->var_id == var_id))
				barchart_var->value = xsg_var_get_double(barchart_var->var_id);

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

			if ((var_id == 0xffffffff) || (barchart_var->var_id == var_id))
				barchart_var->value = xsg_var_get_double(barchart_var->var_id);

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
}

static void scroll_barchart(xsg_widget_t *widget) {
	return;
}

void xsg_widget_barchart_parse(uint64_t *update, uint32_t *widget_id) {
	xsg_widget_t *widget;
	barchart_t *barchart;
	double angle = 0.0;

	widget = xsg_new0(xsg_widget_t, 1);
	barchart = xsg_new0(barchart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->show_var_id = 0xffffffff;
	widget->show = TRUE;
	widget->render_func = render_barchart;
	widget->update_func = update_barchart;
	widget->scroll_func = scroll_barchart;
	widget->data = (void *) barchart;

	*update = widget->update;
	*widget_id = xsg_widgets_add(widget);

	barchart->angle = NULL;
	barchart->min = 0.0;
	barchart->max = 0.0;
	barchart->const_min = FALSE;
	barchart->const_max = FALSE;
	barchart->mask = NULL;
	barchart->var_list = NULL;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Show")) {
			widget->show_var_id = xsg_var_parse_double(*widget_id, *update);
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
			xsg_conf_error("Angle, Min, Max or Mask");
		}
	}

	if (angle != 0.0)
		barchart->angle = xsg_angle_parse(angle, widget->xoffset, widget->yoffset, &widget->width, &widget->height);
}

void xsg_widget_barchart_parse_var(uint32_t var_id) {
	xsg_widget_t *widget;
	barchart_t *barchart;
	barchart_var_t *barchart_var;

	if (!xsg_var_is_double(var_id))
		xsg_error("BarChart allows double vars only");

	barchart_var = xsg_new0(barchart_var_t, 1);

	barchart_var->var_id = var_id;
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
			xsg_conf_error("ColorRange or AddPrev");
		}
	}

	widget = xsg_widgets_last();
	barchart = widget->data;
	barchart->var_list = xsg_list_append(barchart->var_list, barchart_var);
}


