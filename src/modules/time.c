/* time.c
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
#include <time.h>

/******************************************************************************/

typedef struct _strftime_args_t {
	bool local;
	char *format;
	xsg_string_t *buffer;
} strftime_args_t;

/******************************************************************************/

static struct tm *gm_time_tm() {
	time_t curtime;
	struct tm *tm;

	curtime = time(NULL);
	tm = gmtime(&curtime);

	if (unlikely(tm == NULL))
		xsg_error("gmtime returned NULL");

	return tm;
}

static struct tm *local_time_tm() {
	time_t curtime;
	struct tm *tm;

	curtime = time(NULL);
	tm = localtime(&curtime);

	if (unlikely(tm == NULL))
		xsg_error("localtime returned NULL");

	return tm;
}

static struct tm *time_tm(bool local) {
	if (local)
		return local_time_tm();
	else
		return gm_time_tm();
}

/******************************************************************************/

static char *get_strftime(void *arg) {
	strftime_args_t *args = (strftime_args_t *) arg;
	struct tm *tm;

	tm = time_tm(args->local);

	while (1) {
		size_t len, buf_len;

		buf_len = args->buffer->allocated_len;
		len = strftime(args->buffer->str, buf_len, args->format, tm);

		if (likely(len != 0))
			break;

		args->buffer = xsg_string_set_size(args->buffer, (buf_len + 1) * 2);
	}

	xsg_debug("get_strftime: \"%s\"", args->buffer->str);

	return args->buffer->str;
}

static char *get_ctime(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	char *s;

	tm = time_tm(local);

	s = asctime(tm);

	xsg_debug("get_ctime: \"%s\"", s);

	return s;
}

static double get_tv_sec(void *arg) {
	bool local = *((bool *) arg);
	struct timeval tv;
	double d;

	xsg_gettimeofday(&tv, NULL);

	if (local)
		d = (double) mktime(localtime(&tv.tv_sec));
	else
		d = (double) tv.tv_sec;

	xsg_debug("get_tv_sec: %f", d);

	return d;
}

static double get_tv_usec(void *arg) {
	struct timeval tv;
	double d;

	xsg_gettimeofday(&tv, NULL);
	d = (double) tv.tv_usec;
	xsg_debug("get_tv_usec: %f", d);
	return d;
}

static double get_tm_sec(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_sec;
	xsg_debug("get_tm_sec: %f", d);
	return d;
}

static double get_tm_min(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_min;
	xsg_debug("get_tm_min: %f", d);
	return d;
}

static double get_tm_hour(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_hour;
	xsg_debug("get_tm_hour: %f", d);
	return d;
}

static double get_tm_mday(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_mday;
	xsg_debug("get_tm_mday: %f", d);
	return d;
}

static double get_tm_mon(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_mon;
	xsg_debug("get_tm_mon: %f", d);
	return d;
}

static double get_tm_year(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_year;
	xsg_debug("get_tm_year: %f", d);
	return d;
}

static double get_tm_wday(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_wday;
	xsg_debug("get_tm_wday: %f", d);
	return d;
}

static double get_tm_yday(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_yday;
	xsg_debug("get_tm_yday: %f", d);
	return d;
}

static double get_tm_isdst(void *arg) {
	bool local = *((bool *) arg);
	struct tm *tm;
	double d;

	tm = time_tm(local);
	d = (double) tm->tm_isdst;
	xsg_debug("get_tm_isdst: %f", d);
	return d;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
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

		*s = get_strftime;
		*arg = args;
	} else {
		bool *args;

		args = xsg_new(bool, 1);
		*args = local;

		*arg = args;

		if (xsg_conf_find_command("ctime")) {
			*s = get_ctime;
		} else if (xsg_conf_find_command("tv_sec")) {
			*n = get_tv_sec;
		} else if (xsg_conf_find_command("tv_usec")) {
			*n = get_tv_usec;
		} else if (xsg_conf_find_command("tm_sec")) {
			*n = get_tm_sec;
		} else if (xsg_conf_find_command("tm_min")) {
			*n = get_tm_min;
		} else if (xsg_conf_find_command("tm_hour")) {
			*n = get_tm_hour;
		} else if (xsg_conf_find_command("tm_mday")) {
			*n = get_tm_mday;
		} else if (xsg_conf_find_command("tm_mon")) {
			*n = get_tm_mon;
		} else if (xsg_conf_find_command("tm_year")) {
			*n = get_tm_year;
		} else if (xsg_conf_find_command("tm_wday")) {
			*n = get_tm_wday;
		} else if (xsg_conf_find_command("tm_yday")) {
			*n = get_tm_yday;
		} else if (xsg_conf_find_command("tm_isdst")) {
			*n = get_tm_isdst;
		} else {
			xsg_conf_error("strftime, ctime, tv_sec, tv_usec, tm_sec, tm_min, tm_hour, tm_mday, tm_mon, tm_year, tm_wday, tm_yday or tm_isdst expected");
		}
	}
}

char *info() {
	return "date and time functions";
}

