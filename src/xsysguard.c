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
#include <stdarg.h>

#include "modules.h"
#include "conf.h"
#include "main.h"
#include "var.h"
#include "dump.h"
#include "printf.h"
#include "window.h"
#include "widget_line.h"
#include "widget_rectangle.h"
#include "widget_ellipse.h"
#include "widget_polygon.h"
#include "widget_image.h"
#include "widget_barchart.h"
#include "widget_linechart.h"
#include "widget_areachart.h"
#include "widget_text.h"

/******************************************************************************/

#define COLOR_MAGENTA "\e[35m"
#define COLOR_YELLOW  "\e[33m"
#define COLOR_CYAN    "\e[36m"
#define COLOR_BLUE    "\e[34m"
#define COLOR_RED     "\e[31m"
#define COLOR_DEFAULT "\e[0m"

static bool colored_log = FALSE;

/******************************************************************************/

xsg_log_level_t xsg_log_level = XSG_LOG_LEVEL_WARNING;

static void xsg_logv(const char *domain, xsg_log_level_t level, const char *format, va_list args) {
	unsigned int pid;

	if (unlikely(level == XSG_LOG_LEVEL_ERROR))
		if (xsg_log_level < XSG_LOG_LEVEL_ERROR)
			exit(EXIT_FAILURE);

	pid = getpid();

	if (colored_log) {
		switch (level) {
			case XSG_LOG_LEVEL_ERROR:
				fprintf(stderr, COLOR_MAGENTA"[%u][ERR]", pid);
				break;
			case XSG_LOG_LEVEL_WARNING:
				fprintf(stderr, COLOR_YELLOW"[%u][WRN]", pid);
				break;
			case XSG_LOG_LEVEL_MESSAGE:
				fprintf(stderr, COLOR_CYAN"[%u][MSG]", pid);
				break;
			case XSG_LOG_LEVEL_DEBUG:
				fprintf(stderr, COLOR_BLUE"[%u][DBG]", pid);
				break;
			case XSG_LOG_LEVEL_MEM:
				fprintf(stderr, COLOR_RED"[%u][MEM]", pid);
				break;
			default:
				fprintf(stderr, "[%u][???]", pid);
				break;
		}
	} else {
		switch (level) {
			case XSG_LOG_LEVEL_ERROR:
				fprintf(stderr, "[ERR][%u]", pid);
				break;
			case XSG_LOG_LEVEL_WARNING:
				fprintf(stderr, "[WRN][%u]", pid);
				break;
			case XSG_LOG_LEVEL_MESSAGE:
				fprintf(stderr, "[MSG][%u]", pid);
				break;
			case XSG_LOG_LEVEL_DEBUG:
				fprintf(stderr, "[DBG][%u]", pid);
				break;
			case XSG_LOG_LEVEL_MEM:
				fprintf(stderr, "[MEM][%u]", pid);
				break;
			default:
				fprintf(stderr, "[???][%u]", pid);
				break;
		}
	}

	if (domain == NULL)
		fprintf(stderr, "xsysguard: ");
	else
		fprintf(stderr, "xsysguard/%s: ", domain);

	vfprintf(stderr, format, args);

	if (colored_log)
		fprintf(stderr, COLOR_DEFAULT"\n");
	else
		fprintf(stderr, "\n");

	if (unlikely(level == XSG_LOG_LEVEL_ERROR))
		exit(EXIT_FAILURE);
}

void xsg_log(const char *domain, xsg_log_level_t level, const char *format, ...) {
	va_list args;

	va_start(args, format);
	xsg_logv(domain, level, format, args);
	va_end(args);
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
	if (!xsg_conf_find_command("+")) {
		return FALSE;
	} else {
		*var_id = xsg_var_parse(widget_id, update);
		return TRUE;
	}
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
				xsg_window_parse_name();
			} else if (xsg_conf_find_command("Class")) {
				xsg_window_parse_class();
			} else if (xsg_conf_find_command("Resource")) {
				xsg_window_parse_resource();
			} else if (xsg_conf_find_command("Geometry")) {
				xsg_window_parse_geometry();
			} else if (xsg_conf_find_command("Sticky")) {
				xsg_window_parse_sticky();
			} else if (xsg_conf_find_command("SkipTaskbar")) {
				xsg_window_parse_skip_taskbar();
			} else if (xsg_conf_find_command("SkipPager")) {
				xsg_window_parse_skip_pager();
			} else if (xsg_conf_find_command("Layer")) {
				xsg_window_parse_layer();
			} else if (xsg_conf_find_command("Decorations")) {
				xsg_window_parse_decorations();
			} else if (xsg_conf_find_command("OverrideRedirect")) {
				xsg_window_parse_override_redirect();
			} else if (xsg_conf_find_command("Background")) {
				xsg_window_parse_background();
			} else if (xsg_conf_find_command("CacheSize")) {
				xsg_window_parse_cache_size();
			} else if (xsg_conf_find_command("FontCacheSize")) {
				xsg_window_parse_font_cache_size();
			} else if (xsg_conf_find_command("XShape")) {
				xsg_window_parse_xshape();
			} else if (xsg_conf_find_command("ARGBVisual")) {
				xsg_window_parse_argb_visual();
			} else if (xsg_conf_find_command("Show")) {
				xsg_window_parse_show();
			} else {
				xsg_conf_error("Interval, IconName, Class, Resource, Geometry, "
						"Sticky, SkipTaskbar, SkipPager, Layer, "
						"Decorations, OverrideRedirect, Background, CacheSize, XShape "
						"or ARGBVisual");
			}
		} else if (xsg_conf_find_command("SetEnv")) {
			parse_env();
		} else if (xsg_conf_find_command("Line")) {
			xsg_widget_line_parse();
		} else if (xsg_conf_find_command("Rectangle")) {
			xsg_widget_rectangle_parse();
		} else if (xsg_conf_find_command("Ellipse")) {
			xsg_widget_ellipse_parse();
		} else if (xsg_conf_find_command("Polygon")) {
			xsg_widget_polygon_parse();
		} else if (xsg_conf_find_command("Image")) {
			xsg_widget_image_parse();
		} else if (xsg_conf_find_command("BarChart")) {
			xsg_widget_barchart_parse(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widget_barchart_parse_var(var_id);
		} else if (xsg_conf_find_command("LineChart")) {
			xsg_widget_linechart_parse(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widget_linechart_parse_var(var_id);
		} else if (xsg_conf_find_command("AreaChart")) {
			xsg_widget_areachart_parse(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widget_areachart_parse_var(var_id);
		} else if (xsg_conf_find_command("Text")) {
			xsg_widget_text_parse(&update, &widget_id);
			while (parse_var(widget_id, update, &var_id) != 0)
				xsg_widget_text_parse_var(var_id);
		} else {
			xsg_conf_error("#, Set, SetEnv, Line, Rectangle, Ellipse, Polygon, "
					"Image, BarChart, LineChart, AreaChart or Text");
		}
	}
}

static char *find_config_file(char *name) {
	char *file = NULL;
	char **pathv;
	char **p;

	if (unlikely(name == NULL))
		name = "default";

	if (unlikely(strlen(name) < 1))
		name = "default";

	pathv = xsg_get_path_from_env("XSYSGUARD_CONFIG_PATH", XSYSGUARD_CONFIG_PATH);

	if (unlikely(pathv == NULL))
		xsg_error("Cannot get XSYSGUARD_CONFIG_PATH");

	xsg_message("Searching for config...");
	for (p = pathv; *p; p++) {
		file = xsg_build_filename(*p, name, NULL);
		if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR)) {
			xsg_strfreev(pathv);
			xsg_message("Found config file \"%s\"", file);
			return file;
		}
		xsg_free(file);
	}

	xsg_strfreev(pathv);
	xsg_error("Cannot find file: \"%s\"", name);
	return 0;
}

