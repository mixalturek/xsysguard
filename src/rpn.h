/* rpn.h
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
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

#ifndef __RPN_H__
#define __RPN_H__ 1

#include <xsysguard.h>

#include "types.h"

/******************************************************************************/

void xsg_rpn_init(void);
void xsg_rpn_parse(uint64_t update, xsg_var_t *const *var, xsg_rpn_t **rpn, uint32_t n);
double xsg_rpn_get_num(xsg_rpn_t *rpn);
char *xsg_rpn_get_str(xsg_rpn_t *rpn);

/*****************************************************************************/

#endif /* __RPN_H__ */

