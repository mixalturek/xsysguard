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

#define XSG_MODULE(parse, help, info) static const char info_daemon[] = info
#define XSG_MODULE_NAME "daemon"
#include "modules/daemon.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_env[] = info
#define XSG_MODULE_NAME "env"
#include "modules/env.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_exec[] = info
#define XSG_MODULE_NAME "exec"
#include "modules/exec.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_file[] = info
#define XSG_MODULE_NAME "file"
#include "modules/file.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_inotail[] = info
#define XSG_MODULE_NAME "inotail"
#include "modules/inotail.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_number[] = info
#define XSG_MODULE_NAME "number"
#include "modules/number.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_random[] = info
#define XSG_MODULE_NAME "random"
#include "modules/random.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_string[] = info
#define XSG_MODULE_NAME "string"
#include "modules/string.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_tail[] = info
#define XSG_MODULE_NAME "tail"
#include "modules/tail.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_tick[] = info
#define XSG_MODULE_NAME "tick"
#include "modules/tick.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_time[] = info
#define XSG_MODULE_NAME "time"
#include "modules/time.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#define XSG_MODULE(parse, help, info) static const char info_uname[] = info
#define XSG_MODULE_NAME "uname"
#include "modules/uname.c"
#undef XSG_MODULE_NAME
#undef XSG_MODULE

#undef XSG_LOG_DOMAIN
#define XSG_LOG_DOMAIN NULL

/******************************************************************************/

xsg_module_t xsg_modules_array[] = {
	{ parse_daemon, help_daemon, info_daemon, "daemon" },
	{ parse_env, help_env, info_env, "env" },
	{ parse_exec, help_exec, info_exec, "exec" },
	{ parse_file, help_file, info_file, "file" },
	{ parse_inotail, help_inotail, info_inotail, "inotail" },
	{ parse_number, help_number, info_number, "number" },
	{ parse_random, help_random, info_random, "random" },
	{ parse_string, help_string, info_string, "string" },
	{ parse_tail, help_tail, info_tail, "tail" },
	{ parse_tick, help_tick, info_tick, "tick" },
	{ parse_time, help_time, info_time, "time" },
	{ parse_uname, help_uname, info_uname, "uname" },
	{ NULL }
};

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
	const char *(**str)(void *),
	void **arg
)
{
	xsg_module_t *m;

	num[0] = NULL;
	str[0] = NULL;
	arg[0] = NULL;

	for (m = xsg_modules_array; m->parse; m++) {
		if (xsg_conf_find_command(m->name)) {
			m->parse(update, var, num, str, arg);

			if ((num[0] == NULL) && (str[0] == NULL)) {
				xsg_conf_error("module must set *str != NULL "
						"or *num != NULL");
			}

			return TRUE;
		}
	}

	return FALSE;
}

void
xsg_modules_list(void)
{
	xsg_module_t *m;

	for (m = xsg_modules_array; m->parse; m++) {
		printf("%-12s %s\n", m->name, m->info);
	}
}

const char *
xsg_modules_help(const char *name)
{
	xsg_module_t *m;

	for (m = xsg_modules_array; m->parse; m++) {
		if (!strcmp(name, m->name)) {
			return m->help();
		}
	}

	xsg_error("cannot find module: %s", name);

	return NULL;
}

const char *
xsg_modules_info(const char *name)
{
	xsg_module_t *m;

	for (m = xsg_modules_array; m->parse; m++) {
		if (!strcmp(name, m->name)) {
			return m->info;
		}
	}

	xsg_error("cannot find module: %s", name);

	return NULL;
}

