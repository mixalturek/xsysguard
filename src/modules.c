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
#include <sys/types.h>
#include <dirent.h>

#include "modules.h"
#include "conf.h"

/******************************************************************************/

typedef struct {
	char *name;
	char *file;
} module_t;

/******************************************************************************/

static xsg_list_t *modules_list = NULL;

/******************************************************************************/

static const char *read_dir_name(DIR *dir) {
	struct dirent *entry;

	if (unlikely(dir == NULL))
		return NULL;

	entry = readdir(dir);

	while (entry && (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, "..")))
		entry = readdir(dir);

	if (entry)
		return entry->d_name;
	else
		return NULL;
}

/******************************************************************************/

void xsg_modules_init(void) {
	DIR *dir;
	char **pathv;
	char **p;
	char *name;
	const char *filename;

	pathv = xsg_get_path_from_env("XSYSGUARD_MODULE_PATH", XSYSGUARD_MODULE_PATH);

	if (unlikely(pathv == NULL))
		xsg_error("Cannot get XSYSGUARD_MODULE_PATH");

	xsg_debug("Searching for modules...");
	for (p = pathv; *p; p++) {
		xsg_debug("Searching for modules in \"%s\"", *p);
		if ((dir = opendir(*p)) == NULL)
			continue;
		while ((filename = read_dir_name(dir)) != NULL) {
			if ((name = xsg_str_without_suffix(filename, ".so")) != NULL) {
				module_t *m = xsg_new(module_t, 1);
				m->name = name;
				m->file = xsg_build_filename(*p, filename, NULL);
				modules_list = xsg_list_prepend(modules_list, m);
				xsg_message("Found module in \"%s\": \"%s\"", *p, filename);
			}
		}
		closedir(dir);
	}
	xsg_strfreev(pathv);
}

/******************************************************************************/

void xsg_modules_parse(uint64_t update, xsg_var_t *var, double (**num)(void *), char *(**str)(void *), void **arg) {
	xsg_modules_parse_t *parse;
	char *filename = NULL;
	void *module;
	module_t *m = NULL;
	xsg_list_t *l;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		if (xsg_conf_find_command(m->name)) {
			filename = m->file;
			break;
		}
	}

	if (!filename)
		xsg_conf_error("module name expected");

	module = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);

	if (!module)
		xsg_error("Cannot load module %s: %s", m->name, dlerror());

	parse = dlsym(module, "parse");

	if (!parse)
		xsg_error("Cannot load module %s: %s", m->name, dlerror());

	*num = NULL;
	*str = NULL;
	*arg = NULL;

	parse(update, var, num, str, arg);

	if ((*num == NULL) && (*str == NULL))
		xsg_error("module %s must set str != NULL or num != NULL", m->name);
}

void xsg_modules_list() {
	xsg_modules_info_t *info;
	xsg_modules_parse_t *parse;
	xsg_modules_pparse_t *pparse;
	char *filename;
	void *module;
	module_t *m = NULL;
	xsg_list_t *l;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		filename = m->file;

		module = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);

		if (!module) {
			xsg_warning("Cannot load module %s: %s", m->name, dlerror());
			continue;
		}

		info = dlsym(module, "info");
		parse = dlsym(module, "parse");
		pparse = dlsym(module, "pparse");

		if (!parse && !pparse) {
			xsg_warning("Cannot load module %s: %s", m->name, dlerror());
			continue;
		}

		if (info)
			printf("%s:\t %s   (%s)\n", m->name, info(), m->file);
		else
			printf("%s:\t   (%s)\n", m->name, m->file);
	}
}

