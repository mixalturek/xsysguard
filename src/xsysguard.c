/* xsysguard.c
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
#include <string.h>

#include "modules.h"
#include "conf.h"
#include "main.h"
#include "var.h"
#include "imlib.h"
#include "printf.h"
#include "window.h"
#include "widgets.h"
#include "widget_line.h"
#include "widget_rectangle.h"
#include "widget_ellipse.h"
#include "widget_polygon.h"
#include "widget_image.h"
#include "widget_barchart.h"
#include "widget_linechart.h"
#include "widget_areachart.h"
#include "widget_text.h"
#include "fontconfig.h"

/******************************************************************************/

#define DEFAULT_INTERVAL		((uint64_t)(1000))

#define DEFAULT_FONT_CACHE_SIZE		(2 * 1024 * 1024)
#define DEFAULT_IMAGE_CACHE_SIZE	(4 * 1024 * 1024)

#define DEFAULT_LOG_LEVEL		XSG_LOG_LEVEL_WARNING

/******************************************************************************/

#define ESCAPE "\033"
#define COLOR_MAGENTA ESCAPE"[35m"
#define COLOR_YELLOW  ESCAPE"[33m"
#define COLOR_CYAN    ESCAPE"[36m"
#define COLOR_BLUE    ESCAPE"[34m"
#define COLOR_RED     ESCAPE"[31m"
#define COLOR_DEFAULT ESCAPE"[0m"

static bool colored_log = FALSE;

static bool timestamps = FALSE;

/******************************************************************************/

int xsg_log_level = DEFAULT_LOG_LEVEL;

