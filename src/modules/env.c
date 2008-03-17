/* env.c
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

/******************************************************************************/

typedef struct _env_t {
	char *name;
	uint64_t update;
	xsg_buffer_t *buffer;
} env_t;

/******************************************************************************/

static xsg_list_t *env_list = NULL;

/******************************************************************************/

static xsg_buffer_t *
find_env_buffer(const char *name, uint64_t update)
{
	xsg_list_t *l;
	env_t *e;

	for (l = env_list; l; l = l->next) {
		e = l->data;

		if (strcmp(e->name, name) == 0 && e->update == update) {
			return e->buffer;
		}
	}

	e = xsg_new(env_t, 1);

	e->name = xsg_strdup(name);
	e->update = update;
	e->buffer = xsg_buffer_new();

	env_list = xsg_list_append(env_list, (void *) e);

	return e->buffer;
}

/******************************************************************************/

static void
update_envs(uint64_t update)
{
	xsg_list_t *l;

	for (l = env_list; l; l = l->next) {
		env_t *e = l->data;

		if (update % e->update == 0) {
			const char *value;

			value = xsg_getenv(e->name);

			xsg_buffer_add(e->buffer, value, strlen(value));

			xsg_buffer_clear(e->buffer);
		}
	}
}

/******************************************************************************/

static void
parse_env(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	char *name;
	xsg_buffer_t *buffer;

	name = xsg_conf_read_string();

	buffer = find_env_buffer(name, update);

	xsg_free(name);

	xsg_buffer_parse(buffer, NULL, num, str, arg);

	xsg_main_add_update_func(update_envs);
}

static const char *
help_env(void)
{
	static xsg_string_t *string = NULL;

	if (string != NULL) {
		return string->str;
	}

	string = xsg_string_new(NULL);

	xsg_buffer_help(string, XSG_MODULE_NAME, "<name>");

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_env, help_env, "get environment variables");

