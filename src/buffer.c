/* buffer.c
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

/*
 * read:all
 * read:sscanf:<format>
 * read:nscanf:<format>
 * read:cscanf:<format>
 * read:grep:<pattern>:<index>
 * read:igrep:<pattern>:<index>
 *
 * readline:<number>:all
 * readline:<number>:sscanf:<format>
 * readline:<number>:nscanf:<format>
 * readline:<number>:cscanf:<format>
 * readline:<number>:grep:<pattern>:<index>
 * readline:<number>:igrep:<pattern>:<index>
 */

/******************************************************************************/

#include <xsysguard.h>
#include <regex.h>

#include "scanf.h"

/******************************************************************************/

struct _xsg_buffer_t {
	xsg_string_t *read_string;
	xsg_string_t *readline_string;
	xsg_list_t *read_var_list; /* read_var_t */
	xsg_list_t *readline_var_list; /* readline_var_t */
	uint64_t line_number;
};

/******************************************************************************/

typedef struct _read_var_t {
	void (*func)(void *arg, xsg_string_t *string);
	void *arg;
} read_var_t;

typedef struct _readline_var_t {
	void (*func)(void *arg, xsg_string_t *string);
	void *arg;
	uint64_t number;
} readline_var_t;

/******************************************************************************/

xsg_buffer_t *
xsg_buffer_new(void)
{
	xsg_buffer_t *buffer;

	buffer = xsg_new(xsg_buffer_t, 1);

	buffer->read_string = NULL;
	buffer->readline_string = NULL;
	buffer->read_var_list = NULL;
	buffer->readline_var_list = NULL;
	buffer->line_number = 1;

	return buffer;
}

/******************************************************************************/

typedef struct _all_t {
	xsg_var_t *var;
	xsg_string_t *string;
} all_t;

static const char *
get_all(void *arg)
{
	all_t *all = (all_t *) arg;

	return all->string->str;
}

static void
process_all(void *arg, xsg_string_t *string)
{
	all_t *all = (all_t *) arg;

	xsg_string_assign(all->string, string->str);

	if (all->var) {
		xsg_var_dirty(all->var);
	}
}

static void *
parse_all(xsg_buffer_t *buffer, xsg_var_t *var)
{
	all_t *all;

	all = xsg_new(all_t, 1);
	all->var = var;
	all->string = xsg_string_new(NULL);

	return (void *) all;
}

/******************************************************************************/

typedef struct _sscanf_t {
	xsg_var_t *var;
	xsg_string_t *string;
	char *format;
} sscanf_t;

static const char *
get_sscanf(void *arg)
{
	sscanf_t *sscanf = (sscanf_t *) arg;

	return sscanf->string->str;
}

static void
process_sscanf(void *arg, xsg_string_t *string)
{
	sscanf_t *sscanf = (sscanf_t *) arg;
	char *s;

	s = xsg_scanf_string(string->str, sscanf->format);

	if (s == NULL) {
		return;
	}

	xsg_string_assign(sscanf->string, s);

	if (sscanf->var) {
		xsg_var_dirty(sscanf->var);
	}
}

static void *
parse_sscanf(xsg_buffer_t *buffer, xsg_var_t *var)
{
	sscanf_t *sscanf;

	sscanf = xsg_new(sscanf_t, 1);
	sscanf->var = var;
	sscanf->string = xsg_string_new(NULL);
	sscanf->format = xsg_conf_read_string();

	return (void *) sscanf;
}

/******************************************************************************/

typedef struct _nscanf_t {
	xsg_var_t *var;
	double number;
	char *format;
} nscanf_t;

static double
get_nscanf(void *arg)
{
	nscanf_t *nscanf = (nscanf_t *) arg;

	return nscanf->number;
}

static void
process_nscanf(void *arg, xsg_string_t *string)
{
	nscanf_t *nscanf = (nscanf_t *) arg;
	double *n;

	n = xsg_scanf_number(string->str, nscanf->format);

	if (n == NULL) {
		return;
	}

	nscanf->number = *n;

	if (nscanf->var) {
		xsg_var_dirty(nscanf->var);
	}
}

static void *
parse_nscanf(xsg_buffer_t *buffer, xsg_var_t *var)
{
	nscanf_t *nscanf;

	nscanf = xsg_new(nscanf_t, 1);
	nscanf->var = var;
	nscanf->number = DNAN;
	nscanf->format = xsg_conf_read_string();

	return (void *) nscanf;
}

/******************************************************************************/

typedef struct _cscanf_t {
	xsg_var_t *var;
	double number;
	uint64_t prev;
	char *format;
} cscanf_t;

static double
get_cscanf(void *arg)
{
	cscanf_t *cscanf = (cscanf_t *) arg;

	return cscanf->number;
}

static void
process_cscanf(void *arg, xsg_string_t *string)
{
	cscanf_t *cscanf = (cscanf_t *) arg;
	uint64_t *c;
	uint64_t diff;

	c = xsg_scanf_counter(string->str, cscanf->format);

	if (c == NULL) {
		return;
	}

	diff = *c - cscanf->prev;

	if (cscanf->prev == (uint64_t)-1) {
		cscanf->number = DNAN;
	} else {
		cscanf->number = (double) diff;
	}

	cscanf->prev = *c;

	if (cscanf->var) {
		xsg_var_dirty(cscanf->var);
	}
}

