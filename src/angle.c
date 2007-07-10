/* angle.c
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

#include <math.h>

#include "angle.h"

/******************************************************************************/

static void max_area(double sa, double ca, double *width, double *height) {
	double ow, oh;
	unsigned w, h;
	unsigned max;
	unsigned area;

	area = 0;

	ow = *width;
	oh = *height;

	max = MAX(*width, *height);

	for (w = 1; w <= max; w++) {
		for (h = 1; h <= max; h++) {
			double nw, nh;

			nw = fabs(sa * (double) h) + fabs(ca * (double) w);
			nh = fabs(ca * (double) h) + fabs(sa * (double) w);

			if (nw <= ow && nh <= oh && w * h > area) {
				area = w * h;
				*width = (double) w;
				*height = (double) h;
			}
		}
	}
}

xsg_angle_t *xsg_angle_parse(double a, int xoffset, int yoffset, unsigned width, unsigned height) {
	xsg_angle_t *angle;
	double arc, sa, ca, w, h;

	arc = fmod(a, 360.0) / 180.0 * M_PI;

	sa = sin(arc);
	ca = cos(arc);

	w = width;
	h = height;

	max_area(sa, ca, &w, &h);

	xoffset += (width - (int) (fabs(sa * h) + fabs(ca * w))) / 2;
	yoffset += (height - (int) (fabs(ca * h) + fabs(sa * w))) / 2;

	if (sa > 0.0)
		xoffset += sa * h;
	else
		yoffset -= sa * w;

	if (ca < 0.0) {
		xoffset -= ca * w;
		yoffset -= ca * h;
	}

	angle = xsg_new(xsg_angle_t, 1);

	angle->xoffset = xoffset;
	angle->yoffset = yoffset;
	angle->width = w;
	angle->height = h;
	angle->angle = a;
	angle->angle_x = w * ca;
	angle->angle_y = w * sa;
	return angle;
}

