/* widget_barchart.h
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

#ifndef __WIDGET_BARCHART_H__
#define __WIDGET_BARCHART_H__ 1

#include <xsysguard.h>

#include "types.h"

/*****************************************************************************/

extern xsg_widget_t *
xsg_widget_barchart_parse(xsg_window_t *window, uint64_t *update);

extern void
xsg_widget_barchart_parse_var(xsg_var_t *var);

/******************************************************************************/

#endif /* __WIDGET_BARCHART_H__ */

