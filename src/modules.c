/* modules.c
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
#include <stdio.h>
#include <dlfcn.h>

#include "modules.h"
#include "conf.h"

#include <glib.h> // FIXME

/******************************************************************************/

#ifndef HOME_MODULE_DIR
#define HOME_MODULE_DIR g_build_filename(g_get_home_dir(), ".xsysguard", "modules", NULL)
#endif

#ifndef MODULE_DIR
#define MODULE_DIR "/usr/lib/xsysguard"
#endif

/******************************************************************************/

typedef struct {
	char *name;
//	char *dir;
	char *file;
} module_t;

/******************************************************************************/

static xsg_list_t *modules_list = NULL;

/******************************************************************************/

// TODO move to utils.c
static char *remove_suffix(const char *string, const char *suffix) {
	if (!xsg_str_has_suffix(string, suffix))
		return NULL;

	return strndup(string, strlen(string) - strlen(suffix));
}

static void init() {
	GDir *dir;
	char **pathv;
	char **p;
	const char *env;
	char *name;
	const char *filename;

	env = g_getenv("XSYSGUARD_MODULE_PATH");

	if (env) {
		pathv = g_strsplit_set(env, ":", 0);
		for (p = pathv; *p; p++)
			if (*p[0] == '~')
				*p = g_build_filename(g_get_home_dir(), *p, NULL);
	} else {
		pathv = xsg_new0(char *, 3);
		pathv[0] = HOME_MODULE_DIR;
		pathv[1] = MODULE_DIR;
		pathv[2] = NULL;
	}

	xsg_message("Searching for modules...");
	for (p = pathv; *p; p++) {
		xsg_message("Searching for modules in \"%s\"", *p);
		if ((dir = g_dir_open(*p, 0, NULL)) == NULL)
			continue;
		while ((filename = g_dir_read_name(dir)) != NULL) {
			if ((name = remove_suffix(filename, ".so")) != NULL) {
				module_t *m = xsg_new0(module_t, 1);
				m->name = name;
				m->file = g_build_filename(*p, filename, NULL);
				modules_list = xsg_list_prepend(modules_list, m);
				xsg_message("Found module file \"%s\"", filename);
			}
		}
		g_dir_close(dir);
	}
}

/******************************************************************************/

void xsg_modules_parse_double(uint32_t var_id, uint64_t update, double (**func)(void *), void **arg) {
	xsg_modules_parse_double_t *parse_double;
	char *filename = NULL;
	void *module;
	module_t *m = NULL;
	xsg_list_t *l;

	if (!modules_list)
		init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		if (xsg_conf_find_command(m->name)) {
			filename = m->file;
			break;
		}
	}

	if (!filename)
		xsg_conf_error("module name");

	module = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);

	if (!module)
		xsg_error("cannot load module \"%s\": %s", m->name, dlerror());

	parse_double = dlsym(module, "parse_double");

	if (!parse_double)
		xsg_error("cannot find \"parse_double\" symbol in module \"%s\": %s", m->name, dlerror());

	*func = NULL;
	*arg = NULL;

	parse_double(var_id, update, func, arg);

	if (*func == NULL)
		xsg_error("module \"%s\" must set func != NULL", m->name);
}

void xsg_modules_parse_string(uint32_t var_id, uint64_t update, char * (**func)(void *), void **arg) {
	xsg_modules_parse_string_t *parse_string;
	char *filename = NULL;
	void *module;
	module_t *m = NULL;
	xsg_list_t *l;

	if (!modules_list)
		init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		if (xsg_conf_find_command(m->name)) {
			filename = m->file;
			break;
		}
	}

	if (!filename)
		xsg_conf_error("module name");

	module = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);

	if (!module)
		xsg_error("cannot load module \"%s\": %s", m->name, dlerror());

	parse_string = dlsym(module, "parse_string");

	if (!parse_string)
		xsg_error("cannot find \"parse_string\" symbol in module \"%s\": %s", m->name, dlerror());

	*func = NULL;
	*arg = NULL;

	parse_string(var_id, update, func, arg);

	if (*func == NULL)
		xsg_error("module \"%s\" must set func != NULL", m->name);
}

void xsg_modules_list() {
	xsg_modules_parse_double_t *parse_double;
	xsg_modules_parse_string_t *parse_string;
	xsg_modules_info_t *info;
	char *filename;
	void *module;
	module_t *m = NULL;
	xsg_list_t *l;

	if (!modules_list)
		init();

	printf("Available modules:\n");
	for (l = modules_list; l; l = l->next) {
		m = l->data;
		filename = m->file;

		module = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);

		if (!module)
			continue;

		parse_double = dlsym(module, "parse_double");
		parse_string = dlsym(module, "parse_string");

		if ((!parse_double) && (!parse_string))
			continue;

		info = dlsym(module, "info");

		if (info)
			printf("%s - %s\n", m->name, info());
		else
			printf("%s\n", m->name);
	}
}

