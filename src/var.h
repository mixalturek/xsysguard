/* var.h
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

#ifndef __VAR_H__
#define __VAR_H__ 1

#include <xsysguard.h>

#include "types.h"

/******************************************************************************/

void xsg_var_dirty(xsg_var_t *var);
void xsg_var_flush_dirty(void);

/******************************************************************************/

xsg_var_t *xsg_var_parse(xsg_window_t *window, xsg_widget_t *widget, uint64_t update);

double xsg_var_get_num(xsg_var_t *var);
char *xsg_var_get_str(xsg_var_t *var);

/*****************************************************************************/

#endif /* __VAR_H__ */

