/* conf.c
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
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "conf.h"

/******************************************************************************/

static char *log_name;
static char *buf;
static char *ptr;
static unsigned int line;

/******************************************************************************/

static bool is(char c) {
	return (*ptr == c);
}

static void skip_space() {
	while (is(' ') || is('\t')) {
		ptr++;
	}
}

/******************************************************************************/

void xsg_conf_set_buffer(char *name, char *buffer) {
	if (name)
		log_name = name;
	else
		log_name = "";
	buf = buffer;
	ptr = buffer;
	line = 1;
}

void xsg_conf_error(const char *expected) {
	char *p;
	unsigned int n;

	p = ptr;

	while (!is('\0') && !is('\n'))
		ptr++;

	*ptr = '\0';

	while (!is('\n') && ptr > buf)
		ptr--;

	n = p - ptr;

	p = xsg_new0(char, n + 1);
	memset(p, ' ', n);
	p[n-1] = '^';
	p[n] = '\0';

	xsg_error("%s: Cannot parse configuration line %d: %s expected.\n%s\n%s\n",
			log_name, line, expected, ptr, p);
}

/******************************************************************************/

bool xsg_conf_find_command(const char *command) {
	char *nptr;

	skip_space();

	nptr = ptr;

	while (*command != '\0') {
		if (*command != *nptr)
			return FALSE;
		command++;
		nptr++;
	}

	if (*nptr != ',' && *nptr != ':' && *nptr != ' ' && *nptr != '\t' && *nptr != '\n')
		return FALSE;

	if (*nptr == '\n')
		nptr--;

	ptr = nptr + 1;
	return TRUE;
}

bool xsg_conf_find_commentline() {

	skip_space();

	if (is('#')) {
		ptr++;
		while (!is('\0') && !is('\n'))
			ptr++;
		if (is('\n')) {
			line++;
			ptr++;
		}
		return TRUE;
	} else {
		return FALSE;
	}
}

bool xsg_conf_find_newline() {

	skip_space();

	if (!is('\n'))
		return FALSE;

	ptr++;
	line++;
	return TRUE;
}

bool xsg_conf_find_end() {

	skip_space();

	return is('\0');
}

/******************************************************************************/

void xsg_conf_read_newline() {

	skip_space();

	if (!is('\n'))
		xsg_conf_error("newline");

	ptr++;
	line++;
}

bool xsg_conf_read_boolean() {

	if (xsg_conf_find_command("on"))
		return TRUE;
	else if (xsg_conf_find_command("On"))
		return TRUE;
	else if (xsg_conf_find_command("true"))
		return TRUE;
	else if (xsg_conf_find_command("True"))
		return TRUE;
	else if (xsg_conf_find_command("off"))
		return FALSE;
	else if (xsg_conf_find_command("Off"))
		return FALSE;
	else if (xsg_conf_find_command("false"))
		return FALSE;
	else if (xsg_conf_find_command("False"))
		return FALSE;
	else
		xsg_conf_error("on or off");

	return FALSE;
}

int64_t xsg_conf_read_int() {
	int64_t i;
	int n = 0;

	skip_space();

	sscanf(ptr, "%"SCNd64"%n", &i, &n);

	if (n < 1)
		xsg_conf_error("integer");

	ptr += n;
	return i;
}

uint64_t xsg_conf_read_uint() {
	uint64_t u;
	int n = 0;

	skip_space();

	sscanf(ptr, "%"SCNu64"%n", &u, &n);

	if (n < 1)
		xsg_conf_error("unsigned integer");

	ptr += n;
	return u;
}

