/* conf.h
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

#ifndef __CONF_H__
#define __CONF_H__ 1

#include <xsysguard.h>

/******************************************************************************/

extern void
xsg_conf_set_buffer(char *name, char *buffer);

extern void
xsg_conf_set_color_lookup(bool (*func)(char *name, uint32_t *color));

/******************************************************************************/

extern bool
xsg_conf_find_commentline(void);

extern bool
xsg_conf_find_newline(void);

extern bool
xsg_conf_find_comma(void);

extern bool
xsg_conf_find_end(void);

/******************************************************************************/

extern bool
xsg_conf_find_number(double *number_return);

extern bool
xsg_conf_find_string(char **string_return);

/******************************************************************************/

extern bool
xsg_conf_read_offset(int *offset);

extern void
xsg_conf_read_newline(void);

extern uint32_t
xsg_conf_read_color(void);

extern uint64_t
xsg_conf_read_update(void);

/******************************************************************************/

#endif /* __CONF_H__ */

