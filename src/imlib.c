/* widgets.c
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
#include <Imlib2.h>
#include <stdio.h>
#include <math.h>
#include <signal.h>

#include "imlib.h"
#include "argb.h"
#include "fontconfig.h"

/******************************************************************************/

static xsg_hash_table_t *image_hash_table = NULL;

/******************************************************************************/

static void
signal_handler(int signum)
{
	if (signum == SIGUSR1) {
		int cache_size;

		xsg_message("flushing image cache");

		if (image_hash_table != NULL)
			xsg_hash_table_remove_all(image_hash_table);

		cache_size = imlib_get_cache_size();
		imlib_set_cache_size(0);
		imlib_set_cache_size(cache_size);
	}
	if (signum == SIGUSR2) {
		xsg_message("flushing font cache");
		imlib_flush_font_cache();
	}
}

/******************************************************************************/

static const char *
error2str(Imlib_Load_Error error)
{
	switch (error) {
	case IMLIB_LOAD_ERROR_NONE:
		return "IMLIB_LOAD_ERROR_NONE";
	case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
		return "IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST";
	case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
		return "IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY";
	case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
		return "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ";
	case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
		return "IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT";
	case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
		return "IMLIB_LOAD_ERROR_PATH_TOO_LONG";
	case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
		return "IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT";
	case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
		return "IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY";
	case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
		return "IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE";
	case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
		return "IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS";
	case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
		return "IMLIB_LOAD_ERROR_OUT_OF_MEMORY";
	case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
		return "IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS";
	case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
		return "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE";
	case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
		return "IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE";
	case IMLIB_LOAD_ERROR_UNKNOWN:
	default:
		return "IMLIB_LOAD_ERROR_UNKNOWN";
	}
}

Imlib_Image
xsg_imlib_load_image(const char *filename)
{
	static char **pathv = NULL;
	Imlib_Load_Error error;
	Imlib_Image image;
	char **p;
	char *file;

	if (unlikely(image_hash_table == NULL)) {
		image_hash_table = xsg_hash_table_new_full(xsg_str_hash,
				xsg_str_equal, free, free);
	}

	if (unlikely(pathv == NULL)) {
		pathv = xsg_get_path_from_env("XSYSGUARD_IMAGE_PATH",
				XSYSGUARD_IMAGE_PATH);

		if (pathv == NULL) {
			xsg_error("cannot get XSYSGUARD_IMAGE_PATH");
		}
	}

	file = xsg_hash_table_lookup(image_hash_table, filename);

	if (file != NULL) {
		image = imlib_load_image_with_error_return(file, &error);

		if (image != NULL) {
			xsg_debug("loaded image: \"%s\"", file);
			return image;
		} else {
			xsg_message("loading image \"%s\" failed: %s", file,
					error2str(error));
			xsg_hash_table_remove(image_hash_table, filename);
		}
	}

	for (p = pathv; *p; p++) {
		xsg_debug("searching for image \"%s\" in \"%s\"", filename, *p);

		file = xsg_build_filename(*p, filename, NULL);

		image = imlib_load_image_with_error_return(file, &error);

		if (image != NULL) {
			xsg_message("loaded image: \"%s\"", file);
			xsg_hash_table_insert(image_hash_table,
					xsg_strdup(filename), file);
			return image;
		} else {
			xsg_message("loading image \"%s\" failed: %s", file,
					error2str(error));
			xsg_free(file);
		}
	}

	xsg_warning("cannot load image: \"%s\"", filename);

	return NULL;
}

Imlib_Color
xsg_imlib_uint2color(uint32_t u)
{
	Imlib_Color color;

	color.alpha = A_VAL(&u);
	color.red = R_VAL(&u);
	color.green = G_VAL(&u);
	color.blue = B_VAL(&u);

	return color;
}

void
xsg_imlib_blend_mask(Imlib_Image mask)
{
	Imlib_Image image;
	DATA32 *image_data;
	DATA32 *mask_data;
	unsigned int image_width, image_height;
	unsigned int mask_width, mask_height;
	unsigned int width, height;
	unsigned int x, y;

	image = imlib_context_get_image();

	imlib_context_set_image(mask);
	mask_width = imlib_image_get_width();
	mask_height = imlib_image_get_height();
	mask_data = imlib_image_get_data_for_reading_only();

	imlib_context_set_image(image);
	image_width = imlib_image_get_width();
	image_height = imlib_image_get_height();
	image_data = imlib_image_get_data();

	width = MIN(mask_width, image_height);
	height = MIN(mask_height, image_height);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			unsigned char *image;
			unsigned char *mask;
			image = (unsigned char *) image_data
					+ x + y * image_height;
			mask = (unsigned char *) mask_data
					+ x + y * mask_height;

			image[0] = (image[0] * mask[0]) / 0xff;
			image[1] = (image[1] * mask[1]) / 0xff;
			image[2] = (image[2] * mask[2]) / 0xff;
			image[3] = (image[3] * mask[3]) / 0xff;
		}
	}

	imlib_image_put_back_data(image_data);
}

