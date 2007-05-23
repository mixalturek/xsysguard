/* rpn.h
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

#ifndef __RPN_H__
#define __RPN_H__ 1

#include <xsysguard.h>

/******************************************************************************/

void xsg_rpn_init(void);
uint32_t xsg_rpn_parse(uint32_t var_id, uint64_t update);
double xsg_rpn_get_num(uint32_t rpn_id);
char *xsg_rpn_get_str(uint32_t rpn_id);

/*****************************************************************************/

#endif /* __RPN_H__ */

