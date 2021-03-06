/* xsysguard.h
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

#ifndef __XSYSGUARD_H__
#define __XSYSGUARD_H__ 1

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sys/time.h>

/******************************************************************************/

#if defined(__GNUC__) && (__GNUC__ > 2)
# define likely(x) __builtin_expect((x), 1)
# define unlikely(x) __builtin_expect((x), 0)
#else
# define likely(x) (x)
# define unlikely(x) (x)
#endif

#ifndef XSG_API
# if defined(__GNUC_MINOR__) && (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2)
#  define XSG_API __attribute__ ((__externally_visible__))
# else
#  define XSG_API
# endif
#endif

#if defined(__GNUC__)
# define XSG_PRINTF(a, b) __attribute__((__format__(__printf__, a, b)))
#endif

#if defined(__GNUC__) && (__GNUC__ < 3)
# if !defined va_copy && defined __va_copy
#  define va_copy(d, s) __va_copy((d), (s))
# endif
#endif

#ifdef __FAST_MATH__
# warning xsysguard depends on -fno-finite-math-only
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

typedef struct _xsg_var_t xsg_var_t;

/******************************************************************************/

typedef struct _xsg_module_t {
	void (*parse)(
		uint64_t update,
		xsg_var_t *var,
		double (**num)(void *),
		const char *(**str)(void *),
		void **arg
	);
	const char *(*help)(void);
	const char *info;
	const char *name; /* replaced by xsysguard */
} xsg_module_t;

extern XSG_API xsg_module_t xsg_module;

#ifndef XSG_MODULE
# define XSG_MODULE(parse, help, info) \
	xsg_module_t xsg_module = { parse, help, info }
#endif

#ifndef XSG_MODULE_NAME
# define XSG_MODULE_NAME xsg_module.name
#endif

/******************************************************************************
 * conf.c
 ******************************************************************************/

extern XSG_API bool
xsg_conf_read_boolean(void);

extern XSG_API int64_t
xsg_conf_read_int(void);

extern XSG_API uint64_t
xsg_conf_read_uint(void);

extern XSG_API double
xsg_conf_read_double(void);

extern XSG_API char *
xsg_conf_read_string(void);

extern XSG_API bool
xsg_conf_find_command(const char *command);

extern XSG_API XSG_PRINTF(1, 2) void
xsg_conf_error(const char *format, ...);

extern XSG_API XSG_PRINTF(1, 2) void
xsg_conf_warning(const char *format, ...);

/******************************************************************************
 * var.c
 ******************************************************************************/

extern XSG_API void
xsg_var_dirty(xsg_var_t *var);

/******************************************************************************
 * main.c
 ******************************************************************************/

extern XSG_API uint64_t
xsg_main_get_interval(void);

extern XSG_API uint64_t
xsg_main_get_tick(void);

extern XSG_API void
xsg_main_add_init_func(void (*func)(void));

extern XSG_API void
xsg_main_add_shutdown_func(void (*func)(void));

extern XSG_API void
xsg_main_add_update_func(void (*func)(uint64_t tick));

extern XSG_API void
xsg_main_add_signal_handler(void (*func)(int signum), int signum);

typedef struct _xsg_main_timeout_t {
	struct timeval tv; /* absolute time */
	void (*func)(void *arg, bool time_error);
	void *arg;
} xsg_main_timeout_t;

extern XSG_API void
xsg_main_add_timeout(xsg_main_timeout_t *timeout);

extern XSG_API void
xsg_main_remove_timeout(xsg_main_timeout_t *timeout);

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

extern XSG_API void
xsg_main_add_poll(xsg_main_poll_t *poll);

extern XSG_API void
xsg_main_remove_poll(xsg_main_poll_t *poll);

/******************************************************************************
 * list.c
 ******************************************************************************/

typedef struct _xsg_list_t xsg_list_t;

struct _xsg_list_t {
	void *data;
	xsg_list_t *next;
	xsg_list_t *prev;
};

extern XSG_API xsg_list_t *
xsg_list_append(xsg_list_t *list, void *data);

extern XSG_API xsg_list_t *
xsg_list_prepend(xsg_list_t *list, void *data);

extern XSG_API xsg_list_t *
xsg_list_insert(xsg_list_t *list, void *data, int position);

extern XSG_API xsg_list_t *
xsg_list_insert_sorted(
	xsg_list_t *list,
	void *data,
	int (*compare_func)(const void *a, const void *b)
);