void
xsg_imlib_blend_background(
	const char *bg,
	int xoffset,
	int yoffset,
	unsigned width,
	unsigned height,
	int orientation,
	uint64_t update
)
{
	int clip_x, clip_y, clip_w, clip_h;
	Imlib_Image image, bg_img;
	int bg_width, bg_height;
	int xx, yy, x_count, y_count;
	uint64_t tick;

	bg_img = xsg_imlib_load_image(bg);

	if (unlikely(bg_img == NULL)) {
		xsg_warning("cannot load image \"%s\"", bg);
		return;
	}

	image = imlib_context_get_image();
	imlib_context_get_cliprect(&clip_x, &clip_y, &clip_w, &clip_h);

	imlib_context_set_image(bg_img);

	imlib_image_orientate(orientation);

	bg_width = imlib_image_get_width();
	bg_height = imlib_image_get_height();

	imlib_context_set_image(image);

	imlib_context_set_cliprect(xoffset, yoffset, width, height);

	tick = xsg_main_get_tick();

	switch (orientation) {
	case 0:
		xoffset += (tick / update) % bg_width;
		break;
	case 1:
		yoffset += (tick / update) % bg_height;
		break;
	case 2:
		xoffset -= (tick / update) % bg_width;
		break;
	case 3:
		yoffset -= (tick / update) % bg_height;
		break;
	default:
		xsg_error("unknown orientation");
	}

	x_count = 1 + width / bg_width;
	y_count = 1 + height / bg_height;

	for (xx = - 1; xx <= x_count; xx++) {
		for (yy = - 1; yy <= y_count; yy++) {
			int dest_x, dest_y;

			dest_x = xoffset + xx * bg_width;
			dest_y = yoffset + yy * bg_height;

			imlib_blend_image_onto_image(bg_img, 1, 0, 0,
					bg_width, bg_height,
					dest_x, dest_y,
					bg_width, bg_height);
		}
	}

	imlib_context_set_image(bg_img);
	imlib_free_image();

	imlib_context_set_image(image);
	imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
}

Imlib_Image
xsg_imlib_create_color_range_image(
	unsigned width,
	unsigned height,
	Imlib_Color_Range range,
	double range_angle
)
{
	Imlib_Image img, old_img;
	Imlib_Color_Range old_range;

	old_img = imlib_context_get_image();
	old_range = imlib_context_get_color_range();

	img = imlib_create_image(width, height);

	if (unlikely(img == NULL)) {
		return NULL;
	}

	imlib_context_set_image(img);
	imlib_image_set_has_alpha(1);
	imlib_image_clear();
	imlib_context_set_color_range(range);
	imlib_image_fill_color_range_rectangle(0, 0, width, height,
			range_angle);

	imlib_context_set_color_range(old_range);
	imlib_context_set_image(old_img);

	return img;
}

/******************************************************************************
 *
 * font drawing
 *
 ******************************************************************************/

