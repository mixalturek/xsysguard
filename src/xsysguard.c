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
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>

#include "modules.h"
#include "widgets.h"
#include "conf.h"
#include "main.h"
#include "var.h"
#include "printf.h"

/******************************************************************************/

#ifndef DEFAULT_CONFIG_FILE
#define DEFAULT_CONFIG_FILE xsg_build_filename(xsg_get_home_dir(), ".xsysguard", "configs", "default", NULL)
#endif

#ifndef HOME_CONFIG_DIR
#define HOME_CONFIG_DIR xsg_build_filename(xsg_get_home_dir(), ".xsysguard", "configs", NULL)
#endif

#ifndef CONFIG_DIR
#define CONFIG_DIR "/usr/share/xsysguard/configs"
#endif

/******************************************************************************/

static uint32_t log_level = XSG_LOG_LEVEL_ERROR;

void xsg_log(const char *domain, uint32_t level, const char *format, va_list args) {
	if (log_level < level)
		return;

	if (level == XSG_LOG_LEVEL_ERROR)
		fprintf(stderr, "ERROR: ");
	else if (level == XSG_LOG_LEVEL_WARNING)
		fprintf(stderr, "WARNING: ");
	else if (level == XSG_LOG_LEVEL_MESSAGE)
		fprintf(stderr, "MESSAGE: ");
	else if (level == XSG_LOG_LEVEL_DEBUG)
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

	if (!setenv(variable, value, overwrite))
		xsg_warning("Cannot set environment variable %s = %s", variable, value);
}

static bool parse_var(uint32_t widget_id, uint64_t update, uint32_t *var_id) {
	if (xsg_conf_find_command("+"))
		*var_id = xsg_var_parse_double(widget_id, update);
	else if (xsg_conf_find_command("."))
		*var_id = xsg_var_parse_string(widget_id, update);
	else
		return FALSE;
	return TRUE;
}

static void parse_config(char *config_buffer) {
	uint64_t update;
	uint32_t var_id;
	uint32_t widget_id;

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
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widgets_parse_barchart_var(var_id);
		} else if (xsg_conf_find_command("LineChart")) {
			xsg_widgets_parse_linechart(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widgets_parse_linechart_var(var_id);
		} else if (xsg_conf_find_command("AreaChart")) {
			xsg_widgets_parse_areachart(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widgets_parse_areachart_var(var_id);
		} else if (xsg_conf_find_command("Text")) {
			xsg_widgets_parse_text(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widgets_parse_text_var(var_id);
		} else if (xsg_conf_find_command("ImageText")) {
			xsg_widgets_parse_imagetext(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widgets_parse_imagetext_var(var_id);
		} else {
			xsg_conf_error("#, Set, SetEnv, Line, Rectangle, Ellipse, Polygon, "
					"Image, BarChart, LineChart, AreaChart, Text or ImageText");
		}
	}
}

static char *find_config_file(char *name) {
	char *file = NULL;

	file = xsg_build_filename(HOME_CONFIG_DIR, name, NULL);
	if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR))
		return file;

	file = xsg_build_filename(CONFIG_DIR, name, NULL);
	if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR))
		return file;

	xsg_error("Cannot find file \"%s\"", name);
	return 0;
}

static char *get_config_file(const char *filename) {
	struct stat stat_buf;
	char *buffer;
	size_t size, bytes_read;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0)
		xsg_error("cannot read config file: %s: %s", filename, strerror(errno));

	if (fstat(fd, &stat_buf) < 0)
		xsg_error("cannot read config file: %s: fstat failed", filename);

	size = stat_buf.st_size;

	if (size <= 0)
		xsg_error("cannot red config file: %s: null byte size", filename);

	if (!S_ISREG(stat_buf.st_mode))
		xsg_error("cannot read config file: %s: not a regular file", filename);

	buffer = xsg_malloc(size + 1);

	bytes_read = 0;
	while (bytes_read < size) {
		ssize_t rc;

		rc = read(fd, buffer + bytes_read, size - bytes_read);
		if (rc < 0) {
			if (errno != EINTR)
				xsg_error("cannot read config file: %s: %s", filename, strerror(errno));
		} else if (rc == 0) {
			break;
		} else {
			bytes_read += rc;
		}
	}

	buffer[bytes_read] = '\0';

	close(fd);

	return buffer;
}

static void usage(void) {
	printf( "xsysguard " VERSION " Copyright 2005-2007 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"Usage: xsysguard [ARGUMENTS...] [CONFIG]\n\n"
		"Arguments:\n"
		"  -h, --help          Show help options\n"
		"  -l, --log=N         Set loglevel to N = 0 ... 5 (debug)\n"
		"  -f, --file=FILE     Read configuration from FILE\n"
		"  -m, --modules       Print a list of all available modules\n\n");
}

/******************************************************************************
 *
 * main
 *
 ******************************************************************************/

int main(int argc, char **argv) {
	int log = 0;
	char *filename = NULL;
	bool list_modules = FALSE;
	bool print_usage = FALSE;
	char *config_buffer = NULL;

	struct option long_options[] = {
		{ "help",    0, NULL, 'h' },
		{ "log",     1, NULL, 'l' },
		{ "file",    1, NULL, 'f' },
		{ "modules", 0, NULL, 'm' },
		{ NULL,      0, NULL,  0  }
	};

	opterr = 0;
	while (1) {
		int option, option_index = 0;

		option = getopt_long(argc, argv, "hl:f:m", long_options, &option_index);

		if (option == EOF)
			break;

		switch (option) {
			case 'h':
				print_usage = TRUE;
				break;
			case 'f':
				if (optarg)
					filename = strdup(optarg);
				break;
			case 'l':
				if (optarg)
					log = atoi(optarg);
				break;
			case 'm':
				list_modules = TRUE;
				break;
			case '?':
				print_usage = TRUE;
				break;
			default:
				break;
		}
	}

	log_level = log;

	if (print_usage) {
		usage();
		exit(EXIT_SUCCESS);
	}

	if (list_modules) {
		xsg_modules_list();
		exit(EXIT_SUCCESS);
	}

	if (!filename) {
		if (optind < argc)
			filename = find_config_file(argv[optind]);
		else
			filename = DEFAULT_CONFIG_FILE;
	}

	config_buffer = get_config_file(filename);
	parse_config(config_buffer);
	xsg_free(config_buffer);

	xsg_var_init();
	xsg_printf_init();
	xsg_widgets_init();

	xsg_main_loop();

	return 0;
}

