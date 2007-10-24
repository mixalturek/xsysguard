/* imlib.h
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

#ifndef __IMLIB_H__
#define __IMLIB_H__ 1

#include <xsysguard.h>
#include <Imlib2.h>

/******************************************************************************/

void xsg_imlib_init(void);
void xsg_imlib_init_font_path(bool enable_fontconfig);
void xsg_imlib_set_cache_size(int size);
void xsg_imlib_set_font_cache_size(int size);

void xsg_imlib_list_fonts(void);

void xsg_imlib_flush_cache(int signum);

Imlib_Color xsg_imlib_uint2color(uint32_t u);
Imlib_Image xsg_imlib_load_image(const char *filename);
void xsg_imlib_blend_mask(Imlib_Image mask);
void xsg_imlib_blend_background(const char *bg, int x, int y, unsigned w, unsigned h, int orientation, uint64_t update);
Imlib_Image xsg_imlib_create_color_range_image(unsigned width, unsigned height, Imlib_Color_Range range, double range_angle);

void xsg_imlib_text_draw_with_return_metrics(int x, int y, const char *text, int *width_return, int *height_return, int *horizontal_advance_return, int *vertical_advance_return);
void xsg_imlib_text_draw(int x, int y, const char *text);

/******************************************************************************/

#endif /* __IMLIB_H__ */

