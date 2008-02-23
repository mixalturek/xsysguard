/* time.c
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
#include <time.h>

/******************************************************************************/

typedef struct _strftime_args_t {
	bool local;
	char *format;
	xsg_string_t *buffer;
} strftime_args_t;

/******************************************************************************/

static xsg_list_t *var_list = NULL;
static xsg_main_timeout_t *timeout = NULL;

/******************************************************************************/

static void
set_timeout(struct timeval *tv)
{
	struct timeval now;

	xsg_gettimeofday(&now, NULL);

	tv->tv_sec = (now.tv_sec / 60 + 1) * 60;
	tv->tv_usec = 0;
}

static void
timeout_handler(void *arg, bool time_error)
{
	xsg_list_t *l;

	for (l = var_list; l; l = l->next) {
		xsg_var_t *var = l->data;

		xsg_var_dirty(var);
	}

	set_timeout(&timeout->tv);
}

static void
init(void)
{
	timeout = xsg_new(xsg_main_timeout_t, 1);

	timeout->tv.tv_sec = 0;
	timeout->tv.tv_usec = 0;
	timeout->func = timeout_handler;
	timeout->arg = NULL;

	set_timeout(&timeout->tv);
	xsg_main_add_timeout(timeout);
}

/******************************************************************************/

static struct tm *
gm_time_tm(void)
{
	time_t curtime;
	static struct tm *tm;

	curtime = time(NULL);
	tm = gmtime(&curtime);

	if (unlikely(tm == NULL)) {
		xsg_error("gmtime returned NULL");
	}

	return tm;
}

static struct tm *
local_time_tm(void)
{
	time_t curtime;
	static struct tm *tm;

	curtime = time(NULL);
	tm = localtime(&curtime);

	if (unlikely(tm == NULL)) {
		xsg_error("localtime returned NULL");
	}

	return tm;
}

static struct tm *
time_tm(bool local)
{
	if (local) {
		return local_time_tm();
	} else {
		return gm_time_tm();
	}
}

/******************************************************************************/

static char *
get_strftime(void *arg)
{
	strftime_args_t *args = (strftime_args_t *) arg;
	struct tm *tm;

	tm = time_tm(args->local);

	while (1) {
		size_t len, buf_len;

		buf_len = args->buffer->allocated_len;
		len = strftime(args->buffer->str, buf_len, args->format, tm);

		if (likely(len != 0)) {
			break;
		}

		xsg_string_set_size(args->buffer, (buf_len + 1) * 2);
	}

	xsg_debug("get_strftime: \"%s\"", args->buffer->str);

	return args->buffer->str;
}

static double
get_sec(void *arg)
{
	bool local = *((bool *) arg);
	time_t curtime;
	double d;

	curtime = time(NULL);

	if (local) {
		d = (double) mktime(localtime(&curtime));
	} else {
		d = (double) mktime(gmtime(&curtime));
	}

	xsg_debug("get_tv_sec: %f", d);

	return d;
}

static double
get_usec(void *arg)
{
	struct timeval tv;
	double d;

	xsg_gettimeofday(&tv, NULL);
	d = (double) tv.tv_usec;
	xsg_debug("get_tv_usec: %f", d);

	return d;
}

static double
get_tm_sec(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_sec;
	xsg_debug("get_tm_sec: %f", d);

	return d;
}

static double
get_tm_min(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_min;
	xsg_debug("get_tm_min: %f", d);

	return d;
}

static double
get_tm_hour(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_hour;
	xsg_debug("get_tm_hour: %f", d);

	return d;
}

static double
get_tm_mday(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_mday;
	xsg_debug("get_tm_mday: %f", d);

	return d;
}

static double
get_tm_mon(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_mon;
	xsg_debug("get_tm_mon: %f", d);

	return d;
}

static double
get_tm_year(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_year;
	xsg_debug("get_tm_year: %f", d);

	return d;
}

static double
get_tm_wday(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_wday;
	xsg_debug("get_tm_wday: %f", d);

	return d;
}

static double
get_tm_yday(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_yday;
	xsg_debug("get_tm_yday: %f", d);

	return d;
}

static double
get_tm_isdst(void *arg)
{
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_isdst;
	xsg_debug("get_tm_isdst: %f", d);

	return d;
}

/******************************************************************************/

static void
parse(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	bool local = TRUE;

	if (xsg_conf_find_command("gm")) {
		local = FALSE;
	} else if (xsg_conf_find_command("local")) {
		local = TRUE;
	} else {
		xsg_conf_error("gm or local expected");
	}

	if (xsg_conf_find_command("strftime")) {
		strftime_args_t *args;

		args = xsg_new(strftime_args_t, 1);
		args->local = local;
		args->format = xsg_conf_read_string();
		args->buffer = xsg_string_new(NULL);

		*str = get_strftime;
		*arg = args;
	} else {
		bool *args;

		args = xsg_new(bool, 1);
		*args = local;

		*arg = args;

		if (xsg_conf_find_command("sec")) {
			*num = get_sec;
		} else if (xsg_conf_find_command("usec")) {
			*num = get_usec;
		} else if (xsg_conf_find_command("tm_sec")) {
			*num = get_tm_sec;
		} else if (xsg_conf_find_command("tm_min")) {
			*num = get_tm_min;
		} else if (xsg_conf_find_command("tm_hour")) {
			*num = get_tm_hour;
		} else if (xsg_conf_find_command("tm_mday")) {
			*num = get_tm_mday;
		} else if (xsg_conf_find_command("tm_mon")) {
			*num = get_tm_mon;
		} else if (xsg_conf_find_command("tm_year")) {
			*num = get_tm_year;
		} else if (xsg_conf_find_command("tm_wday")) {
			*num = get_tm_wday;
		} else if (xsg_conf_find_command("tm_yday")) {
			*num = get_tm_yday;
		} else if (xsg_conf_find_command("tm_isdst")) {
			*num = get_tm_isdst;
		} else {
			xsg_conf_error("strftime, sec, usec, tm_sec, tm_min, "
					"tm_hour, tm_mday, tm_mon, tm_year, "
					"tm_wday, tm_yday or tm_isdst "
					"expected");
		}
	}

	var_list = xsg_list_append(var_list, (void *) var);

	xsg_main_add_init_func(init);
}

static const char *
help(void)
{
	static xsg_string_t *string = NULL;
	bool local;

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	for (local = 0; local < 2; local++) {
		char *format;

		if (local) {
			xsg_string_append_printf(string,
					"\nS %s:local:strftime:<format>\n\n",
					xsg_module.name);
			format = "N %s:local:%-20s %.0f\n";
		} else {
			xsg_string_append_printf(string,
					"S %s:gm:strftime:<format>\n\n",
					xsg_module.name);
			format = "N %s:gm:%-23s %.0f\n";
		}

		xsg_string_append_printf(string, format, xsg_module.name,
				"sec", get_sec(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"usec", get_usec(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_sec", get_tm_sec(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_min", get_tm_min(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_hour", get_tm_hour(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_mday", get_tm_mday(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_mon", get_tm_mon(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_year", get_tm_year(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_wday", get_tm_wday(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_yday", get_tm_yday(&local));
		xsg_string_append_printf(string, format, xsg_module.name,
				"tm_isdst", get_tm_isdst(&local));
	}

	return string->str;
}

xsg_module_t xsg_module = {
	parse, help, "date and time functions"
};