extern XSG_API xsg_list_t *
xsg_list_remove(xsg_list_t *list, const void *data);

extern XSG_API xsg_list_t *
xsg_list_last(xsg_list_t *list);

extern XSG_API xsg_list_t *
xsg_list_find(xsg_list_t *list, const void *data);

extern XSG_API unsigned int
xsg_list_length(xsg_list_t *list);

extern XSG_API xsg_list_t *
xsg_list_nth(xsg_list_t *list, unsigned int n);

extern XSG_API void *
xsg_list_nth_data(xsg_list_t *list, unsigned int n);

extern XSG_API void
xsg_list_free(xsg_list_t *list);

extern XSG_API xsg_list_t *
xsg_list_delete_link(xsg_list_t *list, xsg_list_t *link);

/******************************************************************************
 * string.c
 ******************************************************************************/

typedef struct _xsg_string_t {
	char *str;
	size_t len;
	size_t allocated_len;
} xsg_string_t;

extern XSG_API xsg_string_t *
xsg_string_new(const char *init);

extern XSG_API xsg_string_t *
xsg_string_sized_new(size_t len);

extern XSG_API void
xsg_string_assign(xsg_string_t *string, const char *val);

extern XSG_API void
xsg_string_truncate(xsg_string_t *string, size_t len);

extern XSG_API void
xsg_string_set_size(xsg_string_t *string, size_t len);

extern XSG_API void
xsg_string_append(xsg_string_t *string, const char *val);

extern XSG_API void
xsg_string_append_c(xsg_string_t *string, char c);

extern XSG_API void
xsg_string_append_len(xsg_string_t *string, const char *val, ssize_t len);

extern XSG_API void
xsg_string_insert_len(
	xsg_string_t *string,
	ssize_t pos,
	const char *val,
	ssize_t len
);

extern XSG_API void
xsg_string_insert_c(xsg_string_t *string, ssize_t pos, char c);

extern XSG_API void
xsg_string_erase(xsg_string_t *string, ssize_t pos, ssize_t len);

extern XSG_API void
xsg_string_up(xsg_string_t *string);

extern XSG_API void
xsg_string_down(xsg_string_t *string);

extern XSG_API XSG_PRINTF(2, 3) void
xsg_string_printf(xsg_string_t *string, const char *format, ...);

extern XSG_API XSG_PRINTF(2, 3) void
xsg_string_append_printf(xsg_string_t *string, const char *format, ...);

extern XSG_API char *
xsg_string_free(xsg_string_t *string, bool free_segment);

/******************************************************************************
 * hash.c
 ******************************************************************************/

extern XSG_API bool
xsg_direct_equal(const void *a, const void *b);

extern XSG_API unsigned
xsg_direct_hash(const void *v);

extern XSG_API bool
xsg_int_equal(const void *a, const void *b);

extern XSG_API unsigned
xsg_int_hash(const void *v);

extern XSG_API bool
xsg_str_equal(const void *a, const void *b);

extern XSG_API unsigned
xsg_str_hash(const void *v);

typedef struct _xsg_hash_table_t xsg_hash_table_t;

extern XSG_API xsg_hash_table_t *
xsg_hash_table_new(
	unsigned (*hash_func)(const void *key),
	bool (*equal_func)(const void *a, const void *b)
);

extern XSG_API xsg_hash_table_t *
xsg_hash_table_new_full(
	unsigned (*hash_func)(const void *key),
	bool (*equal_func)(const void *a, const void *b),
	void (*key_destroy_notify_func)(void *data),
	void (*value_destroy_notify_func)(void *data)
);

extern XSG_API xsg_hash_table_t *
xsg_hash_table_ref(xsg_hash_table_t *hash_table);

extern XSG_API void
xsg_hash_table_unref(xsg_hash_table_t *hash_table);

extern XSG_API void
xsg_hash_table_insert(xsg_hash_table_t *hash_table, void *key, void *value);

extern XSG_API void *
xsg_hash_table_lookup(xsg_hash_table_t *hash_table, const void *key);

extern XSG_API bool
xsg_hash_table_lookup_extended(
	xsg_hash_table_t *hash_table,
	const void *key,
	void **orig_key,
	void **value
);

extern XSG_API bool
xsg_hash_table_remove(xsg_hash_table_t *hash_table, const void *key);

