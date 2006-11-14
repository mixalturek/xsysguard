/* xsysguard.c
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

#include "modules.h"
#include "widgets.h"
#include "conf.h"
#include "main.h"
#include "var.h"

/******************************************************************************/

#ifndef DEFAULT_CONFIG_FILE
#define DEFAULT_CONFIG_FILE g_build_filename(g_get_home_dir(), ".xsysguard", "configs",	"default", NULL)
#endif

#ifndef HOME_CONFIG_DIR
#define HOME_CONFIG_DIR g_build_filename(g_get_home_dir(), ".xsysguard", "configs", NULL)
#endif

#ifndef CONFIG_DIR
#define CONFIG_DIR "/usr/share/xsysguard/configs"
#endif

/******************************************************************************/

static uint32_t log_level = LOG_LEVEL_ERROR;

void xsg_log(const char *domain, uint32_t level, const char *format, va_list args) {
	if (log_level < level)
		return;

	if (level == LOG_LEVEL_ERROR)
		fprintf(stderr, "ERROR: ");
	else if (level == LOG_LEVEL_WARNING)
		fprintf(stderr, "WARNING: ");
	else if (level == LOG_LEVEL_MESSAGE)
		fprintf(stderr, "MESSAGE: ");
	else if (level == LOG_LEVEL_DEBUG)
		fprintf(stderr, "DEBUG: ");

	if (domain != NULL)
		fprintf(stderr, "[%s] ", domain);

	vfprintf(stderr, format, args);

	fprintf(stderr, "\n");
}

/******************************************************************************/

static void parse_env() {
	char *variable;
	char *value;
	bool overwrite = FALSE;

	variable = xsg_conf_read_string();
	value = xsg_conf_read_string();
	overwrite = xsg_conf_find_command("Overwrite");

	xsg_message("Setting environment variable %s = %s", variable, value);

	if (!g_setenv(variable, value, overwrite))
		xsg_warning("Cannot set environment variable %s = %s", variable, value);
}

