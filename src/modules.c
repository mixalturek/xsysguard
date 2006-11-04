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
#include "var.h"

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
	char *dir;
} module_t;

/******************************************************************************/

static xsg_list *modules_list = NULL;

/******************************************************************************/

static char *remove_suffix(const char *string, const char *suffix) {
	if (!g_str_has_suffix(string, suffix))
		return NULL;

	return g_strndup(string, strlen(string) - strlen(suffix));
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
		pathv = g_new0(char *, 3);
		pathv[0] = HOME_MODULE_DIR;
		pathv[1] = MODULE_DIR;
		pathv[2] = NULL;
	}

	g_message("Searching for modules...");
	for (p = pathv; *p; p++) {
		g_message("Searching for modules in \"%s\"", *p);
		if ((dir = g_dir_open(*p, 0, NULL)) == NULL)
			continue;
		while ((filename = g_dir_read_name(dir)) != NULL) {
			if ((name = remove_suffix(filename, ".so")) != NULL) {
				module_t *m = g_new0(module_t, 1);
				m->name = name;
				m->dir = *p;
				modules_list = xsg_list_prepend(modules_list, m);
				g_message("Found module file \"%s\"", filename);
			}
		}
		g_dir_close(dir);
	}
}

/******************************************************************************/

uint16_t xsg_modules_parse_var(uint64_t update, uint16_t id) {
	xsg_modules_parse_func parse;
	char *module_filename = NULL;
	void *module;
	module_t *m = NULL;
	xsg_var var;
	xsg_list *l;

	if (!modules_list)
		init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		if (xsg_conf_find_command(m->name)) {
			module_filename = g_build_filename(
					m->dir,
					g_strconcat(m->name, ".so", NULL),
					NULL);
			break;
		}
	}

	if (!module_filename)
		xsg_conf_error("Modulename");

	module = dlopen(module_filename, RTLD_LAZY | RTLD_LOCAL);

	if (!module)
		g_error("Cannot load module \"%s\": %s", m->name, dlerror());

	parse = dlsym(module, XSG_MODULES_PARSE_FUNC);

	if (!parse)
		g_error("Cannot load module \"%s\": %s", m->name, dlerror());

	var.type = 0;
	var.func = NULL;
	var.args = NULL;

	parse(&var, id, update);

	if (var.type == 0 || var.func == NULL)
		g_error("Module \"%s\" must set var->type and var->func != 0", module_filename);

	return xsg_var_add(&var, update, id);
}

void xsg_modules_list() {
	xsg_modules_parse_func parse;
	xsg_modules_info_func info;
	char *module_filename;
	void *module;
	module_t *m = NULL;
	xsg_list *l;

	if (!modules_list)
		init();

	printf("Available modules:\n");
	for (l = modules_list; l; l = l->next) {
		m = l->data;
		module_filename = g_build_filename(m->dir, g_strconcat(m->name, ".so", NULL), NULL);
		module = dlopen(module_filename, RTLD_LAZY | RTLD_LOCAL);

		if (!module)
			continue;

		parse = dlsym(module, XSG_MODULES_PARSE_FUNC);

		if (!parse)
			continue;

		info = dlsym(module, XSG_MODULES_INFO_FUNC);

		if (info)
			printf("%s - %s\n", m->name, info());
		else
			printf("%s\n", m->name);
	}
}

