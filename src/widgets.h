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
#include <Imlib2.h>

#include "types.h"

/******************************************************************************/

void xsg_widgets_update(uint64_t tick);

void xsg_widgets_update_var(xsg_widget_t *widget, xsg_var_t *var);

xsg_widget_t *xsg_widgets_new(xsg_window_t *window);

xsg_widget_t *xsg_widgets_last(void);

void xsg_widgets_render(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y, int up_w, int up_h);

/******************************************************************************/

#endif /* __WIDGETS_H__ */

