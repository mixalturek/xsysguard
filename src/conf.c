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
#include <ctype.h>
#include <string.h>
#include <endian.h>

#include "conf.h"
#include "argb.h"

/******************************************************************************/

static char *log_name;
static char *buf;
static char *ptr;
static unsigned int line;

/******************************************************************************/

void xsg_conf_set_buffer(char *name, char *buffer) {
	log_name = name;
	buf = buffer;
	ptr = buffer;
	line = 1;
}

/******************************************************************************/

void xsg_conf_error(const char *message) {
	unsigned n;
	char *error_ptr;
	char *line2;

	error_ptr = ptr;

	while (ptr[0] != '\0' && ptr[0] != '\n')
		ptr++;

	ptr[0] = '\0';

	while (ptr[0] != '\n' && ptr > buf)
		ptr--;

	n = error_ptr - ptr;

	line2 = xsg_new(char, n + 1);
	memset(line2, ' ', n);
	line2[n-1] = '^';
	line2[n] = '\0';

	if (log_name)
		xsg_error("%s: Cannot parse configuration line %d: %s.\n%s\n%s\n",
				log_name, line, message, ptr, line2);
	else
		xsg_error("Cannot parse configuration line %d: %s.\n%s\n%s\n",
				line, message, ptr, line2);
}

/******************************************************************************/

static bool is_space(void) {
	return (ptr[0] == ' ' || ptr[0] == '\t' || ptr[0] == ':');
}

static void skip_space() {
	while (is_space())
		ptr++;
	if (ptr[0] == '\\') {
		char *p = ptr;
		while (is_space())
			p++;
		if (p[0] == '\n') {
			ptr = p + 1;
			line++;
		}
	}
}

/******************************************************************************/

static bool is_env(void) {
	return (ptr[0] == '$');
}

static char *env(int *n) {
	char *p, *env;
	int m;

	while (ptr[0] == ' ' || ptr[0] == '\t')
		ptr++;

	if (ptr[0] == '$')
		ptr++;

	if (ptr[0] != '{')
		xsg_conf_error("\"{\" expected");
	ptr++;

	if (ptr[0] == '}')
		xsg_conf_error("name expected");

	for (m = 0; ptr[m] != '}'; m++)
		if (ptr[m] == '\0')
			xsg_conf_error("\"}\" expected");

	p = xsg_strndup(ptr, m);
	env = getenv(p);
	xsg_free(p);

	*n = m + 1;

	if (env == NULL)
		xsg_conf_error("cannot find name in environment");

	return env;
}

/******************************************************************************/

bool xsg_conf_find_command(const char *command) {
	char *nptr;

	skip_space();

	nptr = ptr;

	while (command[0] != '\0') {
		if (command[0] != nptr[0])
			return FALSE;
		command++;
		nptr++;
	}

	if (isalpha(nptr[0]) || nptr[0] == '_') {
		return FALSE;
	} else {
		ptr = nptr;
		return TRUE;
	}
}

bool xsg_conf_find_comma(void) {
	skip_space();

	if (ptr[0] == ',') {
		ptr++;
		return TRUE;
	} else {
		return FALSE;
	}
}

bool xsg_conf_find_commentline(void) {
	skip_space();

	if (ptr[0] == '#') {
		ptr++;
		while (ptr[0] != '\0' && ptr[0] != '\n')
			ptr++;
		if (ptr[0] == '\n')
			ptr++;
		return TRUE;
	} else {
		return FALSE;
	}
}

bool xsg_conf_find_newline(void) {
	skip_space();

	if (ptr[0] != '\n') {
		return FALSE;
	} else {
		ptr++;
		line++;
		return TRUE;
	}
}

bool xsg_conf_find_end(void) {
	skip_space();

	return ptr[0] == '\0';
}

/******************************************************************************/

void xsg_conf_read_newline(void) {
	skip_space();

	if (ptr[0] != '\n')
		xsg_conf_error("newline expected");

	ptr++;
	line++;
}

bool xsg_conf_read_boolean(void) {
	skip_space();

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
		xsg_conf_error("on or off expected");

	return FALSE;
}

int64_t xsg_conf_read_int() {
	int64_t i;
	int n = 0;
	int m = 0;

	skip_space();

	if (is_env()) {
		sscanf(env(&n), "%"SCNd64"%n", &i, &m);
		if (m < 1) xsg_conf_error("cannot convert environment variable to integer");
	} else {
		sscanf(ptr, "%"SCNd64"%n", &i, &n);
		if (n < 1) xsg_conf_error("integer expected");
	}
	ptr += n;
	return i;
}

