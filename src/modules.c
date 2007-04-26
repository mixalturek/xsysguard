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

static void init() {
	DIR *dir;
	char **pathv;
	char **p;
	char *name;
	const char *filename;

	pathv = xsg_get_path_from_env("XSYSGUARD_MODULE_PATH", XSYSGUARD_MODULE_PATH);

	if (unlikely(pathv == NULL))
		xsg_error("Cannot get XSYSGUARD_MODULE_PATH");

	xsg_message("Searching for modules...");
	for (p = pathv; *p; p++) {
		xsg_message("Searching for modules in \"%s\"", *p);
		if ((dir = opendir(*p)) == NULL)
			continue;
		while ((filename = read_dir_name(dir)) != NULL) {
			if ((name = xsg_str_without_suffix(filename, ".so")) != NULL) {
				module_t *m = xsg_new0(module_t, 1);
				m->name = name;
				m->file = xsg_build_filename(*p, filename, NULL);
				modules_list = xsg_list_prepend(modules_list, m);
				xsg_message("Found module file \"%s\"", filename);
			}
		}
		closedir(dir);
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

