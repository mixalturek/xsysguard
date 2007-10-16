/* modules.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
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
	xsg_module_t *module;
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

static int module_compare_name(const void *a, const void *b) {
	const module_t *m = a;
	const module_t *n = b;

	return strcmp(m->name, n->name);
}

static void modules_list_insert_sorted(module_t *module) {
	xsg_list_t *l;

	for (l = modules_list; l; l = l->next) {
		module_t *m = l->data;

		if (0 == strcmp(m->name, module->name)) {
			xsg_warning("Found multiple \"%s\" modules. Using \"%s\".", m->name, m->file);
			return;
		}
	}

	modules_list = xsg_list_insert_sorted(modules_list, module, module_compare_name);
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
				if (strcmp(name, filename) != 0) {
					module_t *m = xsg_new(module_t, 1);
					m->name = name;
					m->file = xsg_build_filename(*p, filename, NULL);
					m->module = NULL;
					modules_list_insert_sorted(m);
					xsg_message("Found module in \"%s\": \"%s\"", *p, filename);
				}
			}
		}
		closedir(dir);
	}
	xsg_strfreev(pathv);
}

/******************************************************************************/

static xsg_module_t *module_load(module_t *m) {
	void *handle;
	xsg_module_t *module;

	if (m == NULL)
		xsg_error("Cannot load module NULL");

	if (m->file == NULL)
		xsg_error("Cannot load module: file is NULL");

	if (m->name == NULL)
		xsg_error("Cannot load module: name is NULL");

	if (m->module != NULL)
		return m->module;

	handle = dlopen(m->file, RTLD_NOW);

	if (!handle)
		xsg_error("Cannot load module %s: %s", m->name, dlerror());

	module = (xsg_module_t *) dlsym(handle, "xsg_module");

	if (!module)
		xsg_error("Cannot load module %s: %s", m->name, dlerror());

	if (module->parse == NULL)
		xsg_error("Cannot load module %s: parse function is NULL");

	if (module->help == NULL)
		xsg_error("Cannot load module %s: help function is NULL");

	if (module->info == NULL)
		xsg_error("Cannot load module %s: info char* is NULL");

	module->name = m->name;

	return module;
}

/******************************************************************************/

bool xsg_modules_parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	module_t *m = NULL;
	xsg_module_t *module;
	xsg_list_t *l;
	uint32_t i;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		m = l->data;
		if (xsg_conf_find_command(m->name))
			break;
		else
			m = NULL;
	}

	if (!m)
		return FALSE;

	module = module_load(m);

	for (i = 0; i < n; i++) {
		num[i] = NULL;
		str[i] = NULL;
		arg[i] = NULL;
	}

	module->parse(update, var, num, str, arg, n);

	for (i = 0; i < n; i++) {
		if ((num[i] == NULL) && (str[i] == NULL)) {
			if (n == 1)
				xsg_conf_error("Module %s must set str[0] != NULL or num[0] != NULL", m->name);
			else
				xsg_conf_error("Module %s does not support past variables for this configuration", m->name);
		}
	}

	return TRUE;
}

void xsg_modules_list() {
	xsg_list_t *l;
	xsg_module_t *module;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		module_t *m = l->data;

		module = module_load(m);

		printf("%-12s %s\n", m->name, module->info);
	}
}

const char *xsg_modules_help(const char *name) {
	xsg_module_t *module;
	xsg_list_t *l;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		module_t *m = l->data;

		if (strcmp(m->name, name) != 0)
			continue;

		module = module_load(m);

		return module->help();
	}

	xsg_error("Cannot find module: %s", name);

	return NULL;
}

const char *xsg_modules_info(const char *name) {
	xsg_module_t *module;
	xsg_list_t *l;

	if (!modules_list)
		xsg_modules_init();

	for (l = modules_list; l; l = l->next) {
		module_t *m = l->data;

		if (strcmp(m->name, name) != 0)
			continue;

		module = module_load(m);

		return module->info;
	}

	xsg_error("Cannot find module: %s", name);

	return NULL;
}