uint64_t xsg_conf_read_uint() {
	uint64_t u;
	int n = 0;
	int m = 0;

	skip_space();

	if (is_env()) {
		sscanf(env(&n), "%"SCNu64"%n", &u, &m);
		if (m < 1) xsg_conf_error("cannot convert environment variable to unsigned integer");
	} else {
		sscanf(ptr, "%"SCNu64"%n", &u, &n);
		if (n < 1) xsg_conf_error("unsigned integer expected");
	}
	ptr += n;
	return u;
}

double xsg_conf_read_double() {
	double d;
	int n = 0;
	int m = 0;

	skip_space();

	if (is_env()) {
		sscanf(env(&n), "%lf%n", &d, &m);
		if (m < 1) xsg_conf_error("cannot convert environment variable to floating-point number");
	} else {
		sscanf(ptr, "%lf%n", &d, &n);
		if (n < 1) xsg_conf_error("floating-point number expected");
	}
	ptr += n;
	return d;
}

/******************************************************************************/

static bool (*color_lookup)(char *name, uint32_t *color) = NULL;

void xsg_conf_set_color_lookup(bool (*func)(char *name, uint32_t *color)) {
	color_lookup = func;
}

uint32_t xsg_conf_read_color() {
	char *name = NULL;
	uint32_t color = 0;
	uint32_t argb = 0;
	uint8_t a = 0;
	uint8_t r = 0;
	uint8_t g = 0;
	uint8_t b = 0;
	int n = 0;
	int m = 0;
	char *e = NULL;

	skip_space();

	if (is_env()) {
		e = env(&m);
		if (e[0] == '#') {
			e++;
			sscanf(e, "%"SCNx32"%n", &color, &n);
		} else {
			if (color_lookup == NULL)
				xsg_conf_error("color (#RGB, #RGBA, #RRGGBB or #RRGGBBAA) expected");
			name = xsg_strdup(e);
		}
	} else {
		if (ptr[0] == '#') {
			ptr++;
			sscanf(ptr, "%"SCNx32"%n", &color, &n);
		} else {
			if (color_lookup == NULL)
				xsg_conf_error("color (#RGB, #RGBA, #RRGGBB or #RRGGBBAA) expected");
			while (isalpha(ptr[n]))
				n++;
			name = xsg_strndup(ptr, n);
		}
	}

	if (name) {
		if (color_lookup(name, &color)) {
			if (e)
				ptr += m;
			else
				ptr += n;
			return color;
		}
		xsg_conf_error("color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA or rgb.txt name) expected");
	}

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
			if (color_lookup)
				xsg_conf_error("color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA or rgb.txt name) expected");
			else
				xsg_conf_error("color (#RGB, #RGBA, #RRGGBB, #RRGGBBAA) expected");
	}

	if (e)
		ptr += m;
	else
		ptr += n;

	A_VAL(&argb) = a;
	R_VAL(&argb) = r;
	G_VAL(&argb) = g;
	B_VAL(&argb) = b;

	return argb;
}

/******************************************************************************/

static char read_escape() {
	char c;
	int  n;

	switch (ptr[0]) {
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

char *xsg_conf_read_string() {
	xsg_string_t *string;
	char quote = '\0';
	bool escape = FALSE;
	char *str;

	skip_space();

	string = xsg_string_new(NULL);

	if (ptr[0] == '\"' || ptr[0] == '\'') {
		quote = ptr[0];
		ptr++;
	}

	while (ptr[0] != '\0') {
		if (escape) {
			string = xsg_string_append_c(string, read_escape());
			escape = FALSE;
			ptr++;
			continue;
		}
		if (ptr[0] == '\\') {
			escape = TRUE;
			ptr++;
			continue;
		}
		if (ptr[0] == quote) {
			ptr++;
			break;
		}
		if (quote == '\0') {
			if (is_space() || ptr[0] == '\n')
				break;
		}
		if ((quote == '\"' || quote == '\0') && is_env()) {
			int n;

			string = xsg_string_append(string, env(&n));
			ptr += n;
		} else {
			string = xsg_string_append_c(string, ptr[0]);
			ptr++;
		}
	}

	str = xsg_strndup(string->str, string->len);

	xsg_string_free(string, TRUE);

	return str;
}

