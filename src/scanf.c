/* scanf.c
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

#include <xsysguard.h>
#include <ctype.h>

#include "scanf.h"

/******************************************************************************/

static long long
strtoll_len(const char *nptr, char **endptr, int base, unsigned long maxlen)
{
	/* TODO: reimplement without string copy */
	if (maxlen == 0) {
		return strtoll(nptr, endptr, base);
	} else {
		static xsg_string_t *s = NULL;
		long long ret;
		unsigned len;
		char *end;

		if (unlikely(s == NULL)) {
			s = xsg_string_new(NULL);
		}

		xsg_string_truncate(s, 0);
		xsg_string_append_len(s, nptr, maxlen);

		ret = strtoll(s->str, &end, base);

		len = end - s->str;
		*endptr = (char *) nptr + len;

		return ret;
	}
}

static unsigned long long
strtoull_len(const char *nptr, char **endptr, int base, unsigned long maxlen)
{
	/* TODO: reimplement without string copy */
	if (maxlen == 0) {
		return strtoull(nptr, endptr, base);
	} else {
		static xsg_string_t *s = NULL;
		unsigned long long ret;
		unsigned len;
		char *end;

		if (unlikely(s == NULL)) {
			s = xsg_string_new(NULL);
		}

		xsg_string_truncate(s, 0);
		xsg_string_append_len(s, nptr, maxlen);

		ret = strtoull(s->str, &end, base);

		len = end - s->str;
		*endptr = (char *) nptr + len;

		return ret;
	}
}

static double
strtod_len(const char *nptr, char **endptr, unsigned long maxlen)
{
	/* TODO: reimplement without string copy */
	if (maxlen == 0) {
		return strtod(nptr, endptr);
	} else {
		static xsg_string_t *s = NULL;
		double ret;
		unsigned len;
		char *end;

		if (unlikely(s == NULL)) {
			s = xsg_string_new(NULL);
		}

		xsg_string_truncate(s, 0);
		xsg_string_append_len(s, nptr, maxlen);

		ret = strtod(s->str, &end);

		len = end - s->str;
		*endptr = (char *) nptr + len;

		return ret;
	}
}

/******************************************************************************/

static void
scan_s_len(const char *nptr, char **endptr, unsigned long maxlen)
{
	unsigned long len = 0;

	while (isspace(nptr)) {
		nptr++;
	}

	while ((maxlen == 0 || len < maxlen) && *nptr != '\0' && !isspace(*nptr)) {
		nptr++;
		len++;
	}

	if (endptr != NULL) {
		*endptr = (char *) nptr;
	}
}

static void
scan_c_len(const char *nptr, char **endptr, unsigned long maxlen)
{
	unsigned long len = 0;

	if (maxlen == 0) {
		maxlen = 1;
	}

	while (len < maxlen && *nptr != '\0') {
		nptr++;
		len++;
	}

	if (endptr != NULL) {
		*endptr = (char *) nptr;
	}
}

static void
scan_set_len(
	const char *nptr,
	char **endptr,
	const char *fmt,
	char **endfmt,
	unsigned long maxlen
)
{
	unsigned char invert;
	unsigned char scanset[256];
	unsigned long len = 0;
	int i;

	if (*fmt != '[') {
		if (endptr != NULL) {
			*endptr = (char *) nptr;
		}
		if (endfmt != NULL) {
			*endfmt = (char *) fmt;
		}
		return;
	}

	fmt++;

	invert = 0;

	if (*fmt == '^') {
		fmt++;
		invert = 1;
	}

	for (i = 0; i < sizeof(scanset); i++) {
		scanset[i] = invert;
	}

	invert = 1 - invert;

	if (*fmt == ']') {
		scanset[(int) (']')] = invert;
		fmt++;
	}

	while (*fmt != ']') {
		if (*fmt == '\0') {
			if (endptr != NULL) {
				*endptr = (char *) nptr;
			}
			if (endfmt != NULL) {
				*endfmt = (char *) fmt;
			}
			return;
		}
		if ((*fmt == '-') && (fmt[1] != ']') && (fmt[-1] < fmt[1])) {
			fmt++;
			i = fmt[-2];
			do {
				scanset[++i] = invert;
			} while (i < *fmt);
		}
		scanset[(int) *fmt] = invert;
		fmt++;
	}

	while ((maxlen == 0 || len < maxlen) && *nptr != '\0') {
		if (!scanset[(int) *nptr]) {
			if (endptr != NULL) {
				*endptr = (char *) nptr;
			}
			if (endfmt != NULL) {
				*endfmt = (char *) fmt;
			}
			return;
		}
		nptr++;
		len++;
	}

	if (endptr != NULL) {
		*endptr = (char *) nptr;
	}
	if (endfmt != NULL) {
		*endfmt = (char *) fmt;
	}
}

