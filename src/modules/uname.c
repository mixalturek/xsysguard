/* uname.c
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
#include <sys/utsname.h>

/******************************************************************************/

static struct utsname utsname;

/******************************************************************************/

static const char *
get_sysname(void *arg)
{
	if (uname(&utsname) < 0) {
		xsg_debug("get_sysname: %s", "ERROR");
		return NULL;
	} else {
		xsg_debug("get_sysname: %s", utsname.sysname);
		return utsname.sysname;
	}
}

static const char *
get_nodename(void *arg)
{
	if (uname(&utsname) < 0) {
		xsg_debug("get_nodename: %s", "ERROR");
		return NULL;
	} else {
		xsg_debug("get_nodename: %s", utsname.nodename);
		return utsname.nodename;
	}
}

static const char *
get_release(void *arg)
{
	if (uname(&utsname) < 0) {
		xsg_debug("get_release: %s", "ERROR");
		return NULL;
	} else {
		xsg_debug("get_release: %s", utsname.release);
		return utsname.release;
	}
}

static const char *
get_version(void *arg)
{
	if (uname(&utsname) < 0) {
		xsg_debug("get_version: %s", "ERROR");
		return NULL;
	} else {
		xsg_debug("get_version: %s", utsname.version);
		return utsname.version;
	}
}

static const char *
get_machine(void *arg)
{
	if (uname(&utsname) < 0) {
		xsg_debug("get_machine: %s", "ERROR");
		return NULL;
	} else {
		xsg_debug("get_machine: %s", utsname.machine);
		return utsname.machine;
	}
}

/******************************************************************************/

static void
parse_uname(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("sysname")) {
		*str = get_sysname;
	} else if (xsg_conf_find_command("nodename")) {
		*str = get_nodename;
	} else if (xsg_conf_find_command("release")) {
		*str = get_release;
	} else if (xsg_conf_find_command("version")) {
		*str = get_version;
	} else if (xsg_conf_find_command("machine")) {
		*str = get_machine;
	} else {
		xsg_conf_error("sysname, nodename, release, version or machine expected");
	}
}

static const char *
help_uname(void)
{
	static xsg_string_t *string = NULL;

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	if (uname(&utsname) < 0) {
		utsname.sysname[0] = '\0';
		utsname.nodename[0] = '\0';
		utsname.release[0] = '\0';
		utsname.version[0] = '\0';
		utsname.machine[0] = '\0';
	}

	xsg_string_append_printf(string, "S %s:%-20s%s\n", XSG_MODULE_NAME,
			"sysname", utsname.sysname);
	xsg_string_append_printf(string, "S %s:%-20s%s\n", XSG_MODULE_NAME,
			"nodename", utsname.nodename);
	xsg_string_append_printf(string, "S %s:%-20s%s\n", XSG_MODULE_NAME,
			"release", utsname.release);
	xsg_string_append_printf(string, "S %s:%-20s%s\n", XSG_MODULE_NAME,
			"version", utsname.version);
	xsg_string_append_printf(string, "S %s:%-20s%s\n", XSG_MODULE_NAME,
			"machine", utsname.machine);

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_uname, help_uname, "get name and information about current kernel");

