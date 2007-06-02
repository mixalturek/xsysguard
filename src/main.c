/* main.c
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
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "main.h"
#include "var.h"

/******************************************************************************/

static xsg_list_t *init_list = NULL;
static xsg_list_t *update_list = NULL;
static xsg_list_t *shutdown_list = NULL;
static xsg_list_t *poll_list = NULL;
static xsg_list_t *signal_handler_list = NULL;

static uint64_t tick = 0;
static uint64_t interval = 1000;

/******************************************************************************/

void xsg_main_set_interval(uint64_t i) {
	interval = i;
}

uint64_t xsg_main_get_interval() {
	return interval;
}

/******************************************************************************/

uint64_t xsg_main_get_tick(void) {
	return tick;
}

/******************************************************************************/

void xsg_main_add_init_func(void (*func)(void)) {
	init_list = xsg_list_append(init_list, (void *) func);
}

void xsg_main_add_update_func(void (*func)(uint64_t)) {
	update_list = xsg_list_append(update_list, func);
}

void xsg_main_add_shutdown_func(void (*func)(void)) {
	shutdown_list = xsg_list_append(shutdown_list, func);
}

void xsg_main_add_signal_handler(void (*func)(int signum)) {
	signal_handler_list = xsg_list_append(signal_handler_list, func);
}

/******************************************************************************/

void xsg_main_add_poll(xsg_main_poll_t *poll) {
	if (unlikely(poll == NULL))
		return;

	poll_list = xsg_list_prepend(poll_list, poll);
}

void xsg_main_remove_poll(xsg_main_poll_t *poll) {
	if (unlikely(poll == NULL))
		return;

	poll_list = xsg_list_remove(poll_list, poll);
}

/******************************************************************************/

static void loop(void) {
	struct timeval time_out;
	struct timeval time_start;
	struct timeval time_end;
	struct timeval time_diff;
	struct timeval time_sleep;
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
	int fd_count, fd_max;
	xsg_list_t *l;

	time_out.tv_sec = interval / 1000;
	time_out.tv_usec = (interval % 1000) * 1000;

	xsg_message("Starting main loop");

	while (1) {
		xsg_gettimeofday(&time_start, 0);

		xsg_message("Tick %"PRIu64, tick);

		for (l = update_list; l; l = l->next) {
			void (*func)(uint64_t) = l->data;
			func(tick);
		}

		while (1) {
			xsg_gettimeofday(&time_end, 0);
			xsg_timeval_sub(&time_diff, &time_end, &time_start);

			if (xsg_timeval_sub(&time_sleep, &time_out, &time_diff))
				break; // timeout

			fd_max = 0;
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&except_fds);

			for (l = poll_list; l; l = l->next) {
				xsg_main_poll_t *p = l->data;;

				if (p->events & XSG_MAIN_POLL_READ) {
					FD_SET(p->fd, &read_fds);
					fd_max = MAX(fd_max, p->fd);
				}
				if (p->events & XSG_MAIN_POLL_WRITE) {
					FD_SET(p->fd, &write_fds);
					fd_max = MAX(fd_max, p->fd);
				}
				if (p->events & XSG_MAIN_POLL_EXCEPT) {
					FD_SET(p->fd, &except_fds);
					fd_max = MAX(fd_max, p->fd);
				}
			}

			xsg_message("Sleeping for %u.%06us", (unsigned) time_sleep.tv_sec, (unsigned) time_sleep.tv_usec);

			fd_count = select(fd_max + 1, &read_fds, &write_fds, &except_fds, &time_sleep);

			if (unlikely(fd_count == -1) && (errno == EINTR))
				continue;

			if (unlikely(fd_count == -1))
				xsg_error("select: %s", strerror(errno));

			if (fd_count == 0)
				break; // timeout

			xsg_message("Interrupted by file descriptor");

			for (l = poll_list; l; l = l->next) {
				xsg_main_poll_events_t events = 0;
				xsg_main_poll_t *p = l->data;

				if ((p->events & XSG_MAIN_POLL_READ) && (FD_ISSET(p->fd, &read_fds)))
					events |= XSG_MAIN_POLL_READ;
				if ((p->events & XSG_MAIN_POLL_WRITE) && (FD_ISSET(p->fd, &write_fds)))
					events |= XSG_MAIN_POLL_WRITE;
				if ((p->events & XSG_MAIN_POLL_EXCEPT) && (FD_ISSET(p->fd, &except_fds)))
					events |= XSG_MAIN_POLL_EXCEPT;
				if (events)
					(p->func)(p->arg, events);
			}
			xsg_var_flush_dirty();
		}
		tick++;
	}
}

static void shutdown(void) {
	xsg_list_t *l;
	void (*func)(void);

	xsg_message("Shutdown...");

	for (l = shutdown_list; l; l = l->next) {
		func = l->data;
		func();
	}

	_exit(EXIT_SUCCESS);
}

static void signal_handler(int signum) {
	xsg_list_t *l;
	void (*func)(int);

	xsg_message("Received signal %d: %s", signum, sys_siglist[signum]);

	for (l = signal_handler_list; l; l = l->next) {
		func = l->data;
		func(signum);
	}

	if (signum == SIGINT || signum == SIGQUIT || signum == SIGTERM)
		xsg_error("Received signal %d: %s", signum, sys_siglist[signum]);
}

static int ssigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
	int n;

	n = sigaction(signum, act, oldact);

	if (n == -1)
		xsg_warning("sigaction for signal number %d failed: %s", signum, strerror(errno));

	return n;
}

void xsg_main_loop() {
	xsg_list_t *l;
	void (*func)(void);
	struct sigaction action;

	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	ssigaction(SIGABRT, &action, NULL);
	ssigaction(SIGINT, &action, NULL);
	ssigaction(SIGQUIT, &action, NULL);
	ssigaction(SIGHUP, &action, NULL);
	ssigaction(SIGTERM, &action, NULL);
	ssigaction(SIGPIPE, &action, NULL);
	ssigaction(SIGUSR1, &action, NULL);
	ssigaction(SIGUSR2, &action, NULL);

	atexit(shutdown);

	for (l = init_list; l; l = l->next) {
		func = l->data;
		func();
	}

	loop();
}

