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
#include <time.h>

#include "modules.h"
#include "conf.h"
#include "main.h"
#include "var.h"
#include "rpn.h"
#include "dump.h"
#include "imlib.h"
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

static bool timestamps = FALSE;

/******************************************************************************/

xsg_log_level_t xsg_log_level = XSG_LOG_LEVEL_WARNING;

static void xsg_logv(const char *domain, xsg_log_level_t level, const char *format, va_list args) {
	unsigned int pid;
	char *prefix = NULL;

	if (unlikely(level == XSG_LOG_LEVEL_ERROR))
		if (xsg_log_level < XSG_LOG_LEVEL_ERROR)
			exit(EXIT_FAILURE);

	if (timestamps) {
		struct timeval tv;
		struct tm *tm;
		char buf[64];

		xsg_gettimeofday(&tv, NULL);

		tm = localtime(&tv.tv_sec);

		if (localtime != NULL)
			if (strftime(buf, sizeof(buf) - 1, "%H:%M:%S", tm) != 0)
				fprintf(stderr, "%s.%06u ", buf, (unsigned) tv.tv_usec);
	}

	if (colored_log) {
		switch (level) {
			case XSG_LOG_LEVEL_ERROR:
				prefix = COLOR_MAGENTA"[ERR]";
				break;
			case XSG_LOG_LEVEL_WARNING:
				prefix = COLOR_YELLOW"[WRN]";
				break;
			case XSG_LOG_LEVEL_MESSAGE:
				prefix = COLOR_CYAN"[MSG]";
				break;
			case XSG_LOG_LEVEL_DEBUG:
				prefix = COLOR_BLUE"[DBG]";
				break;
			case XSG_LOG_LEVEL_MEM:
				prefix = COLOR_RED"[MEM]";
				break;
			default:
				prefix = "[???]";
				break;
		}
	} else {
		switch (level) {
			case XSG_LOG_LEVEL_ERROR:
				prefix = "[ERR]";
				break;
			case XSG_LOG_LEVEL_WARNING:
				prefix = "[WRN]";
				break;
			case XSG_LOG_LEVEL_MESSAGE:
				prefix = "[MSG]";
				break;
			case XSG_LOG_LEVEL_DEBUG:
				prefix = "[DBG]";
				break;
			case XSG_LOG_LEVEL_MEM:
				prefix = "[MEM]";
				break;
			default:
				prefix = "[???]";
				break;
		}
	}

	pid = getpid();

	if (domain == NULL)
		fprintf(stderr, "%s[%u]xsysguard: ", prefix, pid);
	else
		fprintf(stderr, "%s[%u]xsysguard/%s: ", prefix, pid, domain);

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

static void parse_env(const char *config_name) {
	char *variable;
	char *value;
	bool overwrite = FALSE;

	variable = xsg_conf_read_string();
	value = xsg_conf_read_string();
	overwrite = xsg_conf_find_command("Overwrite");

	xsg_message("%s: Setting environment variable %s=\"%s\"", config_name, variable, value);

	if (setenv(variable, value, overwrite) != 0)
		xsg_warning("%s: Cannot set environment variable %s=\"%s\": %s", config_name, variable, value, strerror(errno));
}

static bool parse_var(xsg_window_t *window, xsg_widget_t *widget, uint64_t update, xsg_var_t **var) {
	if (!xsg_conf_find_command("+")) {
		return FALSE;
	} else {
		*var = xsg_var_parse(window, widget, update);
		return TRUE;
	}
}

static void parse_config(char *config_name, char *config_buffer) {
	xsg_window_t *window;
	xsg_widget_t *widget;
	xsg_var_t *var;
	uint64_t update;

	xsg_conf_set_buffer(config_name, config_buffer);

	window = xsg_window_new(config_name);

	while (!xsg_conf_find_end()) {
		if (xsg_conf_find_newline())
			continue;
		if (xsg_conf_find_commentline())
			continue;
		if (xsg_conf_find_command("Set")) {
			if (xsg_conf_find_command("Name")) {
				xsg_window_parse_name(window);
			} else if (xsg_conf_find_command("Class")) {
				xsg_window_parse_class(window);
			} else if (xsg_conf_find_command("Resource")) {
				xsg_window_parse_resource(window);
			} else if (xsg_conf_find_command("Geometry")) {
				xsg_window_parse_geometry(window);
			} else if (xsg_conf_find_command("Sticky")) {
				xsg_window_parse_sticky(window);
			} else if (xsg_conf_find_command("SkipTaskbar")) {
				xsg_window_parse_skip_taskbar(window);
			} else if (xsg_conf_find_command("SkipPager")) {
				xsg_window_parse_skip_pager(window);
			} else if (xsg_conf_find_command("Layer")) {
				xsg_window_parse_layer(window);
			} else if (xsg_conf_find_command("Decorations")) {
				xsg_window_parse_decorations(window);
			} else if (xsg_conf_find_command("OverrideRedirect")) {
				xsg_window_parse_override_redirect(window);
			} else if (xsg_conf_find_command("Background")) {
				xsg_window_parse_background(window);
			} else if (xsg_conf_find_command("XShape")) {
				xsg_window_parse_xshape(window);
			} else if (xsg_conf_find_command("ARGBVisual")) {
				xsg_window_parse_argb_visual(window);
			} else if (xsg_conf_find_command("Visible")) {
				xsg_window_parse_visible(window);
			} else {
				xsg_conf_error("Name, Class, Resource, Geometry, "
						"Sticky, SkipTaskbar, SkipPager, Layer, "
						"Decorations, OverrideRedirect, Background, XShape, "
						"ARGBVisual or Visible");
			}
		} else if (xsg_conf_find_command("SetEnv")) {
			parse_env(config_name);
		} else if (xsg_conf_find_command("Line")) {
			xsg_widget_line_parse(window);
		} else if (xsg_conf_find_command("Rectangle")) {
			xsg_widget_rectangle_parse(window);
		} else if (xsg_conf_find_command("Ellipse")) {
			xsg_widget_ellipse_parse(window);
		} else if (xsg_conf_find_command("Polygon")) {
			xsg_widget_polygon_parse(window);
		} else if (xsg_conf_find_command("Image")) {
			xsg_widget_image_parse(window);
		} else if (xsg_conf_find_command("BarChart")) {
			widget = xsg_widget_barchart_parse(window, &update);
			while (parse_var(window, widget, update, &var) != 0)
				xsg_widget_barchart_parse_var(var);
		} else if (xsg_conf_find_command("LineChart")) {
			widget = xsg_widget_linechart_parse(window, &update);
			while (parse_var(window, widget, update, &var) != 0)
				xsg_widget_linechart_parse_var(var);
		} else if (xsg_conf_find_command("AreaChart")) {
			widget = xsg_widget_areachart_parse(window, &update);
			while (parse_var(window, widget, update, &var) != 0)
				xsg_widget_areachart_parse_var(var);
		} else if (xsg_conf_find_command("Text")) {
			widget = xsg_widget_text_parse(window, &update);
			while (parse_var(window, widget, update, &var) != 0)
				xsg_widget_text_parse_var(var);
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

	xsg_debug("Searching for config...");
	for (p = pathv; *p; p++) {
		file = xsg_build_filename(*p, name, NULL);
		if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR)) {
			xsg_strfreev(pathv);
			xsg_message("%s: Found config file \"%s\"", name, file);
			return file;
		}
		xsg_free(file);
	}

	xsg_strfreev(pathv);
	xsg_error("Cannot find config file: \"%s\"", name);
	return 0;
}

static char *get_config_file(const char *config_name, const char *filename) {
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

	if (config_name != NULL)
		xsg_message("%s: Reading config file \"%s\"", config_name, filename);
	else
		xsg_message("Reading config file \"%s\"", filename);

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
		"  -h, --help         Print this help message to stdout\n"
		"  -m, --modules      Print a list of all available modules to stdout\n"
		"  -f, --file=FILE    Read configuration from FILE\n"
		"  -i, --interval=I   Set main interval to I milliseconds\n"
		"  -F, --fontcache=B  Set imlib font cache size to B bytes\n"
		"  -I, --imgcache=B   Set imlib image cache size to B bytes\n"
		"  -c, --color        Enable colored logging\n"
		"  -t, --time         Add current time to each log line\n"
		"  -l, --log=N        Set loglevel to N: "
		"%d=ERROR, %d=WARNING, %d=MESSAGE, %d=DEBUG\n",
			XSG_LOG_LEVEL_ERROR, XSG_LOG_LEVEL_WARNING, XSG_LOG_LEVEL_MESSAGE, XSG_LOG_LEVEL_DEBUG);
	printf("\n\n");

	printf("XSYSGUARD_CONFIG_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_CONFIG_PATH", XSYSGUARD_CONFIG_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_MODULE_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_MODULE_PATH", XSYSGUARD_MODULE_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_IMAGE_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_IMAGE_PATH", XSYSGUARD_IMAGE_PATH);
	if (pathv != NULL)
		for (p = pathv; *p; p++)
			printf("  %s\n", *p);
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_FONT_PATH:\n");
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
	bool list_modules = FALSE;
	bool print_usage = FALSE;
	uint64_t interval = 1000;
	xsg_list_t *filename_list = NULL;
	xsg_list_t *l;

	struct option long_options[] = {
		{ "help",      0, NULL, 'h' },
		{ "interval",  1, NULL, 'i' },
		{ "fontcache", 1, NULL, 'F' },
		{ "imgcache",  1, NULL, 'I' },
		{ "log",       1, NULL, 'l' },
		{ "color",     0, NULL, 'c' },
		{ "time",      0, NULL, 't' },
		{ "file",      1, NULL, 'f' },
		{ "modules",   0, NULL, 'm' },
		{ NULL,        0, NULL,  0  }
	};

	opterr = 0;
	while (1) {
		int option, option_index = 0;

		option = getopt_long(argc, argv, "hi:F:I:l:f:mct", long_options, &option_index);

		if (option == EOF)
			break;

		switch (option) {
			case 'h':
				print_usage = TRUE;
				break;
			case 'i':
				sscanf(optarg, "%"SCNu64, &interval);
				xsg_main_set_interval(interval);
				break;
			case 'F':
				if (optarg)
					xsg_imlib_set_font_cache_size(atoi(optarg));
				break;
			case 'I':
				if (optarg)
					xsg_imlib_set_cache_size(atoi(optarg));
				break;
			case 'f':
				if (optarg)
					filename_list = xsg_list_append(filename_list, xsg_strdup(optarg));
				break;
			case 'l':
				if (optarg)
					xsg_log_level = atoi(optarg);
				break;
			case 'c':
				colored_log = TRUE;
				break;
			case 't':
				timestamps = TRUE;
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

	if (print_usage) {
		usage();
		exit(EXIT_SUCCESS);
	}

	xsg_modules_init();

	if (list_modules) {
		xsg_modules_list();
		exit(EXIT_SUCCESS);
	}

	xsg_imlib_init();

	for (l = filename_list; l; l = l->next) {
		char *filename = l->data;
		char *config_buffer;

		config_buffer = get_config_file(NULL, filename);
		parse_config(filename, config_buffer);
		xsg_free(config_buffer);
	}

	if ((filename_list == NULL) && (optind >= argc)) {
		char *filename, *config_buffer;

		filename = find_config_file("default");
		config_buffer = get_config_file("default", filename);
		parse_config("default", config_buffer);
		xsg_free(config_buffer);
	}

	while (optind < argc) {
		char *filename, *config_buffer;

		filename = find_config_file(argv[optind]);
		config_buffer = get_config_file(argv[optind], filename);
		parse_config(argv[optind], config_buffer);
		xsg_free(config_buffer);
		optind++;
	}

	xsg_rpn_init();
	xsg_window_init();

	xsg_dump_atexit();

	xsg_main_loop();

	return EXIT_SUCCESS;
}

