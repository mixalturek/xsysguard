/* xsysguard.h
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

#ifndef __XSYSGUARD_H__
#define __XSYSGUARD_H__ 1

#define _GNU_SOURCE 1

#include <unistd.h>
#include <inttypes.h>

#include <glib.h>

/******************************************************************************/

#define TRUE (!FALSE)
#define FALSE (0)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define XSG_INT		0x01
#define XSG_DOUBLE	0x02
#define XSG_STRING	0x03

#define XSG_MODULES_PARSE_FUNC "parse"
#define XSG_MODULES_INFO_FUNC "info"

/******************************************************************************/

typedef int bool;

typedef struct _xsg_list xsg_list;
typedef struct _xsg_string xsg_string;
typedef struct _xsg_var xsg_var;

struct _xsg_list {
	void *data;
	xsg_list *next;
	xsg_list *prev;
};

struct _xsg_string {
	char *str;
	size_t len;
	size_t allocated_len;
};

struct _xsg_var {
	uint8_t type;
	void *(*func)(void *args);
	void *args;
};

typedef void (*xsg_modules_parse_func)(xsg_var *var, uint16_t id, uint64_t update);
typedef char *(*xsg_modules_info_func)(void);

/******************************************************************************/

void parse(xsg_var *var, uint16_t id, uint64_t update);
char *info(void);

/******************************************************************************
 * conf.c
 ******************************************************************************/

bool xsg_conf_read_boolean();
int64_t xsg_conf_read_int();
uint64_t xsg_conf_read_uint();
double xsg_conf_read_double();
char *xsg_conf_read_string();

bool xsg_conf_find_command(const char *command);

void xsg_conf_error(const char *expected);

/******************************************************************************
 * var.c
 ******************************************************************************/

void xsg_var_set_type(uint16_t id, uint8_t type);
void xsg_var_update(uint16_t id);

/******************************************************************************
 * main.c
 ******************************************************************************/

void xsg_main_add_init_func(void (*func)(void));
void xsg_main_add_update_func(void (*func)(uint64_t));
void xsg_main_add_shutdown_func(void (*func)(void));

/******************************************************************************
 * utils.c
 ******************************************************************************/

xsg_list *xsg_list_append(xsg_list *list, void *data);
xsg_list *xsg_list_prepend(xsg_list *list, void *data);
xsg_list *xsg_list_last(xsg_list *list);
unsigned int xsg_list_length(xsg_list *list);
void *xsg_list_nth_data(xsg_list *list, unsigned int n);

xsg_string *xsg_string_new(const char *init);
xsg_string *xsg_string_sized_new(size_t dfl_size);
xsg_string *xsg_string_assign(xsg_string *string, const char *rval);
xsg_string *xsg_string_truncate(xsg_string *string, size_t len);
xsg_string *xsg_string_set_size(xsg_string *string, ssize_t len);
xsg_string *xsg_string_append(xsg_string *string, const char *val);
xsg_string *xsg_string_append_len(xsg_string *string, const char *val, ssize_t len);
xsg_string *xsg_string_insert_len(xsg_string *string, ssize_t pos, const char *val, ssize_t len);
void xsg_string_printf(xsg_string *string, const char *format, ...);
char *xsg_string_free(xsg_string *string, bool free_segment);

/******************************************************************************/

#endif /* __XSYSGUARD_H__ */

