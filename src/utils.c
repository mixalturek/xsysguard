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

/******************************************************************************
 *
 * mem
 *
 ******************************************************************************/

void *xsg_malloc(size_t size) {
	if (likely(size)) {
		void *mem;

		xsg_debug("malloc %lu bytes", size);

		mem = malloc(size);
		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_malloc0(size_t size) {
	if (likely(size)) {
		void *mem;

		xsg_debug("malloc0 %lu bytes", size);

		mem = calloc(1, size);
		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	return NULL;
}

void *xsg_realloc(void *mem, size_t size) {
	if (likely(size)) {

		xsg_debug("realloc %lu bytes", size);

		if (unlikely(mem == NULL))
			mem = malloc(size);
		else
			mem = realloc(mem, size);

		if (likely(mem != NULL))
			return mem;

		xsg_error("Failed to allocate %lu bytes", size);
	}
	if (mem)
		free(mem);

	return NULL;
}

void xsg_free(void *mem) {
	free(mem);
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