extern XSG_API void
xsg_hash_table_remove_all(xsg_hash_table_t *hash_table);

extern XSG_API void
xsg_hash_table_destroy(xsg_hash_table_t *hash_table);

extern XSG_API unsigned int
xsg_hash_table_size(xsg_hash_table_t *hash_table);

extern XSG_API void
xsg_hash_table_foreach(
	xsg_hash_table_t *hash_table,
	void (*func)(void *key, void *value, void *data),
	void *data
);

/******************************************************************************
 * buffer.c
 ******************************************************************************/

typedef struct _xsg_buffer_t xsg_buffer_t;

extern XSG_API xsg_buffer_t *
xsg_buffer_new(void);

extern XSG_API void
xsg_buffer_parse(
	xsg_buffer_t *buffer,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
);

extern XSG_API void
xsg_buffer_add(xsg_buffer_t *buffer, const char *string, size_t len);

extern XSG_API void
xsg_buffer_clear(xsg_buffer_t *buffer);

extern XSG_API void
xsg_buffer_help(xsg_string_t *string, const char *module_name, const char *opt);

/******************************************************************************
 * utils.c
 ******************************************************************************/

/* strfuncs */

extern XSG_API bool
xsg_str_has_suffix(const char *str, const char *suffix);

extern XSG_API char *
xsg_str_without_suffix(const char *str, const char *suffix);

extern XSG_API char **
xsg_strsplit_set(const char *string, const char *delimiters, int max_tokens);

extern XSG_API void
xsg_strfreev(char **str_array);

extern XSG_API int
xsg_strvcmp(char **strv1, char **strv2);

#define xsg_strdup(str) xsg_strdup_(str, __FILE__, __LINE__)
#define xsg_strndup(str, n) xsg_strndup_(str, n, __FILE__, __LINE__)

extern XSG_API char *
xsg_strdup_(const char *str, const char *file, int line);

extern XSG_API char *
xsg_strndup_(const char *str, size_t n, const char *file, int line);

/* byte order */

extern XSG_API uint16_t
xsg_uint16_be(uint16_t u);

extern XSG_API uint16_t
xsg_uint16_le(uint16_t u);

extern XSG_API uint32_t
xsg_uint32_be(uint32_t u);

extern XSG_API uint32_t
xsg_uint32_le(uint32_t u);

extern XSG_API uint64_t
xsg_uint64_be(uint64_t u);

extern XSG_API uint64_t
xsg_uint64_le(uint64_t u);

extern XSG_API double
xsg_double_be(double d);

extern XSG_API double
xsg_double_le(double d);

/* mem */

extern XSG_API void *
xsg_malloc_(size_t size, const char *file, int line);

extern XSG_API void *
xsg_malloc0_(size_t size, const char *file, int line);

extern XSG_API void *
xsg_realloc_(void *mem, size_t size, const char *file, int line);

extern XSG_API void
xsg_free_(void *mem, const char *file, int line);

#define xsg_malloc(size) xsg_malloc_(size, __FILE__, __LINE__)
#define xsg_malloc0(size) xsg_malloc0_(size, __FILE__, __LINE__)
#define xsg_realloc(mem, size) xsg_realloc_(mem, size, __FILE__, __LINE__)
#define xsg_free(mem) xsg_free_(mem, __FILE__, __LINE__)

#define xsg_new(struct_type, n_structs) \
	((struct_type *) xsg_malloc(((size_t) sizeof(struct_type)) \
			* ((size_t) (n_structs))))
#define xsg_new0(struct_type, n_structs) \
	((struct_type *) xsg_malloc0(((size_t) sizeof(struct_type)) \
			* ((size_t) (n_structs))))
#define xsg_renew(struct_type, mem, n_structs) \
	((struct_type *) xsg_realloc((mem), ((size_t) sizeof(struct_type)) \
			* ((size_t) (n_structs))))

extern XSG_API int
xsg_vasprintf(char **strp, const char *fmt, va_list ap);

extern XSG_API XSG_PRINTF(2, 3) int
xsg_asprintf(char **strp, const char *fmt, ...);

/* misc */

extern XSG_API char *
xsg_build_filename(const char *first_element, ...);

extern XSG_API const char *
xsg_get_home_dir(void);

extern XSG_API char *
xsg_dirname(const char *file_name);