static void *
parse_cscanf(xsg_buffer_t *buffer, xsg_var_t *var)
{
	cscanf_t *cscanf;

	cscanf = xsg_new(cscanf_t, 1);
	cscanf->var = var;
	cscanf->number = DNAN;
	cscanf->prev = (uint64_t)-1;
	cscanf->format = xsg_conf_read_string();

	return (void *) cscanf;
}

/******************************************************************************/

typedef struct _re_t {
	xsg_var_t *var;
	xsg_string_t *string;
	char *pattern;
	int index;
	regex_t *regex;
} re_t;

static const char *
get_re(void *arg)
{
	re_t *re = (re_t *) arg;

	return re->string->str;
}

static void
process_re(void *arg, xsg_string_t *string)
{
	re_t *re = (re_t *) arg;

	if (re->index < 0) {
		if (regexec(re->regex, string->str, 0, NULL, 0) != 0) {
			return;
		}

		xsg_string_assign(re->string, string->str);
	} else {
		regmatch_t *regmatch, *match;

		regmatch = alloca(sizeof(regmatch_t) * (re->index + 1));

		if (regexec(re->regex, string->str, re->index + 1, regmatch, 0) != 0) {
			return;
		}

		match = &regmatch[re->index];
		xsg_string_truncate(re->string, 0);
		xsg_string_insert_len(re->string, 0,
				string->str + match->rm_so,
				match->rm_eo - match->rm_so);
	}

	if (re->var) {
		xsg_var_dirty(re->var);
	}
}

static void *
parse_re(xsg_buffer_t *buffer, xsg_var_t *var, bool ignore_case)
{
	re_t *re;
	int cflags;
	int errcode;

	re = xsg_new(re_t, 1);
	re->var = var;
	re->string = xsg_string_new(NULL);
	re->pattern = xsg_conf_read_string();
	re->index = xsg_conf_read_int();
	re->regex = xsg_new(regex_t, 1);

	cflags = REG_EXTENDED;
	cflags |= (re->index < 0) ? REG_NOSUB : 0;
	cflags |= ignore_case ? REG_ICASE : 0;

	errcode = regcomp(re->regex, re->pattern, cflags);

	if (unlikely(errcode != 0)) {
		char *errbuffer;
		size_t len;

		len = regerror(errcode, re->regex, NULL, 0);
		errbuffer = xsg_new(char, len);
		regerror(errcode, re->regex, errbuffer, len);

		xsg_conf_error("%s", errbuffer);
	}

	return (void *) re;
}

/******************************************************************************/

