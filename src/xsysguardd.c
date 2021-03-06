/* xsysguardd.c
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
#include <getopt.h>
#include <errno.h>
#include <time.h>

#include "modules.h"
#include "conf.h"
#include "main.h"
#include "vard.h"
#include "writebuffer.h"

/******************************************************************************/

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

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

static bool log_to_stderr = FALSE;

/******************************************************************************/

int xsg_log_level = XSG_LOG_LEVEL_WARNING;

static void
xsg_logv(const char *domain, int level, const char *format, va_list args)
{
	static bool already_running = FALSE;
	unsigned int pid;

	if (already_running) {
		return;
	}

	already_running = TRUE;

	if (unlikely(level == XSG_LOG_LEVEL_ERROR)) {
		if (xsg_log_level < XSG_LOG_LEVEL_ERROR) {
			exit(EXIT_FAILURE);
		}
	}

	pid = getpid();

	if (log_to_stderr) {
		const char *prefix = NULL;

		if (timestamps) {
			struct timeval tv;
			struct tm *tm;
			char buf[64];

			xsg_gettimeofday(&tv, NULL);

			tm = localtime(&tv.tv_sec);

			if (localtime != NULL) {
				int retval;

				retval = strftime(buf, sizeof(buf) - 1,
						"%H:%M:%S", tm);

				if (retval != 0) {
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

		if (domain == NULL) {
			fprintf(stderr, "%s[%u]xsysguardd: ", prefix, pid);
		} else {
			fprintf(stderr, "%s[%u]xsysguardd/%s: ", prefix, pid,
					domain);
		}

		vfprintf(stderr, format, args);

		if (colored_log) {
			fprintf(stderr, COLOR_DEFAULT"\n");
		} else {
			fprintf(stderr, "\n");
		}

	} else {
		static bool shutdown = FALSE;
		char buffer[1024];
		size_t len = 0;

		if (domain == NULL) {
			len = snprintf(buffer, sizeof(buffer) - 1,
					"[%u]xsysguardd: ", pid);
		}
		if (domain != NULL) {
			len = snprintf(buffer, sizeof(buffer) - 1,
					"[%u]xsysguardd/%s: ", pid, domain);
		}

		len += vsnprintf(buffer + len, sizeof(buffer) - 1 - len,
				format, args);

		xsg_writebuffer_queue_log(level, buffer, len);

		if (unlikely(level == XSG_LOG_LEVEL_ERROR)) {
			shutdown = TRUE;
		}

		if (shutdown) {
			xsg_writebuffer_forced_flush();
		}
	}

	if (unlikely(level == XSG_LOG_LEVEL_ERROR)) {
		exit(EXIT_FAILURE);
	}

	already_running = FALSE;
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
read_data(void *ptr, size_t size, FILE *stream)
{
	if (fread(ptr, 1, size, stream) != size) {
		xsg_error("reading configuration failed: inconsistent data");
	}
}

static uint64_t
read_config(FILE *stream, bool log_level_overwrite)
{
	const char *init = "\nxsysguardd_init_version_1\n";
	unsigned i;
	uint64_t interval;
	uint8_t log_level;
	uint64_t timeout;

	/* init */
	for (i = 0; i < sizeof(init); i++) {
		char c;

		read_data(&c, 1, stream);

		if (c != init[i]) {
			xsg_error("reading configuration failed: "
					"initialization string not found");
		}
	}

	/* interval */
	read_data(&interval, sizeof(uint64_t), stream);
	interval = xsg_uint64_be(interval);
	xsg_main_set_interval(interval);

	/* log level */
	read_data(&log_level, sizeof(uint8_t), stream);
	if (!log_level_overwrite) {
		xsg_log_level = log_level;
	}

	/* last alive timeout */
	read_data(&timeout, sizeof(uint64_t), stream);
	timeout = xsg_uint64_be(timeout);

	while (TRUE) {
		uint8_t type;
		uint32_t id;
		uint64_t update;
		uint32_t config_len;
		char *config;

		/* type */
		read_data(&type, sizeof(uint8_t), stream);

		if (type == 0x00) {
			break;
		}

		/* id */
		read_data(&id, sizeof(uint32_t), stream);
		id = xsg_uint32_be(id);

		/* update */
		read_data(&update, sizeof(uint64_t), stream);
		update = xsg_uint64_be(update);

		/* config_len */
		read_data(&config_len, sizeof(uint32_t), stream);
		config_len = xsg_uint32_be(config_len);

		/* config */
		config = xsg_new(char, config_len + 1);
		read_data(config, config_len, stream);
		config[config_len] = '\0';

		xsg_conf_set_buffer(NULL, config);
		xsg_vard_parse(type, id, update);

		xsg_free(config);
	}

	return timeout;
}

/******************************************************************************/

static void
usage(void)
{
#ifdef XSYSGUARD_MODULE_PATH
	char **pathv;
	char **p;
#endif

	printf( "xsysguardd " XSYSGUARD_VERSION " Copyright 2005-2008 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"Usage: xsysguardd [OPTIONS...]\n\n"
		"Options:\n"
		"  -h, --help          Print this help message to stdout\n"
		"  -H, --mhelp=MODULE  Print help message for MODULE to stdout\n"
		"  -L, --license       Print license to stdout\n"
		"  -m, --modules       Print a list of all available modules to stdout\n"
		"  -s, --stderr        Print log messages to stderr\n"
		"  -c, --color         Enable colored logging\n"
		"  -t, --time          Add current time to each log line\n"
		"  -l, --log=N         Set loglevel to N: ");

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

#ifdef XSYSGUARD_MODULE_PATH
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
#endif
}

static void
license(void)
{
	printf( "xsysguardd " XSYSGUARD_VERSION " Copyright 2005-2008 by Sascha Wessel <sawe@users.sf.net>\n\n"
		"  This program is free software; you can redistribute it and/or modify\n"
		"  it under the terms of the GNU General Public License as published by\n"
		"  the Free Software Foundation; either version 2 of the License, or\n"
		"  (at your option) any later version.\n\n"
		"  This program is distributed in the hope that it will be useful,\n"
		"  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		"  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		"  GNU General Public License for more details.\n\n");
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
	bool print_usage = FALSE;
	bool print_license = FALSE;
	bool log_level_overwrite = FALSE;
	char *mhelp = NULL;

	struct option long_options[] = {
		{ "help",    0, NULL, 'h' },
		{ "mhelp",   1, NULL, 'H' },
		{ "license", 0, NULL, 'L' },
		{ "log",     1, NULL, 'l' },
		{ "color",   0, NULL, 'c' },
		{ "time",    0, NULL, 't' },
		{ "stderr",  0, NULL, 's' },
		{ "modules", 0, NULL, 'm' },
		{ NULL,      0, NULL,  0  }
	};

	if (isatty(STDOUT_FILENO) || isatty(STDIN_FILENO)) {
		print_usage = TRUE;
	}

	if (isatty(STDERR_FILENO)) {
		log_to_stderr = TRUE;
	}

	opterr = 0;
	while (1) {
		int option, option_index = 0;

		option = getopt_long(argc, argv, "hH:Ll:csmt", long_options,
				&option_index);

		if (option == EOF)
			break;

		switch (option) {
		case 'h':
			print_usage = TRUE;
			log_to_stderr = TRUE;
			break;
		case 'H':
			if (optarg && !mhelp) {
				mhelp = xsg_strdup(optarg);
			}
			break;
		case 'l':
			if (optarg) {
				xsg_log_level = atoi(optarg);
			}
			log_level_overwrite = TRUE;
			break;
		case 'c':
			colored_log = TRUE;
			break;
		case 's':
			log_to_stderr = TRUE;
			break;
		case 't':
			timestamps = TRUE;
			break;
		case 'm':
			list_modules = TRUE;
			log_to_stderr = TRUE;
			break;
		case '?':
			print_usage = TRUE;
			log_to_stderr = TRUE;
			break;
		case 'L':
			print_license = TRUE;
			log_to_stderr = TRUE;
			break;
		default:
			break;
		}
	}

	if (list_modules) {
		xsg_modules_init();
		xsg_modules_list();
		exit(EXIT_SUCCESS);
	}

	if (mhelp != NULL) {
		const char *info, *help;

		xsg_modules_init();

		info = xsg_modules_info(mhelp);
		help = xsg_modules_help(mhelp);

		if (help) {
			printf("%s - %s\n\n%s\n", mhelp, info, help);
		} else {
			printf("%s - %s\n", mhelp, info);
		}

		exit(EXIT_SUCCESS);
	}

	if (print_license) {
		license();
		exit(EXIT_SUCCESS);
	}

	if (print_usage) {
		usage();
		exit(EXIT_SUCCESS);
	}

	xsg_modules_init();

	xsg_vard_init(read_config(stdin, log_level_overwrite));

	xsg_main_loop(0);

	return EXIT_SUCCESS;
}

