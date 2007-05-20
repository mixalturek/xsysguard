/* utils.c
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

#include <xsysguard.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

/******************************************************************************
 *
 * mem
 *
 ******************************************************************************/

void *xsg_malloc(size_t size) {
	if (likely(size)) {
		void *mem;

		mem = malloc(size);

		if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_MEM))
			xsg_log(NULL, XSG_LOG_LEVEL_MEM, "malloc:  %4lu bytes / %9p", size, mem);

		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_malloc0(size_t size) {
	if (likely(size)) {
		void *mem;

		mem = calloc(1, size);

		if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_MEM))
			xsg_log(NULL, XSG_LOG_LEVEL_MEM, "malloc0: %4lu bytes / %9p", size, mem);

		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_realloc(void *mem, size_t size) {
	if (likely(size)) {
		void *old = mem;

		if (unlikely(mem == NULL))
			mem = malloc(size);
		else
			mem = realloc(mem, size);

		if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_MEM))
			xsg_log(NULL, XSG_LOG_LEVEL_MEM, "realloc: %4lu bytes / %9p -> %9p", size, old, mem);

		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	if (mem)
		free(mem);

	return NULL;
}

void xsg_free(void *mem) {
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_MEM))
		xsg_log(NULL, XSG_LOG_LEVEL_MEM, "free: %9p", mem);
	free(mem);
}

/******************************************************************************/

int xsg_vasprintf(char **strp, const char *fmt, va_list ap) {
	va_list ap2;
	int len;
	char c;

	if (unlikely(strp == NULL))
		return -1;

	va_copy(ap2, ap);

	len = vsnprintf(&c, 1, fmt, ap) + 1;

	*strp = xsg_new(char, len);

	len = vsprintf(*strp, fmt, ap2);

	va_end(ap2);

	return len;
}

int xsg_asprintf(char **strp, const char *fmt, ...) {
	va_list args;
	int len;

	va_start(args, fmt);
	len = xsg_vasprintf(strp, fmt, args);
	va_end(args);

	return len;
}

/******************************************************************************
 *
 * strfuncs
 *
 ******************************************************************************/

