/* xsysguardd.c
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "modules.h"
#include "conf.h"
#include "main.h"
#include "var.h"

/******************************************************************************/

static uint32_t log_level = LOG_LEVEL_ERROR;

void xsg_log(const char *domain, uint32_t level, const char *format, va_list args) {
	char buffer[1025];
	uint16_t len = 0;
	uint8_t type = 0;
	uint8_t l;

	if (log_level < level)
		return;

	if (level == LOG_LEVEL_ERROR)
		len += snprintf(buffer, sizeof(buffer) - 1 - len, "ERROR: ");
	else if (level == LOG_LEVEL_WARNING)
		len += snprintf(buffer, sizeof(buffer) - 1 - len, "WARNING: ");
	else if (level == LOG_LEVEL_MESSAGE)
		len += snprintf(buffer, sizeof(buffer) - 1 - len, "MESSAGE: ");
	else if (level == LOG_LEVEL_DEBUG)
		len += snprintf(buffer, sizeof(buffer) - 1 - len, "DWBUG: ");

	if (domain != NULL)
		len += snprintf(buffer, sizeof(buffer) - 1 - len, "[%s] ", domain);

	len += vsnprintf(buffer, sizeof(buffer) - 1 - len, format, args);

	len += snprintf(buffer, sizeof(buffer) - 1 - len, "\n");

	l = level;

	fwrite(&type, 1, 1, stdout);
	fwrite(&l, 1, 1, stdout);
	fwrite(&len, 2, 1, stdout);
	fwrite(buffer, len, 1, stdout);
	fflush(stdout);
}

/******************************************************************************/

static void read_data(void *ptr, size_t size, FILE *stream) {

	if (fread(ptr, 1, size, stream) != size)
		xsg_error("cannot read configuration");
}

static void read_config(FILE *stream) {

	while (1) {
		uint8_t command;
		read_data(&command, 1, stream);
		if (command == 0x00) {
			break;
		} else if (command == 0x01) {
			uint64_t interval;
			read_data(&interval, 8, stream);
			xsg_main_set_interval(GUINT64_FROM_BE(interval));
		} else if (command == 0x02) {
			read_data(&log_level, 1, stream);
		} else if (command == 0x03) {
			uint16_t var_id;
			uint64_t update;
			uint16_t config_len;
			char *config;

			read_data(&var_id, 2, stream);
			var_id = GUINT32_FROM_BE(var_id);
			read_data(&update, 8, stream);
			update = GUINT64_FROM_BE(update);
			read_data(&config_len, 2, stream);
			config_len = GUINT16_FROM_BE(config_len);
			config = xsg_new0(char, config_len + 1);
			read_data(config, config_len, stream);
			xsg_conf_set_buffer(config);
			xsg_var_parse(update, var_id);
			xsg_free(config);
		} else {
			xsg_error("inconsistent configuration");
		}
	}
}


int main(int argc, char **argv) {
	bool list_modules = FALSE;
	GOptionContext *context;
	GError *error = NULL;
	int log = 0;

	GOptionEntry option_entries[] = {
		{ "log", 'l', 0, G_OPTION_ARG_INT, &log,
			"Set loglevel to N = 0 ... 5 (DEBUG)", "N" },
		{ "modules", 'm', 0, G_OPTION_ARG_NONE, &list_modules,
			"Print a list of all available modules to stdout", NULL },
		{ NULL }
	};

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, option_entries, NULL);
	g_option_context_parse(context, &argc, &argv, &error);
	g_option_context_free(context);

	if (error)
		xsg_error("%s", error->message);

	log_level = log;

	if (list_modules) {
		xsg_modules_list();
		exit(0);
	}

	xsg_var_init();

	read_config(stdin);

	xsg_main_loop();

	return 0;
}

