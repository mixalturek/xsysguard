/* widgets.c
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

#include <xsysguard.h>
#include <Imlib2.h>

/******************************************************************************/

typedef union {
	uint32_t uint;
	struct {
		unsigned char alpha;
		unsigned char red;
		unsigned char green;
		unsigned char blue;
	} argb;
	struct {
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		unsigned char alpha;
	} rgba;
} color_t;

/******************************************************************************/

Imlib_Image xsg_imlib_load_image(const char *filename) {
	Imlib_Image image = NULL;
	static char **pathv = NULL;
	char **p;
	char *file;

	if (unlikely(pathv == NULL)) {
		pathv = xsg_get_path_from_env("XSYSGUARD_IMAGE_PATH", XSYSGUARD_IMAGE_PATH);

		if (pathv == NULL)
			xsg_error("Cannot get XSYSGUARD_IMAGE_PATH");
	}

	for (p = pathv; *p; p++) {
		xsg_message("Searching for image \"%s\" in \"%s\"", filename, *p);
		file = xsg_build_filename(*p, filename, NULL);
		if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR)) {
			image = imlib_load_image(file);
		}
		if (image) {
			xsg_message("Found image \"%s\"", file);
			xsg_free(file);
			return image;
		}
		xsg_free(file);
	}

	return NULL;
}

Imlib_Color xsg_imlib_uint2color(uint32_t u) {
	Imlib_Color color;
	color_t c;

	c.uint = u;
	color.alpha = c.argb.alpha & 0xff;
	color.red = c.argb.red & 0xff;
	color.green = c.argb.green & 0xff;
	color.blue = c.argb.blue & 0xff;

	return color;
}

void xsg_imlib_blend_mask(Imlib_Image image, Imlib_Image mask) {
	DATA32 *image_data;
	DATA32 *mask_data;
	unsigned int image_width, image_height;
	unsigned int mask_width, mask_height;
	unsigned int width, height;
	unsigned int x, y;

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
			image = (unsigned char *) image_data + x + y * image_height;
			mask = (unsigned char *) mask_data + x + y * mask_height;

			image[0] = (image[0] * mask[0]) / 0xff;
			image[1] = (image[1] * mask[1]) / 0xff;
			image[2] = (image[2] * mask[2]) / 0xff;
			image[3] = (image[3] * mask[3]) / 0xff;
		}
	}

	imlib_image_put_back_data(image_data);
}