void
xsg_buffer_parse(
	xsg_buffer_t *buffer,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	if (unlikely(buffer == NULL)) {
		xsg_warning("xsg_buffer_parse: buffer is NULL");
		return;
	}
	if (unlikely(num == NULL)) {
		xsg_warning("xsg_buffer_parse: num is NULL");
		return;
	}
	if (unlikely(str == NULL)) {
		xsg_warning("xsg_buffer_parse: str is NULL");
		return;
	}
	if (unlikely(arg == NULL)) {
		xsg_warning("xsg_buffer_parse: arg is NULL");
		return;
	}

	if (xsg_conf_find_command("read")) {
		read_var_t *read_var;

		read_var = xsg_new(read_var_t, 1);

		buffer->read_var_list = xsg_list_append(buffer->read_var_list,
				(void *) read_var);

		if (buffer->read_string == NULL) {
			buffer->read_string = xsg_string_new(NULL);
		}

		if (xsg_conf_find_command("all")) {
			*str = get_all;
			*arg = parse_all(buffer, var);
			read_var->func = process_all;
			read_var->arg = *arg;
		} else if (xsg_conf_find_command("scanf")) {
			if (xsg_conf_find_command("string")
			 || xsg_conf_find_command("str")
			 || xsg_conf_find_command("s")) {
				*str = get_sscanf;
				*arg = parse_sscanf(buffer, var);
				read_var->func = process_sscanf;
				read_var->arg = *arg;
			} else if (xsg_conf_find_command("number")
				|| xsg_conf_find_command("num")
				|| xsg_conf_find_command("n")) {
				*num = get_nscanf;
				*arg = parse_nscanf(buffer, var);
				read_var->func = process_nscanf;
				read_var->arg = *arg;
			} else if (xsg_conf_find_command("counter")
				|| xsg_conf_find_command("count")
				|| xsg_conf_find_command("c")) {
				*num = get_cscanf;
				*arg = parse_cscanf(buffer, var);
				read_var->func = process_cscanf;
				read_var->arg = *arg;
			} else {
				xsg_conf_error("string, str, s, number, num, "
						"n, counter, count or c "
						"expected");
			}
		} else if (xsg_conf_find_command("grep")) {
			*str = get_re;
			*arg = parse_re(buffer, var, FALSE);
			read_var->func = process_re;
			read_var->arg = *arg;
		} else if (xsg_conf_find_command("igrep")) {
			*str = get_re;
			*arg = parse_re(buffer, var, TRUE);
			read_var->func = process_re;
			read_var->arg = *arg;
		} else {
			xsg_conf_error("all, scanf, "
					"grep or igrep expected");
		}
	} else if (xsg_conf_find_command("readline")) {
		readline_var_t *readline_var;

		readline_var = xsg_new(readline_var_t, 1);

		readline_var->number = xsg_conf_read_uint();

		buffer->readline_var_list = xsg_list_append(
				buffer->readline_var_list,
				(void *) readline_var);

		if (buffer->readline_string == NULL) {
			buffer->readline_string = xsg_string_new(NULL);
		}

		if (xsg_conf_find_command("all")) {
			*str = get_all;
			*arg = parse_all(buffer, var);
			readline_var->func = process_all;
			readline_var->arg = *arg;
		} else if (xsg_conf_find_command("scanf")) {
			if (xsg_conf_find_command("string")
			 || xsg_conf_find_command("str")
			 || xsg_conf_find_command("s")) {
				*str = get_sscanf;
				*arg = parse_sscanf(buffer, var);
				readline_var->func = process_sscanf;
				readline_var->arg = *arg;
			} else if (xsg_conf_find_command("number")
				|| xsg_conf_find_command("num")
				|| xsg_conf_find_command("n")) {
				*num = get_nscanf;
				*arg = parse_nscanf(buffer, var);
				readline_var->func = process_nscanf;
				readline_var->arg = *arg;
			} else if (xsg_conf_find_command("counter")
				|| xsg_conf_find_command("count")
				|| xsg_conf_find_command("c")) {
				*num = get_cscanf;
				*arg = parse_cscanf(buffer, var);
				readline_var->func = process_cscanf;
				readline_var->arg = *arg;
			} else {
				xsg_conf_error("string, str, s, number, num, "
						"n, counter, count or c "
						"expected");
			}
		} else if (xsg_conf_find_command("grep")) {
			*str = get_re;
			*arg = parse_re(buffer, var, FALSE);
			readline_var->func = process_re;
			readline_var->arg = *arg;
		} else if (xsg_conf_find_command("igrep")) {
			*str = get_re;
			*arg = parse_re(buffer, var, TRUE);
			readline_var->func = process_re;
			readline_var->arg = *arg;
		} else {
			xsg_conf_error("all, scanf, "
					"grep or igrep expected");
		}
	} else {
		xsg_conf_error("read or readline expected");
	}
}

/******************************************************************************/

static void
process_read(xsg_buffer_t *buffer)
{
	xsg_list_t *l;

	for (l = buffer->read_var_list; l; l = l->next) {
		read_var_t *read_var = l->data;

		(read_var->func)(read_var->arg, buffer->read_string);
	}
}

static void
process_readline(xsg_buffer_t *buffer)
{
	xsg_list_t *l;

	for (l = buffer->readline_var_list; l; l = l->next) {
		readline_var_t *readline_var = l->data;

		if (readline_var->number == 0) {
			(readline_var->func)(readline_var->arg,
					buffer->readline_string);
		}
		if (readline_var->number == buffer->line_number) {
			(readline_var->func)(readline_var->arg,
					buffer->readline_string);
		}
	}
}

/******************************************************************************/

void
xsg_buffer_add(xsg_buffer_t *buffer, const char *string, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++) {
		xsg_string_append_c(buffer->read_string, string[i]);
		if (buffer->readline_string != NULL) {
			if (likely(string[i] != '\n')) {
				xsg_string_append_c(buffer->readline_string,
						string[i]);
			} else {
				process_readline(buffer);
				xsg_string_truncate(buffer->readline_string, 0);
				buffer->line_number++;
			}
		}
	}
}

/******************************************************************************/

void
xsg_buffer_clear(xsg_buffer_t *buffer)
{
	process_read(buffer);
	process_readline(buffer);
	xsg_string_truncate(buffer->read_string, 0);
	xsg_string_truncate(buffer->readline_string, 0);
	buffer->line_number = 1;
}

/******************************************************************************/

static void
xsg_buffer_help_helper(
	xsg_string_t *string,
	const char *module_name,
	const char *opt,
	const char *s
)
{
	xsg_string_append_printf(string, "S %s:%s:%s:%s\n", module_name,
			opt, s, "all");
	xsg_string_append_printf(string, "S %s:%s:%s:%s\n", module_name,
			opt, s, "scanf:string:<format>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s\n", module_name,
			opt, s, "scanf:number:<format>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s\n", module_name,
			opt, s, "scanf:counter:<format>");
	xsg_string_append_printf(string, "S %s:%s:%s:%s\n", module_name,
			opt, s, "grep:<pattern>:<index>");
	xsg_string_append_printf(string, "S %s:%s:%s:%s\n", module_name,
			opt, s, "igrep:<pattern>:<index>");
}

void
xsg_buffer_help(xsg_string_t *string, const char *module_name, const char *opt)
{
	xsg_buffer_help_helper(string, module_name, opt, "read");
	xsg_buffer_help_helper(string, module_name, opt, "readline:<number>");
}