uint32_t xsg_conf_read_color() {
	uint32_t color = 0;
	uint32_t argb = 0;
	uint8_t a = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	int n = 0;

	skip_space();

	if (!is('#'))
		xsg_conf_error("color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA)");

	ptr++;

	sscanf(ptr, "%"SCNx32"%n", &color, &n);

	switch (n) {
		case 3: // rgb
			r = (color >> 8) & 0xf;
			g = (color >> 4) & 0xf;
			b = color & 0xf;
			r |= r << 4;
			g |= g << 4;
			b |= b << 4;
			a = 0xff;
			break;
		case 4: // rgba
			r = (color >> 12) & 0xf;
			g = (color >> 8) & 0xf;
			b = (color >> 4) & 0xf;
			a = color & 0xf;
			r |= r << 4;
			g |= g << 4;
			b |= b << 4;
			a |= a << 4;
			break;
		case 6: // rrggbb
			r = (color >> 16) & 0xff;
			g = (color >> 8) & 0xff;
			b = color & 0xff;
			a = 0xff;
			break;
		case 8: // rrggbbaa
			r = (color >> 24) & 0xff;
			g = (color >> 16) & 0xff;
			b = (color >> 8) & 0xff;
			a = color & 0xff;
			break;
		default:
			xsg_conf_error("color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA)");
	}

	ptr += n;

#if __BYTE_ORDER == __LITTLE_ENDIAN
	((uint8_t *)(&argb))[0] = a;
	((uint8_t *)(&argb))[1] = r;
	((uint8_t *)(&argb))[2] = g;
	((uint8_t *)(&argb))[3] = b;
#elif __BYTE_ORDER == __BIG_ENDIAN
	((uint8_t *)(&argb))[3] = a;
	((uint8_t *)(&argb))[2] = r;
	((uint8_t *)(&argb))[1] = g;
	((uint8_t *)(&argb))[0] = b;
#endif

	return argb;
}

double xsg_conf_read_double() {
	double d;
	int n = 0;

	skip_space();

	sscanf(ptr, "%lf%n", &d, &n);

	if (n < 1)
		xsg_conf_error("floating-point number");

	ptr += n;
	return d;
}

static char read_escape() {
	char c;
	int  n;

	switch (*ptr) {
		case ' ':
			return ' ';
		case 'a':
			return '\a';
		case 'b':
			return '\b';
		case 'f':
			return '\f';
		case 'n':
			return '\n';
		case 'r':
			return '\r';
		case 't':
			return '\t';
		case 'v':
			return '\v';
		case '\\':
			return '\\';
		case '?':
			return '\?';
		case '\'':
			return '\'';
		case '\"':
			return '\"';
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			sscanf(ptr, "%3"SCNo8"%n", &c, &n);
			ptr += n - 1;
			return c;
		case 'x':
			ptr++;
			if (*ptr == '\0')
				return 0;
			sscanf(ptr, "%2"SCNx8"%n", &c, &n);
			ptr += n - 1;
			return c;
		case '\0':
			return '\0';
		default:
			return '\\';
	}
}

static char *read_env() {
	char *begin;
	const char *env;
	char *p;

	begin = ptr;

	while (!is(' ') && !is(':') && !is(',') && !is('\t') && !is('\n') && !is('\0'))
		ptr++;

	p = xsg_new0(char, ptr - begin + 1);
	strncpy(p, begin, ptr - begin);

	env = getenv(p);

	if (env) {
		xsg_free(p);
		return xsg_strdup(env);
	} else {
		return p;
	}
}

char *xsg_conf_read_string() {
	char quote = 0;
	bool escape;
	char *start_ptr;
	char *dest;
	char *string;
	size_t len;

	skip_space();

	if (is('\"') || is('\'') || is('`')) {
		quote = *ptr;
		ptr++;
	} else {
		return read_env();
	}

	start_ptr = ptr;
	escape = FALSE;

	while (*ptr) {
		if (escape) {
			read_escape();
			escape = FALSE;
			ptr++;
			continue;
		}
		if (is('\\')) {
			escape = TRUE;
			ptr++;
			continue;
		}
		if (is(quote)) {
			ptr++;
			break;
		}
		ptr++;
	}

	len = ptr - start_ptr;
	ptr = start_ptr;

	string = xsg_new0(char, len + 1);

	dest = string;

	escape = FALSE;
	while (*ptr) {
		if (escape) {
			*dest = read_escape();
			escape = FALSE;
			dest++;
			ptr++;
			continue;
		}
		if (is('\\')) {
			escape = TRUE;
			ptr++;
			continue;
		}
		if (is(quote)) {
			ptr++;
			break;
		}
		*dest = *ptr;
		dest++;
		ptr++;
	}
	return string;
}