static void parse_config(char *config_buffer) {
	uint64_t update;
	uint16_t val_id;
	uint16_t widget_id;

	xsg_conf_set_buffer(config_buffer);

	while (!xsg_conf_find_end()) {
		if (xsg_conf_find_newline())
			continue;
		if (xsg_conf_find_comment())
			continue;
		if (xsg_conf_find_command("Set")) {
			if (xsg_conf_find_command("Interval")) {
				uint64_t interval;
				interval = xsg_conf_read_uint();
				xsg_main_set_interval(interval);
				xsg_conf_read_newline();
			} else if (xsg_conf_find_command("Name")) {
				xsg_widgets_parse_name();
			} else if (xsg_conf_find_command("Class")) {
				xsg_widgets_parse_class();
			} else if (xsg_conf_find_command("Resource")) {
				xsg_widgets_parse_resource();
			} else if (xsg_conf_find_command("Geometry")) {
				xsg_widgets_parse_geometry();
			} else if (xsg_conf_find_command("Sticky")) {
				xsg_widgets_parse_sticky();
			} else if (xsg_conf_find_command("SkipTaskbar")) {
				xsg_widgets_parse_skip_taskbar();
			} else if (xsg_conf_find_command("SkipPager")) {
				xsg_widgets_parse_skip_pager();
			} else if (xsg_conf_find_command("Layer")) {
				xsg_widgets_parse_layer();
			} else if (xsg_conf_find_command("Decorations")) {
				xsg_widgets_parse_decorations();
			} else if (xsg_conf_find_command("Background")) {
				xsg_widgets_parse_background();
			} else if (xsg_conf_find_command("CacheSize")) {
				xsg_widgets_parse_cache_size();
			} else if (xsg_conf_find_command("XShape")) {
				xsg_widgets_parse_xshape();
			} else if (xsg_conf_find_command("ARGBVisual")) {
				xsg_widgets_parse_argb_visual();
			} else {
				xsg_conf_error("Interval, IconName, Class, Resource, Geometry, "
						"Sticky, SkipTaskbar, SkipPager, Layer, "
						"Decorations, Background, CacheSize, XShape "
						"or ARGBVisual");
			}
		} else if (xsg_conf_find_command("SetEnv")) {
			parse_env();
		} else if (xsg_conf_find_command("Line")) {
			xsg_widgets_parse_line();
		} else if (xsg_conf_find_command("Rectangle")) {
			xsg_widgets_parse_rectangle();
		} else if (xsg_conf_find_command("Ellipse")) {
			xsg_widgets_parse_ellipse();
		} else if (xsg_conf_find_command("Polygon")) {
			xsg_widgets_parse_polygon();
		} else if (xsg_conf_find_command("Image")) {
			xsg_widgets_parse_image();
		} else if (xsg_conf_find_command("BarChart")) {
			xsg_widgets_parse_barchart(&update, &widget_id);
			while ((val_id = xsg_var_parse(update, widget_id)) != 0)
				xsg_widgets_parse_barchart_val(val_id);
		} else if (xsg_conf_find_command("LineChart")) {
			xsg_widgets_parse_linechart(&update, &widget_id);
			while ((val_id = xsg_var_parse(update, widget_id)) != 0)
				xsg_widgets_parse_linechart_val(val_id);
		} else if (xsg_conf_find_command("AreaChart")) {
			xsg_widgets_parse_areachart(&update, &widget_id);
			while ((val_id = xsg_var_parse(update, widget_id)) != 0)
				xsg_widgets_parse_areachart_val(val_id);
		} else if (xsg_conf_find_command("Text")) {
			xsg_widgets_parse_text(&update, &widget_id);
			while ((val_id = xsg_var_parse(update, widget_id)) != 0)
				xsg_widgets_parse_text_val(val_id);
		} else if (xsg_conf_find_command("ImageText")) {
			xsg_widgets_parse_imagetext(&update, &widget_id);
			while ((val_id = xsg_var_parse(update, widget_id)) != 0)
				xsg_widgets_parse_imagetext_val(val_id);
		} else {
			xsg_conf_error("#, Set, SetEnv, Line, Rectangle, Ellipse, Polygon, "
					"Image, BarChart, LineChart, AreaChart, Text or ImageText");
		}
	}
}

static char *find_config_file(char *name) {
	char *file = NULL;

	file = g_build_filename(HOME_CONFIG_DIR, name, NULL);
	if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
		return file;

	file = g_build_filename(CONFIG_DIR, name, NULL);
	if (g_file_test(file, G_FILE_TEST_IS_REGULAR))
		return file;

	xsg_error("Cannot find file \"%s\"", name);
	return 0;
}

/******************************************************************************
 *
 * main
 *
 ******************************************************************************/

int main(int argc, char **argv) {
	int log = 0;
	char *file = NULL;
	bool list_modules = FALSE;
	char **args = NULL;
	GOptionContext *context;
	char *config_buffer = NULL;
	GError *error = NULL;

	GOptionEntry option_entries[] = {
		{ "log", 'l', 0, G_OPTION_ARG_INT, &log,
			"Set loglevel to N = 0 ... 5 (DEBUG)", "N" },
		{ "file", 'f', 0, G_OPTION_ARG_FILENAME, &file,
			"Read configuration from FILE (absolute path)", "FILE" },
		{ "modules", 'm', 0, G_OPTION_ARG_NONE, &list_modules,
			"Print a list of all available modules to stdout", NULL },
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &args, 0, 0, },
		{ NULL }
	};

	context = g_option_context_new("[CONFIG]");
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

	if (!file) {
		if (args)
			file = find_config_file(args[0]);
		else
			file = DEFAULT_CONFIG_FILE;
	}

	if (!g_file_get_contents(file, &config_buffer, NULL, &error))
		xsg_error("%s", error->message);

	xsg_var_init();

	parse_config(config_buffer);

	g_free(config_buffer);

	xsg_widgets_init();

	xsg_main_loop();

	return 0;;
}

