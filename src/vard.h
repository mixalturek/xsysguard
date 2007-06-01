/* vard.h
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

#ifndef __VARD_H__
#define __VARD_H__ 1

#include <xsysguard.h>

/******************************************************************************/

void xsg_var_init(void);

void xsg_var_dirty(uint32_t var_id);
void xsg_var_flush_dirty(void);

/******************************************************************************/

void xsg_var_parse(uint32_t id, uint64_t update, uint8_t type);

void xsg_var_queue_vars(void);

/*****************************************************************************/

#endif /* __VAR_H__ */