static bool
xsg_imlib_check_text_draw_bug(void)
{
	Imlib_Image old_image;
	Imlib_Text_Direction old_direction;
	int old_red = 0, old_green = 0, old_blue = 0, old_alpha = 0;
	int old_clip_x = 0, old_clip_y = 0, old_clip_w = 0, old_clip_h = 0;
	bool has_bug = TRUE;
	int c;

	old_image = imlib_context_get_image();
	old_direction = imlib_context_get_direction();
	imlib_context_get_color(&old_red, &old_green, &old_blue, &old_alpha);
	imlib_context_get_cliprect(&old_clip_x, &old_clip_y,
			&old_clip_w, &old_clip_h);

	imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);
	imlib_context_set_cliprect(0, 0, 0, 0);

	for (c = 1; c < 256; c++) {
		Imlib_Image image;
		int width = 0;
		int height = 0;
		char text[2];
		int x, y;

		text[0] = c;
		text[1] = '\0';

		imlib_get_text_advance(text, &width, &height);

		if (width < 1 || height < 1) {
			continue;
		}

		image = imlib_create_image(width * 2, height);

		if (image == NULL) {
			continue;
		}

		imlib_context_set_image(image);
		imlib_image_clear_color(0, 0, 0, 0xff);
		imlib_context_set_color(0xff, 0xff, 0xff, 0xff);
		imlib_context_set_cliprect(width, 0, width, height);

		imlib_text_draw(width, 0, text);

		for (x = 0; x < width * 2; x++) {
			for (y = 0; y < height; y++) {
				Imlib_Color c = { 0 };

				imlib_image_query_pixel(x, y, &c);

				if (c.red != 0 || c.green != 0 || c.blue != 0) {
					imlib_free_image();
					has_bug = FALSE;
					goto restore_context;
				}
			}
		}

		imlib_free_image();
	}

	restore_context:

	imlib_context_set_image(old_image);
	imlib_context_set_direction(old_direction);
	imlib_context_set_color(old_red, old_green, old_blue, old_alpha);
	imlib_context_set_cliprect(old_clip_x, old_clip_y,
			old_clip_w, old_clip_h);

	if (has_bug) {
		xsg_warning("found cliprect bug in imlib_text_draw. "
				"enabling workaround...");
	}

	return has_bug;
}

static bool
xsg_imlib_has_text_draw_bug(void)
{
	static bool initialized = FALSE;
	static bool has_bug = FALSE;

	if (unlikely(imlib_context_get_font() == NULL)) {
		return FALSE;
	}

	if (unlikely(!initialized)) {
		has_bug = xsg_imlib_check_text_draw_bug();
		initialized = TRUE;
	}

	return has_bug;
}

void
xsg_imlib_text_draw_with_return_metrics(
	int xoffset,
	int yoffset,
	const char *text,
	int *width_return,
	int *height_return,
	int *horizontal_advance_return,
	int *vertical_advance_return
)
{
	if (likely(!xsg_imlib_has_text_draw_bug())) {
		imlib_text_draw_with_return_metrics(xoffset, yoffset, text,
				width_return, height_return,
				horizontal_advance_return,
				vertical_advance_return);
	} else {
		Imlib_Image image, tmp;
		Imlib_Text_Direction direction;
		double angle;
		int clip_x = 0;
		int clip_y = 0;
		int clip_w = 0;
		int clip_h = 0;
		int width = 0;
		int height = 0;
		int next_x = 0;
		int next_y = 0;

		image = imlib_context_get_image();

		if (image == NULL) {
			return;
		}

		direction = imlib_context_get_direction();
		angle = imlib_context_get_angle();
		imlib_context_get_cliprect(&clip_x, &clip_y, &clip_w, &clip_h);

		imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);
		imlib_context_set_cliprect(0, 0, 0, 0);

		imlib_get_text_advance(text, &width, &height);

		tmp = imlib_create_image(width, height);

		if (tmp == NULL) {
			imlib_context_set_direction(direction);
			imlib_context_set_cliprect(clip_x, clip_y,
					clip_w, clip_h);
			return;
		}

		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear_color(0, 0, 0, 0);

		imlib_text_draw_with_return_metrics(0, 0, text, NULL, NULL,
				&next_x, &next_y);

		switch (direction) {
		case IMLIB_TEXT_TO_RIGHT:
			angle = 0.0;
			break;
		case IMLIB_TEXT_TO_DOWN:
			{
				int tmp = width;
				width = height;
				height = tmp;;
			}
			angle = 0.0;
			imlib_image_orientate(1);
			break;
		case IMLIB_TEXT_TO_LEFT:
			angle = 0.0;
			imlib_image_orientate(2);
			break;
		case IMLIB_TEXT_TO_UP:
			{
				int tmp = width;
				width = height;
				height = tmp;
			}
			angle = 0.0;
			imlib_image_orientate(3);
			break;
		default:
			break;
		}

		imlib_context_set_direction(direction);
		imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
		imlib_context_set_image(image);

		if (angle == 0.0) {
			imlib_blend_image_onto_image(tmp,
					imlib_image_has_alpha(), 0, 0,
					width, height, xoffset, yoffset,
					width, height);
		} else {
			int xx, yy;
			double sa, ca;

			sa = sin(angle);
			ca = cos(angle);
			xx = xoffset;
			yy = yoffset;
			if (sa > 0.0) {
				xx += sa * height;
			} else {
				yy -= sa * width;
			}
			if (ca < 0.0) {
				xx -= ca * width;
				yy -= ca * height;
			}
			imlib_blend_image_onto_image_skewed(tmp,
					imlib_image_has_alpha(), 0, 0,
					width, height, xx, yy,
					(width * ca), (width * sa), 0, 0);
		}

		imlib_context_set_image(tmp);
		imlib_free_image();

		imlib_context_set_image(image);

		switch (direction) {
		case IMLIB_TEXT_TO_RIGHT:
		case IMLIB_TEXT_TO_LEFT:
			if (width_return) {
				*width_return = width;
			}
			if (height_return) {
				*height_return = height;
			}
			if (horizontal_advance_return) {
				*horizontal_advance_return = next_x;
			}
			if (vertical_advance_return) {
				*vertical_advance_return = next_y;
			}
			break;
		case IMLIB_TEXT_TO_DOWN:
		case IMLIB_TEXT_TO_UP:
			if (width_return) {
				*width_return = height;
			}
			if (height_return) {
				*height_return = width;
			}
			if (horizontal_advance_return) {
				*horizontal_advance_return = next_y;
			}
			if (vertical_advance_return) {
				*vertical_advance_return = next_x;
			}
			break;
		case IMLIB_TEXT_TO_ANGLE:
			{
				double sa, ca;
				double x1, x2, xt;
				double y1, y2, yt;

				sa = sin(angle);
				ca = cos(angle);

				x1 = x2 = 0.0;
				xt = ca * width;
				if (xt < x1) {
					x1 = xt;
				}
				if (xt > x2) {
					x2 = xt;
				}
				xt = -(sa * height);
				if (xt < x1) {
					x1 = xt;
				}
				if (xt > x2) {
					x2 = xt;
				}
				xt = ca * width - sa * height;
				if (xt < x1) {
					x1 = xt;
				}
				if (xt > x2) {
					x2 = xt;
				}
				width = (int) (x2 - x1);
				y1 = y2 = 0.0;
				yt = sa * width;
				if (yt < y1) {
					y1 = yt;
				}
				if (yt > y2) {
					y2 = yt;
				}
				yt = ca * height;
				if (yt < y1) {
					y1 = yt;
				}
				if (yt > y2) {
					y2 = yt;
				}
				yt = sa * width + ca * height;
				if (yt < y1) {
					y1 = yt;
				}
				if (yt > y2) {
					y2 = yt;
				}
				height = (int) (y2 - y1);
			}
			if (width_return) {
				*width_return = width;
			}
			if (height_return) {
				*height_return = height;
			}
			if (horizontal_advance_return) {
				*horizontal_advance_return = next_x;
			}
			if (vertical_advance_return) {
				*vertical_advance_return = next_y;
			}
			break;
		default:
			break;
		}

	}
}

