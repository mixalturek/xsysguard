/* argb.h
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

#ifndef __ARGB_H__
#define __ARGB_H__ 1

#include <xsysguard.h>

/******************************************************************************/

#define A_VAL(p) ((uint8_t *)(p))[xsg_argb_am_big_endian() ? 0 : 3]
#define R_VAL(p) ((uint8_t *)(p))[xsg_argb_am_big_endian() ? 1 : 2]
#define G_VAL(p) ((uint8_t *)(p))[xsg_argb_am_big_endian() ? 2 : 1]
#define B_VAL(p) ((uint8_t *)(p))[xsg_argb_am_big_endian() ? 3 : 0]

/******************************************************************************/

static bool
xsg_argb_am_big_endian(void)
{
	long one = 1;
	return !(*((char *)(&one)));
}

/******************************************************************************/

#endif /* __ARGB_H__ */

