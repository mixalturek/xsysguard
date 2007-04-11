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

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>

/******************************************************************************/

#if defined(__GNUC__) && (__GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96))
#pragma GCC system_header
#endif

#if defined(__GNUC__) && (__GNUC__ > 2)
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

/******************************************************************************/

#define TRUE (!FALSE)
#define FALSE (0)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ABS(a) (((a) < 0) ? -(a) : (a))

#define ISNAN(d) (((d) != (d)) ? TRUE : FALSE)

#define DNAN ((double)(0.0/0.0))
#define DINF ((double)(1.0/0.0))

/******************************************************************************/

typedef int bool;

/******************************************************************************/

typedef void xsg_modules_parse_double_t(uint32_t id, uint64_t update, double (**func)(void *), void **arg);
typedef void xsg_modules_parse_string_t(uint32_t id, uint64_t update, char * (**func)(void *), void **arg);
typedef char *xsg_modules_info_t(void);

xsg_modules_parse_double_t parse_double;
xsg_modules_parse_string_t parse_string;
xsg_modules_info_t info;

/******************************************************************************
 * conf.c
 ******************************************************************************/

bool xsg_conf_read_boolean(void);
int64_t xsg_conf_read_int(void);
uint64_t xsg_conf_read_uint(void);
double xsg_conf_read_double(void);
char *xsg_conf_read_string(void);

bool xsg_conf_find_command(const char *command);

void xsg_conf_error(const char *expected);

/******************************************************************************
 * var.c
 ******************************************************************************/

void xsg_var_update(uint32_t id);

/******************************************************************************
 * main.c
 ******************************************************************************/

uint64_t xsg_main_get_counter(void);

void xsg_main_add_init_func(void (*func)(void));
void xsg_main_add_update_func(void (*func)(uint64_t));
void xsg_main_add_shutdown_func(void (*func)(void));

typedef enum {
	XSG_MAIN_POLL_READ   = 1 << 0,
	XSG_MAIN_POLL_WRITE  = 1 << 1,
	XSG_MAIN_POLL_EXCEPT = 1 << 2,
} xsg_main_poll_t;

void xsg_main_add_poll_func(int fd, void (*func)(void *, xsg_main_poll_t), void *arg, xsg_main_poll_t events);
void xsg_main_remove_poll_func(int fd, void (*func)(void *, xsg_main_poll_t), void *arg, xsg_main_poll_t events);

/******************************************************************************
 * list.c
 ******************************************************************************/

typedef struct _xsg_list_t xsg_list_t;

struct _xsg_list_t {
	void *data;
	xsg_list_t *next;
	xsg_list_t *prev;
};

xsg_list_t *xsg_list_append(xsg_list_t *list, void *data);
xsg_list_t *xsg_list_prepend(xsg_list_t *list, void *data);
xsg_list_t *xsg_list_last(xsg_list_t *list);
unsigned int xsg_list_length(xsg_list_t *list);
void *xsg_list_nth_data(xsg_list_t *list, unsigned int n);
void xsg_list_free(xsg_list_t *list);
xsg_list_t *xsg_list_delete_link(xsg_list_t *list, xsg_list_t *link);

/******************************************************************************
 * string.c
 ******************************************************************************/

typedef struct _xsg_string_t xsg_string_t;

struct _xsg_string_t {
	char *str;
	size_t len;
	size_t allocated_len;
};

xsg_string_t *xsg_string_new(const char *init);
xsg_string_t *xsg_string_sized_new(size_t dfl_size);
xsg_string_t *xsg_string_assign(xsg_string_t *string, const char *rval);
xsg_string_t *xsg_string_truncate(xsg_string_t *string, size_t len);
xsg_string_t *xsg_string_set_size(xsg_string_t *string, ssize_t len);
xsg_string_t *xsg_string_append(xsg_string_t *string, const char *val);
xsg_string_t *xsg_string_append_len(xsg_string_t *string, const char *val, ssize_t len);
xsg_string_t *xsg_string_insert_len(xsg_string_t *string, ssize_t pos, const char *val, ssize_t len);
void xsg_string_printf(xsg_string_t *string, const char *format, ...);
char *xsg_string_free(xsg_string_t *string, bool free_segment);