static void
xsg_logv(const char *domain, int level, const char *format, va_list args)
{
	unsigned int pid;
	const char *prefix = NULL;

	if (unlikely(level == XSG_LOG_LEVEL_ERROR)) {
		if (xsg_log_level < XSG_LOG_LEVEL_ERROR) {
			exit(EXIT_FAILURE);
		}
	}

	if (timestamps) {
		struct timeval tv;
		struct tm *tm;
		char buf[64];

		xsg_gettimeofday(&tv, NULL);

		tm = localtime(&tv.tv_sec);

		if (localtime != NULL) {
			if (strftime(buf, sizeof(buf) - 1, "%H:%M:%S", tm) != 0) {
				fprintf(stderr, "%s.%06u ", buf,
						(unsigned) tv.tv_usec);
			}
		}
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

	if (domain == NULL) {
		fprintf(stderr, "%s[%u]xsysguard: ", prefix, pid);
	} else {
		fprintf(stderr, "%s[%u]xsysguard/%s: ", prefix, pid, domain);
	}

	vfprintf(stderr, format, args);

	if (colored_log) {
		fprintf(stderr, COLOR_DEFAULT"\n");
	} else {
		fprintf(stderr, "\n");
	}

	if (unlikely(level == XSG_LOG_LEVEL_ERROR)) {
		exit(EXIT_FAILURE);
	}
}

void
xsg_log(const char *domain, int level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	xsg_logv(domain, level, format, args);
	va_end(args);
}

/******************************************************************************/

static void
parse_env(const char *config_name)
{
	char *variable;
	char *value;
	bool overwrite = FALSE;

	variable = xsg_conf_read_string();
	value = xsg_conf_read_string();
	overwrite = xsg_conf_find_command("Overwrite");

	xsg_message("%s: setting environment variable %s=%s",
			config_name, variable, value);

	if (xsg_setenv(variable, value, overwrite) != 0) {
		xsg_warning("%s: cannot set environment variable %s=%s: %s",
				config_name, variable, value, strerror(errno));
	}

	xsg_free(variable);
	xsg_free(value);
}

static void
parse_config(
	char *config_name,
	char *config_buffer,
	int flags,
	int xoffset,
	int yoffset)
{
	xsg_window_t *window;

	xsg_conf_set_buffer(config_name, config_buffer);

	window = xsg_window_new(config_name, flags, xoffset, yoffset);

	while (!xsg_conf_find_end()) {
		if (xsg_conf_find_newline()) {
			continue;
		}
		if (xsg_conf_find_commentline()) {
			continue;
		}
		if (xsg_conf_find_command("Set")) {
			if (xsg_conf_find_command("Name")) {
				xsg_window_parse_name(window);
			} else if (xsg_conf_find_command("Class")) {
				xsg_window_parse_class(window);
			} else if (xsg_conf_find_command("Resource")) {
				xsg_window_parse_resource(window);
			} else if (xsg_conf_find_command("Size")) {
				xsg_window_parse_size(window);
			} else if (xsg_conf_find_command("Position")) {
				xsg_window_parse_position(window);
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
			} else if (xsg_conf_find_command("Mouse")) {
				xsg_window_parse_mouse(window);
			} else {
				xsg_conf_error("Name, Class, Resource, "
						"Size, Position, Sticky, "
						"SkipTaskbar, SkipPager, "
						"Layer, Decorations, "
						"OverrideRedirect, Background, "
						"XShape, ARGBVisual or "
						"Visible expected");
			}
		} else if (xsg_conf_find_command("ModuleEnv")) {
			char *module_name = xsg_conf_read_string();
			xsg_modules_help(module_name);
			xsg_free(module_name);
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
			xsg_widget_barchart_parse(window);
		} else if (xsg_conf_find_command("LineChart")) {
			xsg_widget_linechart_parse(window);
		} else if (xsg_conf_find_command("AreaChart")) {
			xsg_widget_areachart_parse(window);
		} else if (xsg_conf_find_command("Text")) {
			xsg_widget_text_parse(window);
		} else {
			xsg_conf_error("#, Set, SetEnv, ModuleEnv, Line, "
					"Rectangle, Ellipse, Polygon, "
					"Image, BarChart, LineChart, "
					"AreaChart or Text expected");
		}
	}
}

static char *
find_config_file(char *name)
{
	char *file = NULL;
	char **pathv;
	char **p;

	if (unlikely(name == NULL)) {
		xsg_error("cannot find config file: NULL");
	}

	if (unlikely(strlen(name) < 1)) {
		xsg_error("cannot find config file: config name len is 0");
	}

	if (name[0] == '/') {
		return xsg_strdup(name);
	}

	if (name[0] == '.' && name[1] == '/') {
		return xsg_strdup(name);
	}

	if (name[0] == '.' && name[1] == '.' && name[2] == '/') {
		return xsg_strdup(name);
	}

	if (name[0] == '~' && name[1] == '/') {
		return xsg_build_filename(xsg_get_home_dir(), name + 1, NULL);
	}

	pathv = xsg_get_path_from_env("XSYSGUARD_CONFIG_PATH",
			XSYSGUARD_CONFIG_PATH);

	if (unlikely(pathv == NULL)) {
		xsg_error("Cannot get XSYSGUARD_CONFIG_PATH");
	}

	xsg_debug("searching for config...");
	for (p = pathv; *p; p++) {
		file = xsg_build_filename(*p, name, NULL);
		if (xsg_file_test(file, XSG_FILE_TEST_IS_REGULAR)) {
			xsg_strfreev(pathv);
			xsg_message("%s: found config file: %s", name, file);
			return file;
		}
		xsg_free(file);
	}

	xsg_strfreev(pathv);
	xsg_error("cannot find config file: %s", name);
	return NULL;
}

static char *
get_config_file(const char *config_name, const char *filename)
{
	struct stat stat_buf;
	char *buffer;
	size_t size, bytes_read;
	int fd;

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		xsg_error("cannot read config file: %s: %s", filename,
				strerror(errno));
	}

	xsg_set_cloexec_flag(fd, TRUE);

	if (fstat(fd, &stat_buf) < 0) {
		xsg_error("cannot read config file: %s: fstat failed",
				filename);
	}

	size = stat_buf.st_size;

	if (size <= 0) {
		xsg_error("cannot read config file: %s: null byte size",
				filename);
	}

	if (!S_ISREG(stat_buf.st_mode)) {
		xsg_error("cannot read config file: %s: not a regular file",
				filename);
	}

	buffer = xsg_malloc(size + 1);

	if (config_name != NULL) {
		xsg_message("%s: reading config file: %s", config_name,
				filename);
	} else {
		xsg_message("reading config file: %s", filename);
	}

	bytes_read = 0;
	while (bytes_read < size) {
		ssize_t rc;

		rc = read(fd, buffer + bytes_read, size - bytes_read);
		if (rc < 0) {
			if (errno != EINTR) {
				xsg_error("Cannot read config file: %s: %s",
						filename, strerror(errno));
			}
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

static void
usage(bool enable_fontconfig)
{
	char **pathv;
	char **p;

	printf( "xsysguard " XSYSGUARD_VERSION " Copyright 2005-2008 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"Usage: xsysguard [OPTIONS...] CONFIG[{+-}<X>{+-}<Y>] [NAME=VALUE] [CONFIG] ...\n\n"
		"Options:\n"
		"  -h, --help         Print this help message to stdout\n"
		"  -H, --mhelp=MODULE Print help message for MODULE to stdout\n"
		"  -L, --license      Print license to stdout\n"
		"  -m, --modules      Print a list of all available modules to stdout\n"
		"  -f, --fonts        Print a list of all available fonts to stdout\n"
		"  -d, --fontdirs     Print a list of all font dirs to stdout (libfontconfig)\n"
		"  -i, --interval=N   Set main interval to N milliseconds (default: %"PRIu64")\n"
		"  -n, --num=N        Exit after N tick's\n"
		"  -N, --nofontconfig Disable libfontconfig\n"
		"  -F, --fontcache=N  Set Imlib2's font cache size to N bytes (default: %d)\n"
		"  -I, --imgcache=N   Set Imlib2's image cache size to N bytes (default: %d)\n"
		"  -c, --color        Enable colored logging\n"
		"  -t, --time         Add current time to each log line\n"
		"  -l, --log=N        Set loglevel to N: ",
		DEFAULT_INTERVAL, DEFAULT_FONT_CACHE_SIZE, DEFAULT_IMAGE_CACHE_SIZE);

	if (XSG_LOG_LEVEL_ERROR <= XSG_LOG_LEVEL_MAX) {
		printf("%d=ERROR", XSG_LOG_LEVEL_ERROR);
	}
	if (XSG_LOG_LEVEL_WARNING <= XSG_LOG_LEVEL_MAX) {
		printf(", %d=WARNING", XSG_LOG_LEVEL_WARNING);
	}
	if (XSG_LOG_LEVEL_MESSAGE <= XSG_LOG_LEVEL_MAX) {
		printf(", %d=MESSAGE", XSG_LOG_LEVEL_MESSAGE);
	}
	if (XSG_LOG_LEVEL_DEBUG <= XSG_LOG_LEVEL_MAX) {
		printf(", %d=DEBUG", XSG_LOG_LEVEL_DEBUG);
	}
	if (XSG_LOG_LEVEL_MEM <= XSG_LOG_LEVEL_MAX) {
		printf(", %d=MEM", XSG_LOG_LEVEL_MEM);
	}

	printf("\n\n");

	printf("XSYSGUARD_CONFIG_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_CONFIG_PATH",
			XSYSGUARD_CONFIG_PATH);
	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			printf("  %s\n", *p);
		}
	}
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_MODULE_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_MODULE_PATH",
			XSYSGUARD_MODULE_PATH);
	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			printf("  %s\n", *p);
		}
	}
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_IMAGE_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_IMAGE_PATH",
			XSYSGUARD_IMAGE_PATH);
	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			printf("  %s\n", *p);
		}
	}
	xsg_strfreev(pathv);
	printf("\n");

	printf("XSYSGUARD_FONT_PATH:\n");
	pathv = xsg_get_path_from_env("XSYSGUARD_FONT_PATH",
			XSYSGUARD_FONT_PATH);
	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			printf("  %s\n", *p);
		}
	}
	xsg_strfreev(pathv);
	printf("\n");

	if (!enable_fontconfig) {
		return;
	}

	pathv = xsg_fontconfig_get_path();
	if (pathv != NULL) {
		unsigned n = 0;
		for (p = pathv; *p; p++) {
			n++;
		}
		xsg_strfreev(pathv);
		printf("Found libfontconfig (%u font dirs). Print list with "
				"`xsysguard -d`.\n", n);
	}
	printf("\n");
}

