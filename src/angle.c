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

xsg_angle_t *xsg_angle_parse(double a, int xoffset, int yoffset, unsigned int *width, unsigned int *height) {
	xsg_angle_t *angle;
	double arc, sa, ca;
	unsigned int w, h;

	arc = a / 180.0 * M_PI;

	sa = sin(arc);
	ca = cos(arc);

	w = *width;
	h = *height;

	if (sa > 0.0)
		xoffset += sa * h;
	else
		yoffset -= sa * w;

	if (ca < 0.0) {
		xoffset -= ca * w;
		yoffset -= ca * h;
	}

	angle = xsg_new0(xsg_angle_t, 1);

	angle->xoffset = xoffset;
	angle->yoffset = yoffset;
	angle->width = *width;
	angle->height = *height;
	angle->angle = a;
	angle->angle_x = w * ca;
	angle->angle_y = w * sa;

	*width = ceil(fabs(sa * h) + fabs(ca * w));
	*height = ceil(fabs(ca * h) + fabs(sa *w));

	return angle;
}

