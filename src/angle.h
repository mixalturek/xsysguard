/* angle.h
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

#ifndef __ANGLE_H__
#define __ANGLE_H__ 1

#include <xsysguard.h>

/******************************************************************************/

typedef struct _xsg_angle_t {
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	double angle;
	int angle_x;
	int angle_y;
} xsg_angle_t;

/******************************************************************************/

xsg_angle_t *xsg_angle_parse(double a, int xoffset, int yoffset, unsigned width, unsigned height);

/******************************************************************************/

#endif /* __ANGLE_H__ */