static void
license(void)
{
	printf( "xsysguard " XSYSGUARD_VERSION " Copyright 2005-2008 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"  This program is free software; you can redistribute it and/or modify\n"
		"  it under the terms of the GNU General Public License as published by\n"
		"  the Free Software Foundation; either version 2 of the License, or\n"
		"  (at your option) any later version.\n\n"
		"  This program is distributed in the hope that it will be useful,\n"
		"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"  GNU General Public License for more details.\n\n");
}

static void
list_font_dirs(void)
{
	char **pathv;
	char **p;

	pathv = xsg_fontconfig_get_path();
	if (pathv != NULL) {
		for (p = pathv; *p; p++) {
			printf("%s\n", *p);
		}
		xsg_strfreev(pathv);
	}
}

/******************************************************************************
 *
 * main
 *
 ******************************************************************************/

int
main(int argc, char **argv)
{
	bool list_modules = FALSE;
	bool list_fonts = FALSE;
	bool list_dirs = FALSE;
	char *mhelp = NULL;
	bool print_usage = FALSE;
	bool print_license = FALSE;
	uint64_t interval = DEFAULT_INTERVAL;
	uint64_t num = 0;
	int font_cache_size = DEFAULT_FONT_CACHE_SIZE;
	int image_cache_size = DEFAULT_IMAGE_CACHE_SIZE;
	bool enable_fontconfig = TRUE;

	struct option long_options[] = {
		{ "help",         0, NULL, 'h' },
		{ "mhelp",        1, NULL, 'H' },
		{ "license",      0, NULL, 'L' },
		{ "interval",     1, NULL, 'i' },
		{ "num",          1, NULL, 'n' },
		{ "nofontconfig", 0, NULL, 'N' },
		{ "fontcache",    1, NULL, 'F' },
		{ "imgcache",     1, NULL, 'I' },
		{ "log",          1, NULL, 'l' },
		{ "color",        0, NULL, 'c' },
		{ "time",         0, NULL, 't' },
		{ "modules",      0, NULL, 'm' },
		{ "fonts",        0, NULL, 'f' },
		{ "fontdirs",     0, NULL, 'd' },
		{ NULL,           0, NULL,  0  }
	};

	opterr = 0;
	while (1) {
		int option, option_index = 0;

		option = getopt_long(argc, argv, "hH:Li:n:NF:I:l:mfdct",
				long_options, &option_index);

		if (option == EOF) {
			break;
		}

		switch (option) {
		case 'h':
			print_usage = TRUE;
			break;
		case 'H':
			if (optarg && !mhelp) {
				mhelp = xsg_strdup(optarg);
			}
			break;
		case 'i':
			sscanf(optarg, "%"SCNu64, &interval);
			xsg_main_set_interval(interval);
			break;
		case 'n':
			sscanf(optarg, "%"SCNu64, &num);
			break;
		case 'N':
			enable_fontconfig = FALSE;
			break;
		case 'F':
			if (optarg) {
				font_cache_size = atoi(optarg);
			}
			break;
		case 'I':
			if (optarg) {
				image_cache_size = atoi(optarg);
			}
			break;
		case 'l':
			if (optarg) {
				xsg_log_level = atoi(optarg);
			}
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
		case 'f':
			list_fonts = TRUE;
			break;
		case 'd':
			list_dirs = TRUE;
			break;
		case '?':
			print_usage = TRUE;
			break;
		case 'L':
			print_license = TRUE;
			break;
		default:
			break;
		}
	}

	if (print_license) {
		license();
		exit(EXIT_SUCCESS);
	}

	if (print_usage) {
		usage(enable_fontconfig);
		exit(EXIT_SUCCESS);
	}

	if (list_dirs) {
		if (enable_fontconfig) {
			list_font_dirs();
		}
		exit(EXIT_SUCCESS);
	}

	xsg_modules_init();

	if (list_modules) {
		xsg_modules_list();
		exit(EXIT_SUCCESS);
	}

	if (mhelp != NULL) {
		const char *info, *help;

		info = xsg_modules_info(mhelp);
		help = xsg_modules_help(mhelp);

		if (help) {
			printf("%s - %s\n\n%s\n", mhelp, info, help);
		} else {
			printf("%s - %s\n", mhelp, info);
		}

		exit(EXIT_SUCCESS);
	}

	xsg_imlib_init_font_path(enable_fontconfig);

	if (list_fonts) {
		xsg_imlib_list_fonts();
		exit(EXIT_SUCCESS);
	}

	xsg_main_add_update_func(xsg_window_update);
	xsg_main_add_update_func(xsg_widgets_update);

	xsg_imlib_set_font_cache_size(font_cache_size);
	xsg_imlib_set_cache_size(image_cache_size);
	xsg_conf_set_color_lookup(xsg_window_color_lookup);

	if (optind >= argc) {
		usage(enable_fontconfig);
		exit(EXIT_SUCCESS);
	}

	while (optind < argc) {
		char *config_name, *config_buffer, *filename;
		int flags = 0, xoffset = 0, yoffset = 0;

		if (strchr(argv[optind], '=') != NULL) {
			xsg_message("putenv: %s", argv[optind]);
			putenv(argv[optind]);
			optind++;
			continue;
		}

		/* extract {+-}<xoffset>{+-}<yoffset> */
		config_name = xsg_window_extract_position(argv[optind], &flags,
				&xoffset, &yoffset);

		filename = find_config_file(config_name);
		config_buffer = get_config_file(argv[optind], filename);
		parse_config(argv[optind], config_buffer, flags, xoffset, yoffset);

		xsg_free(config_name);
		xsg_free(config_buffer);
		xsg_free(filename);

		optind++;
	}

	xsg_window_init();
	xsg_imlib_init();

	xsg_main_loop(num);

	return EXIT_SUCCESS;
}