typedef enum {
	XSG_FILE_TEST_IS_REGULAR    = 1 << 0,
	XSG_FILE_TEST_IS_SYMLINK    = 1 << 1,
	XSG_FILE_TEST_IS_DIR        = 1 << 2,
	XSG_FILE_TEST_IS_EXECUTABLE = 1 << 3,
	XSG_FILE_TEST_EXISTS        = 1 << 4
} xsg_file_test_t;

extern XSG_API bool
xsg_file_test(const char *filename, xsg_file_test_t test);

extern XSG_API char **
xsg_get_path_from_env(const char *env_name, const char *default_path);

extern XSG_API int
xsg_gettimeofday(struct timeval *tv, void *tz);

extern XSG_API char *
xsg_getenv(const char *name);

extern XSG_API int
xsg_setenv(const char *name, const char *value, int overwrite);

extern XSG_API int
xsg_unsetenv(const char *name);

extern XSG_API void
xsg_set_cloexec_flag(int fd, bool value);

#define xsg_timerisset(tvp) ((tvp)->tv_sec || (tvp)->tv_usec)

#define xsg_timerclear(tvp) ((tvp)->tv_sec = (tvp)->tv_usec = 0)

#define xsg_timercmp(a, b, CMP)						\
	(((a)->tv_sec == (b)->tv_sec) ?					\
	 ((a)->tv_usec CMP (b)->tv_usec) :				\
	 ((a)->tv_sec CMP (b)->tv_sec))

#define xsg_timeradd(a, b, result)					\
	do {								\
		(result)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
		(result)->tv_usec = (a)->tv_usec + (b)->tv_usec;	\
		if ((result)->tv_usec >= 1000000) {			\
			++(result)->tv_sec;				\
			(result)->tv_usec -= 1000000;			\
		}							\
	} while (0)

#define xsg_timersub(a, b, result)					\
	do {								\
		(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
		(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;	\
		if ((result)->tv_usec < 0) {				\
			--(result)->tv_sec;				\
			(result)->tv_usec += 1000000;			\
		}							\
	} while (0)

/******************************************************************************
 * logging
 ******************************************************************************/

#define XSG_LOG_LEVEL_ERROR	1
#define XSG_LOG_LEVEL_WARNING	2
#define XSG_LOG_LEVEL_MESSAGE	3
#define XSG_LOG_LEVEL_DEBUG	4
#define XSG_LOG_LEVEL_MEM	5

#ifndef XSG_LOG_DOMAIN
# define XSG_LOG_DOMAIN XSG_MODULE_NAME
#endif /* XSG_LOG_DOMAIN */

#ifndef XSG_LOG_LEVEL_MAX
# define XSG_LOG_LEVEL_MAX XSG_LOG_LEVEL_MESSAGE
#endif /* XSG_LOG_LEVEL_MAX */

extern XSG_API int xsg_log_level;

extern XSG_API XSG_PRINTF(3, 4) void
xsg_log(const char *domain, int level, const char *format, ...);

#if (XSG_LOG_LEVEL_ERROR <= XSG_LOG_LEVEL_MAX)
# define xsg_error(...) do { \
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_ERROR)) \
		xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_ERROR, __VA_ARGS__); \
	} while(0)
#else
# define xsg_error(...)
#endif

#if (XSG_LOG_LEVEL_WARNING <= XSG_LOG_LEVEL_MAX)
# define xsg_warning(...) do { \
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_WARNING)) \
		xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_WARNING, __VA_ARGS__); \
	} while (0)
#else
# define xsg_warning(...)
#endif

#if (XSG_LOG_LEVEL_MESSAGE <= XSG_LOG_LEVEL_MAX)
# define xsg_message(...) do { \
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_MESSAGE)) \
		xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_MESSAGE, __VA_ARGS__); \
	} while(0)
#else
# define xsg_message(...)
#endif

#if (XSG_LOG_LEVEL_DEBUG <= XSG_LOG_LEVEL_MAX)
# define xsg_debug(...) do { \
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_DEBUG)) \
		xsg_log(XSG_LOG_DOMAIN, XSG_LOG_LEVEL_DEBUG, __VA_ARGS__); \
	} while(0)
#else
# define xsg_debug(...)
#endif

/******************************************************************************
 * compat
 ******************************************************************************/

#ifdef SunOS
# include <ieeefp.h>
# define isinf(x) (!finite(x) && (x) == (x))
#endif /* SunOS */

/******************************************************************************/

#endif /* __XSYSGUARD_H__ */