bool xsg_str_has_suffix(const char *str, const char *suffix) {
	int str_len;
	int suffix_len;

	if (unlikely(str == NULL))
		return FALSE;
	if (unlikely(suffix == NULL))
		return FALSE;

	str_len = strlen(str);
	suffix_len = strlen(suffix);

	if (str_len < suffix_len)
		return FALSE;

	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

char *xsg_str_without_suffix(const char *str, const char *suffix) {
	int str_len;
	int suffix_len;

	if (unlikely(str == NULL))
		return NULL;
	if (unlikely(suffix == NULL))
		return xsg_strdup(str);

	str_len = strlen(str);
	suffix_len = strlen(suffix);

	if (str_len < suffix_len)
		return xsg_strdup(str);

	if (strcmp(str + str_len - suffix_len, suffix) != 0)
		return xsg_strdup(str);

	return xsg_strndup(str, str_len - suffix_len);
}

char **xsg_strsplit_set(const char *string, const char *delimiters, int max_tokens) {
	bool delim_table[256];
	xsg_list_t *tokens, *list;
	int n_tokens;
	const char *s;
	const char *current;
	char *token;
	char **result;

	if (unlikely(string == NULL))
		return NULL;
	if (unlikely(delimiters == NULL))
		return NULL;

	if (max_tokens < 1)
		max_tokens = INT_MAX;

	if (*string == '\0') {
		result = xsg_new(char *, 1);
		result[0] = NULL;
		return result;
	}

	memset(delim_table, FALSE, sizeof(delim_table));
	for (s = delimiters; *s != '\0'; ++s)
		delim_table[*(unsigned char *)s] = TRUE;

	tokens = NULL;
	n_tokens = 0;

	s = current = string;
	while (*s != '\0') {
		if (delim_table[*(unsigned char *)s] && n_tokens + 1 < max_tokens) {
			char *token;

			token = xsg_strndup(current, s - current);
			tokens = xsg_list_prepend(tokens, token);
			++n_tokens;

			current = s + 1;
		}

		++s;
	}

	token = xsg_strndup(current, s - current);
	tokens = xsg_list_prepend(tokens, token);
	++n_tokens;

	result = xsg_new(char *, n_tokens + 1);

	result[n_tokens] = NULL;
	for (list = tokens; list != NULL; list = list->next)
		result[--n_tokens] = list->data;

	xsg_list_free(tokens);

	return result;
}

void xsg_strfreev(char **str_array) {
	if (str_array) {
		int i;

		for (i = 0; str_array[i] != NULL; i++)
			xsg_free(str_array[i]);

		xsg_free(str_array);
	}
}

char *xsg_strdup(const char *str) {
	char *new_str;
	size_t length;

	if (str) {
		length = strlen(str) + 1;
		new_str = xsg_new(char, length);
		memcpy(new_str, str, length);
	} else {
		new_str = NULL;
	}

	return new_str;
}

char *xsg_strndup(const char *str, size_t n) {
	char *new_str;

	if (str) {
		new_str = xsg_new(char, n+ 1);
		strncpy(new_str, str, n);
		new_str[n] = '\0';
	} else {
		new_str = NULL;
	}

	return new_str;
}

/******************************************************************************
 *
 * byte order
 *
 ******************************************************************************/

static int am_big_endian(void) {
	long one = 1;
	return !(*((char *)(&one)));
}

static uint16_t swap_16(uint16_t u) {
	return (u >> 8) | (u << 8);
}

static uint32_t swap_32(uint32_t u) {
	return  ((u >> 24)) |
		((u <<  8) & 0x00ff0000) |
		((u >>  8) & 0x0000ff00) |
		((u << 24));
}

static uint64_t swap_64(uint64_t u) {
	return  ((u >> 56)) |
		((u << 40) & 0x00ff000000000000ULL) |
		((u << 24) & 0x0000ff0000000000ULL) |
		((u <<  8) & 0x000000ff00000000ULL) |
		((u >>  8) & 0x00000000ff000000ULL) |
		((u >> 24) & 0x0000000000ff0000ULL) |
		((u >> 40) & 0x000000000000ff00ULL) |
		((u << 56));
}

uint16_t xsg_uint16_be(uint16_t u) {
	if (am_big_endian())
		return u;
	else
		return swap_16(u);
}

uint16_t xsg_uint16_le(uint16_t u) {
	if (am_big_endian())
		return swap_16(u);
	else
		return u;
}

uint32_t xsg_uint32_be(uint32_t u) {
	if (am_big_endian())
		return u;
	else
		return swap_32(u);
}

uint32_t xsg_uint32_le(uint32_t u) {
	if (am_big_endian())
		return swap_32(u);
	else
		return u;
}

uint64_t xsg_uint64_be(uint64_t u) {
	if (am_big_endian())
		return u;
	else
		return swap_64(u);
}

uint64_t xsg_uint64_le(uint64_t u) {
	if (am_big_endian())
		return swap_64(u);
	else
		return u;
}

double xsg_double_be(double d) {
	if (am_big_endian()) {
		return d;
	} else {
		union {
			double d;
			uint64_t u;
		} u;
		u.d = d;
		u.u = swap_64(u.u);
		return u.d;
	}
}

double xsg_double_le(double d) {
	if (am_big_endian()) {
		union {
			double d;
			uint64_t u;
		} u;
		u.d = d;
		u.u = swap_64(u.u);
		return u.d;
	} else {
		return d;
	}
}

/******************************************************************************
 *
 * misc
 *
 ******************************************************************************/

static char *xsg_build_path_va(const char *separator, const char *first_element, va_list *args, char **str_array) {
	xsg_string_t *result;
	int separator_len = strlen(separator);
	bool is_first = TRUE;
	bool have_leading = FALSE;
	const char *single_element = NULL;
	const char *next_element;
	const char *last_trailing = NULL;
	int i = 0;

	result = xsg_string_new(NULL);

	if (str_array)
		next_element = str_array[i++];
	else
		next_element = first_element;

	while (TRUE) {
		const char *element;
		const char *start;
		const char *end;

		if (next_element) {
			element = next_element;
			if (str_array)
				next_element = str_array[i++];
			else
				next_element = va_arg(*args, char *);
		} else {
			break;
		}

		/* Ignore empty elements */
		if (!*element)
			continue;

		start = element;

		if (separator_len) {
			while (start && strncmp(start, separator, separator_len) == 0)
				start += separator_len;
		}

		end = start + strlen (start);

		if (separator_len) {
			while (end >= start + separator_len && strncmp(end - separator_len, separator, separator_len) == 0)
				end -= separator_len;

			last_trailing = end;
			while (last_trailing >= element + separator_len && strncmp(last_trailing - separator_len, separator, separator_len) == 0)
				last_trailing -= separator_len;

			if (!have_leading) {
				/* If the leading and trailing separator strings are in the
				 * same element and overlap, the result is exactly that element
				 */
				if (last_trailing <= start)
					single_element = element;

				xsg_string_append_len(result, element, start - element);
				have_leading = TRUE;
			} else {
				single_element = NULL;
			}
		}

		if (end == start)
			continue;

		if (!is_first)
			xsg_string_append(result, separator);

		xsg_string_append_len(result, start, end - start);
		is_first = FALSE;
	}

	if (single_element) {
		xsg_string_free(result, TRUE);
		return xsg_strdup(single_element);
	} else {
		if (last_trailing)
			xsg_string_append(result, last_trailing);

		return xsg_string_free(result, FALSE);
	}
}

char *xsg_build_filename(const char *first_element, ...) {
	char *str;
	va_list args;

	va_start(args, first_element);

	str = xsg_build_path_va("/", first_element, &args, NULL);

	va_end(args);

	return str;
}

static const char *home_dir = NULL;

const char *xsg_get_home_dir(void) {
	if (!home_dir)
		home_dir = xsg_strdup(getenv("HOME"));
	return home_dir;
}

bool xsg_file_test(const char *filename, xsg_file_test_t test) {
	if ((test & XSG_FILE_TEST_EXISTS) && (access(filename, F_OK) == 0))
		return TRUE;

	if ((test & XSG_FILE_TEST_IS_EXECUTABLE) && (access(filename, X_OK) == 0)) {
		if (getuid() != 0)
			return TRUE;
	} else {
		test &= ~XSG_FILE_TEST_IS_EXECUTABLE;
	}

	if (test & XSG_FILE_TEST_IS_SYMLINK) {
		struct stat s;

		if ((lstat(filename, &s) == 0) && S_ISLNK(s.st_mode))
			return TRUE;
	}

	if (test & (XSG_FILE_TEST_IS_REGULAR | XSG_FILE_TEST_IS_DIR | XSG_FILE_TEST_IS_EXECUTABLE)) {
		struct stat s;

		if (stat(filename, &s) == 0) {
			if ((test & XSG_FILE_TEST_IS_REGULAR) && S_ISREG(s.st_mode))
				return TRUE;

			if ((test & XSG_FILE_TEST_IS_DIR) && S_ISDIR(s.st_mode))
				return TRUE;

			if ((test & XSG_FILE_TEST_IS_EXECUTABLE) && ((s.st_mode & S_IXOTH) || (s.st_mode & S_IXUSR) || (s.st_mode & S_IXGRP)))
				return TRUE;
		}
	}

	return FALSE;
}

char **xsg_get_path_from_env(const char *env_name, const char *default_path) {
	const char *path;
	char **pathv;
	char **p;

	path = getenv(env_name);

	if (path == NULL)
		path = default_path;

	if (path == NULL)
		return NULL;

	pathv = xsg_strsplit_set(path, ":", 0);

	for (p = pathv; *p; p++) {
		if (*p[0] == '~') {
			char *q;

			q = xsg_build_filename(xsg_get_home_dir(), (*p) + 1, NULL);
			xsg_free(*p);
			*p = q;
		}
	}

	return pathv;
}

int xsg_timeval_sub(struct timeval *result, struct timeval *x, struct timeval *y) {
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	return x->tv_sec < y->tv_sec;
}

int xsg_gettimeofday(struct timeval *tv, void *tz) {
	int ret;

	ret = gettimeofday(tv, tz);

	if (unlikely(ret))
		xsg_error("gettimeofday(%p, %p) failed", tv, tz);

	return ret;
}


