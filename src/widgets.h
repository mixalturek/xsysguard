/* widgets.h
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

#ifndef __WIDGETS_H__
#define __WIDGETS_H__ 1

#include <xsysguard.h>

/*****************************************************************************/

void xsg_widgets_init();
void xsg_widgets_update(uint32_t widget_id, uint32_t var_id);

/*****************************************************************************/

void xsg_widgets_parse_name(void);
void xsg_widgets_parse_class(void);
void xsg_widgets_parse_resource(void);
void xsg_widgets_parse_geometry(void);
void xsg_widgets_parse_sticky(void);
void xsg_widgets_parse_skip_taskbar(void);
void xsg_widgets_parse_skip_pager(void);
void xsg_widgets_parse_layer(void);
void xsg_widgets_parse_decorations(void);
void xsg_widgets_parse_background(void);
void xsg_widgets_parse_grab_root(void);
void xsg_widgets_parse_cache_size(void);
void xsg_widgets_parse_xshape(void);
void xsg_widgets_parse_argb_visual(void);

/*****************************************************************************/

void xsg_widgets_parse_line(void);
void xsg_widgets_parse_rectangle(void);
void xsg_widgets_parse_ellipse(void);
void xsg_widgets_parse_polygon(void);
void xsg_widgets_parse_image(void);
void xsg_widgets_parse_barchart(uint64_t *update, uint32_t *widget_id);
void xsg_widgets_parse_barchart_val(uint32_t var_id);
void xsg_widgets_parse_linechart(uint64_t *update, uint32_t *widget_id);
void xsg_widgets_parse_linechart_val(uint32_t var_id);
void xsg_widgets_parse_areachart(uint64_t *update, uint32_t *widget_id);
void xsg_widgets_parse_areachart_val(uint32_t var_id);
void xsg_widgets_parse_text(uint64_t *update, uint32_t *widget_id);
void xsg_widgets_parse_text_val(uint32_t var_id);
void xsg_widgets_parse_imagetext(uint64_t *update, uint32_t *widget_id);
void xsg_widgets_parse_imagetext_val(uint32_t var_id);

/******************************************************************************/

#endif /* __WIDGETS_H__ */

