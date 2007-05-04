/* widget.h
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

#ifndef __WIDGET_H__
#define __WIDGET_H__ 1

#include <xsysguard.h>
#include <Imlib2.h>

/*****************************************************************************/

#ifndef TRACE
# define T(func) func
#else
# define T(func) { \
	struct timeval timeval_begin, timeval_end, timeval_diff; \
	xsg_gettimeofday(&timeval_begin, NULL); \
	func; \
	xsg_gettimeofday(&timeval_end, NULL); \
	xsg_timeval_sub(&timeval_diff, &timeval_end, &timeval_begin); \
	xsg_debug("T %u.%06us [%04d] %s: %s", \
			(unsigned) timeval_diff.tv_sec, \
			(unsigned) timeval_diff.tv_usec, \
			__LINE__, \
			__FUNCTION__, \
			#func); \
}
#endif

/******************************************************************************/

typedef struct _xsg_widget_t xsg_widget_t;

struct _xsg_widget_t {
	uint64_t update;
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	void (*render_func)(xsg_widget_t *widget, Imlib_Image buffer, int x, int y);
	void (*update_func)(xsg_widget_t *widget, uint32_t var_id);
	void (*scroll_func)(xsg_widget_t *widget);
	void *data;
};

/******************************************************************************/

typedef struct _xsg_widget_angle_t xsg_widget_angle_t;

struct _xsg_widget_angle_t {
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	double angle;
	int angle_x;
	int angle_y;
};

/******************************************************************************/

#endif /* __WIDGET_H__ */