void
xsg_imlib_text_draw(int xoffset, int yoffset, const char *text)
{
	if (likely(!xsg_imlib_has_text_draw_bug())) {
		imlib_text_draw(xoffset, yoffset, text);
	} else {
		xsg_imlib_text_draw_with_return_metrics(xoffset, yoffset, text,
				NULL, NULL, NULL, NULL);
	}
}

/******************************************************************************/

void
xsg_imlib_list_fonts(void)
{
	char **list;
	int num, i;

	list = imlib_list_fonts(&num);

	for (i = 0; i < num; i++) {
		printf("%s\n", list[i]);
	}

	imlib_free_font_list(list, num);
}

/******************************************************************************
 *
 * init
 *
 ******************************************************************************/

void
xsg_imlib_init_font_path(bool enable_fontconfig)
{
	char **pathv, **p;

	pathv = xsg_get_path_from_env("XSYSGUARD_FONT_PATH",
			XSYSGUARD_FONT_PATH);

	if (unlikely(pathv == NULL)) {
		xsg_error("cannot get XSYSGUARD_FONT_PATH");
	}

	for (p = pathv; *p; p++) {
		xsg_message("adding dir to font path: \"%s\"", *p);
		imlib_add_path_to_font_path(*p);
	}

	xsg_strfreev(pathv);

	if (!enable_fontconfig) {
		return;
	}

	pathv = xsg_fontconfig_get_path();

	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			xsg_message("adding dir to font path (fontconfig): "
					"\"%s\"", *p);
			imlib_add_path_to_font_path(*p);
		}
		xsg_strfreev(pathv);
	}
}

void
xsg_imlib_init(void)
{
	xsg_main_add_signal_handler(signal_handler);
}

/******************************************************************************/

void
xsg_imlib_set_cache_size(int size)
{
	imlib_set_cache_size(size);
}

void
xsg_imlib_set_font_cache_size(int size)
{
	imlib_set_font_cache_size(size);
}