/******************************************************************************
 * utils.c
 ******************************************************************************/

/* strfuncs */
bool xsg_str_has_suffix(const char *str, const char *suffix);
char *xsg_str_without_suffix(const char *str, const char *suffix);
char **xsg_strsplit_set(const char *string, const char *delimiters, int max_tokens);
void xsg_strfreev(char **str_array);

/* byte order */
uint16_t xsg_uint16_be(uint16_t u);
uint16_t xsg_uint16_le(uint16_t u);
uint32_t xsg_uint32_be(uint32_t u);
uint32_t xsg_uint32_le(uint32_t u);
uint64_t xsg_uint64_be(uint64_t u);
uint64_t xsg_uint64_le(uint64_t u);
double xsg_double_be(double d);
double xsg_double_le(double d);

/* mem */
void *xsg_malloc(size_t size);
void *xsg_malloc0(size_t size);
void *xsg_realloc(void *mem, size_t size);
void xsg_free(void *mem);

#define xsg_new(struct_type, n_structs) \
	((struct_type *) xsg_malloc(((size_t) sizeof(struct_type)) * ((size_t) (n_structs))))
#define xsg_new0(struct_type, n_structs) \
	((struct_type *) xsg_malloc0(((size_t) sizeof(struct_type)) * ((size_t) (n_structs))))
#define xsg_renew(struct_type, mem, n_structs) \
	((struct_type *) xsg_realloc((mem), ((size_t) sizeof(struct_type)) * ((size_t) (n_structs))))

/* misc */
char *xsg_build_filename(const char *first_element, ...);
const char *xsg_get_home_dir(void);

typedef enum {
	XSG_FILE_TEST_IS_REGULAR    = 1 << 0,
	XSG_FILE_TEST_IS_SYMLINK    = 1 << 1,
	XSG_FILE_TEST_IS_DIR        = 1 << 2,
	XSG_FILE_TEST_IS_EXECUTABLE = 1 << 3,
	XSG_FILE_TEST_EXISTS        = 1 << 4
} xsg_file_test_t;

bool xsg_file_test(const char *filename, xsg_file_test_t test);

/******************************************************************************
 * logging
 ******************************************************************************/

#ifndef LOG_DOMAIN
#define LOG_DOMAIN ((char *) 0)
#endif /* LOG_DOMAIN */

#define LOG_LEVEL_ERROR   0x00
#define LOG_LEVEL_WARNING 0x01
#define LOG_LEVEL_MESSAGE 0x02
#define LOG_LEVEL_DEBUG   0x03

void xsg_log(const char *domain, uint32_t level, const char *format, va_list args);

static void xsg_error(const char *format, ...) {
	va_list args;
	va_start(args, format);
	xsg_log(LOG_DOMAIN, LOG_LEVEL_ERROR, format, args);
	va_end(args);
	abort();
}
static void xsg_warning(const char *format, ...) {
	va_list args;
	va_start(args, format);
	xsg_log(LOG_DOMAIN, LOG_LEVEL_WARNING, format, args);
	va_end(args);
}
static void xsg_message(const char *format, ...) {
	va_list args;
	va_start(args, format);
	xsg_log(LOG_DOMAIN, LOG_LEVEL_MESSAGE, format, args);
	va_end(args);
}
static void xsg_debug(const char *format, ...) {
	va_list args;
	va_start(args, format);
	xsg_log(LOG_DOMAIN, LOG_LEVEL_DEBUG, format, args);
	va_end(args);
}

/******************************************************************************/

#endif /* __XSYSGUARD_H__ */

