/* writebuffer.h
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

#ifndef __WRITEBUFFER_H__
#define __WRITEBUFFER_H__ 1

#include <xsysguard.h>

/******************************************************************************/

bool xsg_writebuffer_ready(void);
void xsg_writebuffer_flush(void);

/******************************************************************************/

void xsg_writebuffer_queue_num(uint32_t id, double num);
void xsg_writebuffer_queue_str(uint32_t id, xsg_string_t *str);

void xsg_writebuffer_queue_alive(void);

void xsg_writebuffer_queue_log(uint8_t level, const char *message, size_t len);

/*****************************************************************************/

#endif /* __WRITEBUFFER_H__ */

