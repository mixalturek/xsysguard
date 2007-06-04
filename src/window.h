/* window.h
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

#ifndef __WINDOW_H__
#define __WINDOW_H__ 1

#include <xsysguard.h>

/*****************************************************************************/

void xsg_window_init(void);

void xsg_window_update(uint32_t window_id, uint32_t widget_id, uint32_t var_id);

/*****************************************************************************/

void xsg_window_set_cache_size(int size);
void xsg_window_set_font_cache_size(int size);

/******************************************************************************/

uint32_t xsg_window_new(char *config);

/******************************************************************************/

void xsg_window_parse_name(uint32_t window_id);
void xsg_window_parse_class(uint32_t window_id);
void xsg_window_parse_resource(uint32_t window_id);
void xsg_window_parse_geometry(uint32_t window_id);
void xsg_window_parse_sticky(uint32_t window_id);
void xsg_window_parse_skip_taskbar(uint32_t window_id);
void xsg_window_parse_skip_pager(uint32_t window_id);
void xsg_window_parse_layer(uint32_t window_id);
void xsg_window_parse_decorations(uint32_t window_id);
void xsg_window_parse_override_redirect(uint32_t window_id);
void xsg_window_parse_background(uint32_t window_id);
void xsg_window_parse_grab_root(uint32_t window_id);
void xsg_window_parse_xshape(uint32_t window_id);
void xsg_window_parse_argb_visual(uint32_t window_id);
void xsg_window_parse_visible(uint32_t window_id);

/*****************************************************************************/

void xsg_window_render(void);
void xsg_window_render_xshape(void);

void xsg_window_update_append_rect(uint32_t window_id, int xoffset, int yoffset, int width, int height);

/******************************************************************************/

void xsg_window_add_widget(uint32_t window_id, void *widget);

char *xsg_window_get_config_name(uint32_t window_id);

/******************************************************************************/

#endif /* __WINDOW_H__ */

