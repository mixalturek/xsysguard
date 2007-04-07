/* conf.h
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

#ifndef __CONF_H__
#define __CONF_H__ 1

#include <xsysguard.h>

/******************************************************************************/

//bool xsg_conf_read_boolean(void);
//int64_t xsg_conf_read_int(void);
//uint64_t xsg_conf_read_uint(void);
//double xsg_conf_read_double(void);
//char *xsg_conf_read_string(void);

//bool xsg_conf_find_command(const char *command);

//void xsg_conf_error(const char *expected);

/******************************************************************************/

void xsg_conf_set_buffer(char *buffer);

bool xsg_conf_find_comment(void);
bool xsg_conf_find_newline(void);
bool xsg_conf_find_end(void);

void xsg_conf_read_newline(void);
uint32_t xsg_conf_read_color(void);

/******************************************************************************/

#endif /* __CONF_H__ */

