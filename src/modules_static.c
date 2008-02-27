/* modules.c
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
#include <string.h>
#include <stdio.h>

#include "modules.h"
#include "conf.h"

/******************************************************************************/

#undef XSG_LOG_DOMAIN
#define XSG_LOG_DOMAIN XSG_MODULE_NAME

#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char *info_number = info
#define XSG_MODULE_NAME "number"
#include "modules/number.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char *info_string = info
#define XSG_MODULE_NAME "string"
#include "modules/string.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char *info_file = info
#define XSG_MODULE_NAME "file"
#include "modules/file.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#undef XSG_LOG_DOMAIN
#define XSG_LOG_DOMAIN NULL

/******************************************************************************/

void
xsg_modules_init(void)
{
}

bool
xsg_modules_parse(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	num[0] = NULL;
	str[0] = NULL;
	arg[0] = NULL;

	if (xsg_conf_find_command("number")) {
		parse_number(update, var, num, str, arg);
	} else if (xsg_conf_find_command("string")) {
		parse_string(update, var, num, str, arg);
	} else if (xsg_conf_find_command("file")) {
		parse_file(update, var, num, str, arg);
	} else {
		return FALSE;
	}

	if ((num[0] == NULL) && (str[0] == NULL)) {
		xsg_conf_error("module must set *str != NULL or "
				"*num != NULL");
	}

	return TRUE;
}

void
xsg_modules_list()
{
	printf("%-12s %s\n", "file", info_file);
	printf("%-12s %s\n", "number", info_number);
	printf("%-12s %s\n", "string", info_string);
}

const char *
xsg_modules_help(const char *name)
{
	if (!strcmp(name, "number")) {
		return help_number();
	} else if (!strcmp(name, "string")) {
		return help_string();
	} else if (!strcmp(name, "file")) {
		return help_file();
	}

	xsg_error("cannot find module: %s", name);

	return NULL;
}

const char *
xsg_modules_info(const char *name)
{
	if (!strcmp(name, "number")) {
		return info_number;
	} else if (!strcmp(name, "string")) {
		return info_string;
	} else if (!strcmp(name, "file")) {
		return info_file;
	}

	xsg_error("cannot find module: %s", name);

	return NULL;
}

