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

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/time.h>

/******************************************************************************/

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

#define DNAN ((double)(0.0/0.0))
#define DINF ((double)(1.0/0.0))

/******************************************************************************/

typedef int bool;

/******************************************************************************/

typedef void xsg_modules_parse_t(uint32_t id, uint64_t update, double (**n)(void *), char *(**s)(void *), void **arg);
typedef char *xsg_modules_info_t(void);

xsg_modules_parse_t parse;
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

void xsg_var_dirty(uint32_t id);

/******************************************************************************
 * main.c
 ******************************************************************************/

uint64_t xsg_main_get_interval(void);
uint64_t xsg_main_get_tick(void);

void xsg_main_add_init_func(void (*func)(void));
void xsg_main_add_update_func(void (*func)(uint64_t tick));
void xsg_main_add_shutdown_func(void (*func)(void));
void xsg_main_add_signal_handler(void (*func)(int signum));

typedef enum _xsg_main_poll_events_t {
	XSG_MAIN_POLL_READ   = 1 << 0,
	XSG_MAIN_POLL_WRITE  = 1 << 1,
	XSG_MAIN_POLL_EXCEPT = 1 << 2
} xsg_main_poll_events_t;

typedef struct _xsg_main_poll_t {
	int fd;
	xsg_main_poll_events_t events;
	void (*func)(void *arg, xsg_main_poll_events_t events);
	void *arg;
} xsg_main_poll_t;

void xsg_main_add_poll(xsg_main_poll_t *poll);
void xsg_main_remove_poll(xsg_main_poll_t *poll);

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
xsg_list_t *xsg_list_remove(xsg_list_t *list, const void *data);
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
xsg_string_t *xsg_string_append_c(xsg_string_t *string, char c);
xsg_string_t *xsg_string_append_len(xsg_string_t *string, const char *val, ssize_t len);
xsg_string_t *xsg_string_insert_len(xsg_string_t *string, ssize_t pos, const char *val, ssize_t len);
xsg_string_t *xsg_string_insert_c(xsg_string_t *string, ssize_t pos, char c);
void xsg_string_printf(xsg_string_t *string, const char *format, ...);
void xsg_string_append_printf(xsg_string_t *string, const char *format, ...);
char *xsg_string_free(xsg_string_t *string, bool free_segment);

/******************************************************************************
 * utils.c
 ******************************************************************************/

/* strfuncs */
bool xsg_str_has_suffix(const char *str, const char *suffix);
char *xsg_str_without_suffix(const char *str, const char *suffix);
char **xsg_strsplit_set(const char *string, const char *delimiters, int max_tokens);
void xsg_strfreev(char **str_array);
char *xsg_strdup(const char *str);
char *xsg_strndup(const char *str, size_t n);

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

int xsg_vasprintf(char **strp, const char *fmt, va_list ap);
int xsg_asprintf(char **strp, const char *fmt, ...);

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

char **xsg_get_path_from_env(const char *env_name, const char *default_path);

int xsg_timeval_sub(struct timeval *result, struct timeval *x, struct timeval *y);
int xsg_gettimeofday(struct timeval *tv, void *tz);

/******************************************************************************
 * logging
 ******************************************************************************/

#ifndef XSG_LOG_DOMAIN
#define XSG_LOG_DOMAIN ((char *) 0)
#endif /* XSG_LOG_DOMAIN */

typedef enum {
	XSG_LOG_LEVEL_ERROR   = 1,
	XSG_LOG_LEVEL_WARNING = 2,
	XSG_LOG_LEVEL_MESSAGE = 3,
	XSG_LOG_LEVEL_DEBUG   = 4,
	XSG_LOG_LEVEL_MEM     = 5
} xsg_log_level_t;

extern xsg_log_level_t xsg_log_level;

void xsg_log(const char *domain, xsg_log_level_t level, const char *format, ...);

#define xsg_error(...) xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_ERROR, __VA_ARGS__)
#define xsg_warning(...) likely(xsg_log_level < XSG_LOG_LEVEL_WARNING) ? : xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_WARNING, __VA_ARGS__)
#define xsg_message(...) likely(xsg_log_level < XSG_LOG_LEVEL_MESSAGE) ? : xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_MESSAGE, __VA_ARGS__)
#define xsg_debug(...) likely(xsg_log_level < XSG_LOG_LEVEL_DEBUG) ? : xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_DEBUG, __VA_ARGS__)

/******************************************************************************/

#endif /* __XSYSGUARD_H__ */

