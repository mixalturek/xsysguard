/* widget_areachart.h
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

#ifndef __WIDGET_AREACHART_H__
#define __WIDGET_AREACHART_H__ 1

#include <xsysguard.h>

/*****************************************************************************/

void xsg_widget_areachart_parse(uint32_t window_id, uint64_t *update, uint32_t *widget_id);
void xsg_widget_areachart_parse_var(uint32_t var_id);

/******************************************************************************/

#endif // __WIDGET_AREACHART_H__

