/* window.h
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

#ifndef __WINDOW_H__
#define __WINDOW_H__ 1

#include <xsysguard.h>

#include "types.h"

/*****************************************************************************/

extern void
xsg_window_init(void);

extern bool
xsg_window_color_lookup(char *name, uint32_t *color);

extern char *
xsg_window_extract_position(
	char *str,
	int *flags_return,
	int *x_return,
	int *y_return
);

extern void
xsg_window_update(uint64_t tick);

extern void
xsg_window_update_var(
	xsg_window_t *window,
	xsg_widget_t *widget,
	xsg_var_t *var
);

/*****************************************************************************/

extern xsg_window_t *
xsg_window_new(char *config_name, int flags, int xoffset, int yoffset);

/******************************************************************************/

extern void
xsg_window_parse_name(xsg_window_t *window);

extern void
xsg_window_parse_class(xsg_window_t *window);

extern void
xsg_window_parse_resource(xsg_window_t *window);

extern void
xsg_window_parse_size(xsg_window_t *window);

extern void
xsg_window_parse_position(xsg_window_t *window);

extern void
xsg_window_parse_sticky(xsg_window_t *window);

extern void
xsg_window_parse_skip_taskbar(xsg_window_t *window);

extern void
xsg_window_parse_skip_pager(xsg_window_t *window);

extern void
xsg_window_parse_layer(xsg_window_t *window);

extern void
xsg_window_parse_decorations(xsg_window_t *window);

extern void
xsg_window_parse_override_redirect(xsg_window_t *window);

extern void
xsg_window_parse_background(xsg_window_t *window);

extern void
xsg_window_parse_grab_root(xsg_window_t *window);

extern void
xsg_window_parse_xshape(xsg_window_t *window);

extern void
xsg_window_parse_argb_visual(xsg_window_t *window);

extern void
xsg_window_parse_visible(xsg_window_t *window);

extern void
xsg_window_parse_mouse(xsg_window_t *window);

/*****************************************************************************/

extern void
xsg_window_render(void);

extern void
xsg_window_update_append_rect(
	xsg_window_t *window,
	int xoffset,
	int yoffset,
	int width,
	int height
);

/******************************************************************************/

extern void
xsg_window_add_widget(xsg_window_t *window, xsg_widget_t *widget);

extern char *
xsg_window_get_config_name(xsg_window_t *window);

/******************************************************************************/

#endif /* __WINDOW_H__ */

