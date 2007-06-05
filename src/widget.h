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

#include "window.h"

/*****************************************************************************/

struct _xsg_widget_t {
	xsg_window_t *window;

	uint64_t update;

	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;

	uint64_t visible_update;
	xsg_var_t *visible_var;
	bool visible;

	void (*render_func)(xsg_widget_t *widget, Imlib_Image buffer, int x, int y);
	void (*update_func)(xsg_widget_t *widget, xsg_var_t *var);
	void (*scroll_func)(xsg_widget_t *widget);

	void *data;
};

/******************************************************************************/

#endif /* __WIDGET_H__ */