static char *get_config_file(const char *filename) {
	struct stat stat_buf;
	char *buffer;
	size_t size, bytes_read;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0)
		xsg_error("Cannot read config file: %s: %s", filename, strerror(errno));

	if (fstat(fd, &stat_buf) < 0)
		xsg_error("Cannot read config file: %s: fstat failed", filename);

	size = stat_buf.st_size;

	if (size <= 0)
		xsg_error("Cannot read config file: %s: null byte size", filename);

	if (!S_ISREG(stat_buf.st_mode))
		xsg_error("Cannot read config file: %s: not a regular file", filename);

	buffer = xsg_malloc(size + 1);

	bytes_read = 0;
	while (bytes_read < size) {
		ssize_t rc;

		rc = read(fd, buffer + bytes_read, size - bytes_read);
		if (rc < 0) {
			if (errno != EINTR)
				xsg_error("Cannot read config file: %s: %s", filename, strerror(errno));
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
	char **pathv;
	char **p;

	printf( "xsysguard " VERSION " Copyright 2005-2007 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"Usage: xsysguard [ARGUMENTS...] [CONFIG]\n\n"
		"Arguments:\n"
		"  -h, --help       Print this help message to stdout\n"
		"  -m, --modules    Print a list of all available modules to stdout\n"
		"  -f, --file=FILE  Read configuration from FILE\n"
		"  -c, --color      Enable colored logging\n"
		"  -l, --log=N      Set loglevel to N: "
		"%d=ERROR, %d=WARNING, %d=MESSAGE, %d=DEBUG\n",
			XSG_LOG_LEVEL_ERROR, XSG_LOG_LEVEL_WARNING, XSG_LOG_LEVEL_MESSAGE, XSG_LOG_LEVEL_DEBUG);
	printf("\n\n");

	printf("Config path:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_CONFIG_PATH", XSYSGUARD_CONFIG_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("Module path:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_MODULE_PATH", XSYSGUARD_MODULE_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("Image path:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_IMAGE_PATH", XSYSGUARD_IMAGE_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("Font path:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_FONT_PATH", XSYSGUARD_FONT_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");
}

/******************************************************************************
 *
 * main
 *
 ******************************************************************************/

int main(int argc, char **argv) {
	int log = xsg_log_level;
	char *filename = NULL;
	bool list_modules = FALSE;
	bool print_usage = FALSE;
	char *config_buffer = NULL;

	struct option long_options[] = {
		{ "help",    0, NULL, 'h' },
		{ "log",     1, NULL, 'l' },
		{ "color",   0, NULL, 'c' },
		{ "file",    1, NULL, 'f' },
		{ "modules", 0, NULL, 'm' },
		{ NULL,      0, NULL,  0  }
	};

	opterr = 0;
	while (1) {
		int option, option_index = 0;

		option = getopt_long(argc, argv, "hl:f:mc", long_options, &option_index);

		if (option == EOF)
			break;

		switch (option) {
			case 'h':
				print_usage = TRUE;
				break;
			case 'f':
				if (optarg)
					filename = xsg_strdup(optarg);
				break;
			case 'l':
				if (optarg)
					log = atoi(optarg);
				break;
			case 'c':
				colored_log = TRUE;
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

	xsg_log_level = log;

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
			filename = find_config_file("default");
	}

	config_buffer = get_config_file(filename);
	parse_config(config_buffer);
	xsg_free(config_buffer);

	xsg_var_init();
	xsg_printf_init();
	xsg_window_init();

	xsg_dump_atexit();

	xsg_main_loop();

	return EXIT_SUCCESS;
}

