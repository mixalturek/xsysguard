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

#include "imlib.h"

/******************************************************************************/

Imlib_Image xsg_imlib_load_image(const char *filename) {
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
			Imlib_Image image;
			Imlib_Load_Error error;
			char *error_str = NULL;

			xsg_message("Found image in \"%s\". Loading...", file);

			image = imlib_load_image_with_error_return(file, &error);

			if (image) {
				xsg_message("Loaded image \"%s\"", file);
				xsg_free(file);
				return image;
			} else {
				switch (error) {
					case IMLIB_LOAD_ERROR_NONE:
						error_str = "IMLIB_LOAD_ERROR_NONE";
						break;
					case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST:
						error_str = "IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST";
						break;
					case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY:
						error_str = "IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY";
						break;
					case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ:
						error_str = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ";
						break;
					case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT:
						error_str = "IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT";
						break;
					case IMLIB_LOAD_ERROR_PATH_TOO_LONG:
						error_str = "IMLIB_LOAD_ERROR_PATH_TOO_LONG";
						break;
					case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT:
						error_str = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT";
						break;
					case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY:
						error_str = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY";
						break;
					case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE:
						error_str = "IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE";
						break;
					case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS:
						error_str = "IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS";
						break;
					case IMLIB_LOAD_ERROR_OUT_OF_MEMORY:
						error_str = "IMLIB_LOAD_ERROR_OUT_OF_MEMORY";
						break;
					case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS:
						error_str = "IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS";
						break;
					case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE:
						error_str = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE";
						break;
					case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE:
						error_str = "IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE";
						break;
					case IMLIB_LOAD_ERROR_UNKNOWN:
					default:
						error_str = "IMLIB_LOAD_ERROR_UNKNOWN";
						break;
				}
				xsg_warning("Loading image \"%s\" failed: %s", file, error_str);
			}
		}

		xsg_free(file);
	}

	return NULL;
}

Imlib_Color xsg_imlib_uint2color(uint32_t u) {
	Imlib_Color color;

	color.alpha = A_VAL(&u);
	color.red = R_VAL(&u);
	color.green = G_VAL(&u);
	color.blue = B_VAL(&u);

	return color;
}

void xsg_imlib_blend_mask(Imlib_Image mask) {
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