/******************************************************************************/

static bool
scan(const char **buffer, const char **format)
{
	const char *b = *buffer;
	const char *f = *format;

	while (*b && *f) {
		if (isspace(*f)) {
			f++;
			while (isspace(*b)) {
				b++;
			}
			continue;
		} else if (*f == '%') {
			unsigned long len;
			char *n, *m;

			f++;

			if (*f == '%') {
				if (*b == '%') {
					f++;
					b++;
					continue;
				} else {
					return FALSE;
				}
			}

			if (*f != '*') {
				*buffer = b;
				*format = f;
				return TRUE;
			}

			f++;

			len = strtoul(f, &n, 10);
			f = n;

			switch (*f) {
			case 'd':
				strtoll_len(b, &n, 10, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'i':
				strtoll_len(b, &n, 0, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'o':
				strtoull_len(b, &n, 8, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'u':
				strtoull_len(b, &n, 10, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'x':
			case 'X':
			case 'p':
				strtoull_len(b, &n, 16, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'f':
			case 'e':
			case 'g':
			case 'E':
			case 'a':
				strtod_len(b, &n, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 's':
				scan_s_len(b, &n, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case 'c':
				scan_c_len(b, &n, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				break;
			case '[':
				scan_set_len(b, &n, f, &m, len);
				if (n == b) {
					return FALSE;
				}
				b = n;
				f = m;
				break;
			default:
				return FALSE;
			}
			f++;
		} else {
			if (*f != *b) {
				return FALSE;
			}
			f++;
			b++;
		}
	}

	return FALSE;
}

/******************************************************************************/

char *
xsg_scanf_string(const char *buffer, const char *format)
{
	static xsg_string_t *string = NULL;
	unsigned long len;
	char *n;

	if (!scan(&buffer, &format)) {
		return NULL;
	}

	len = strtoul(format, &n, 10);
	format = n;

	switch (*format) {
	case 'd':
		strtoll_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'i':
		strtoll_len(buffer, &n, 0, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'o':
		strtoull_len(buffer, &n, 8, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'u':
		strtoull_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'x':
	case 'X':
	case 'p':
		strtoull_len(buffer, &n, 16, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'f':
	case 'e':
	case 'g':
	case 'E':
	case 'a':
		strtod_len(buffer, &n, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 's':
		scan_s_len(buffer, &n, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case 'c':
		scan_c_len(buffer, &n, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	case '[':
		scan_set_len(buffer, &n, format, NULL, len);
		if (n == buffer) {
			return NULL;
		}
		len = n - buffer;
		break;
	default:
		return NULL;
	}

	if (unlikely(string == NULL)) {
		string = xsg_string_new(NULL);
	}

	xsg_string_truncate(string, 0);
	xsg_string_append_len(string, buffer, len);

	return string->str;
}

double *
xsg_scanf_number(const char *buffer, const char *format)
{
	static double number = DNAN;
	unsigned long len;
	char *n;

	if (!scan(&buffer, &format)) {
		return NULL;
	}

	len = strtoul(format, &n, 10);
	format = n;

	switch (*format) {
	case 'd':
		number = (double) strtoll_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'i':
		number = (double) strtoll_len(buffer, &n, 0, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'o':
		number = (double) strtoull_len(buffer, &n, 8, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'u':
		number = (double) strtoull_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'x':
	case 'X':
	case 'p':
		number = (double) strtoull_len(buffer, &n, 16, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'f':
	case 'e':
	case 'g':
	case 'E':
	case 'a':
		number = (double) strtod_len(buffer, &n, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 's':
	case 'c':
	case '[':
	default:
		return NULL;
	}

	format++;

	return &number;
}

uint64_t *
xsg_scanf_counter(const char *buffer, const char *format)
{
	static uint64_t counter = 0;
	unsigned long len;
	char *n;

	if (!scan(&buffer, &format)) {
		return NULL;
	}

	len = strtoul(format, &n, 10);
	format = n;

	switch (*format) {
	case 'd':
		counter = (uint64_t) strtoll_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'i':
		counter = (uint64_t) strtoll_len(buffer, &n, 0, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'o':
		counter = (uint64_t) strtoull_len(buffer, &n, 8, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'u':
		counter = (uint64_t) strtoull_len(buffer, &n, 10, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'x':
	case 'X':
		counter = (uint64_t) strtoull_len(buffer, &n, 16, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 'f':
	case 'e':
	case 'g':
	case 'E':
	case 'a':
		counter = (uint64_t) strtod_len(buffer, &n, len);
		if (n == buffer) {
			return NULL;
		}
		break;
	case 's':
	case 'c':
	case '[':
	default:
		return NULL;
	}

	format++;

	return &counter;
}

