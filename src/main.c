/* main.c
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
#include <sys/time.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

#include "main.h"
#include "var.h"

/******************************************************************************/

typedef struct flist_t flist_t;

struct flist_t {
	void (*func)(void);
	flist_t *next;
};

/******************************************************************************/

static flist_t *init_list = NULL;
static flist_t *shutdown_list = NULL;
static flist_t *update_list = NULL;
static flist_t *signal_handler_list = NULL;

static xsg_list_t *poll_list = NULL;
static xsg_list_t *timeout_list = NULL;

static uint64_t tick = 0;
static uint64_t interval = 1000;

static int last_received_signum = 0;

static bool time_error = FALSE;

/******************************************************************************/

void
xsg_main_set_interval(uint64_t i)
{
	xsg_message("setting interval to %"PRIu64, i);
	interval = i;
}

uint64_t
xsg_main_get_interval(void)
{
	return interval;
}

/******************************************************************************/

uint64_t
xsg_main_get_tick(void)
{
	return tick;
}

/******************************************************************************/

static flist_t *
add_func(flist_t *list, void (*func)(void))
{
	flist_t *tmp;

	if (unlikely(func == NULL)) {
		return list;
	}

	for (tmp = list; tmp; tmp = tmp->next) {
		if (unlikely(func == tmp->func)) {
			return list;
		}
	}

	tmp = xsg_new(flist_t, 1);
	tmp->func = func;
	tmp->next = list;

	return tmp;
}

static flist_t *
remove_func(flist_t *list, void (*func)(void))
{
	flist_t *tmp, *prev = NULL;

	if (unlikely(func == NULL)) {
		return list;
	}

	tmp = list;
	while (tmp) {
		if (tmp->func == func) {
			if (prev) {
				prev->next = tmp->next;
			} else {
				list = tmp->next;
			}
			xsg_free(tmp);
			break;
		}
		prev = tmp;
		tmp = prev->next;
	}

	return list;
}

/******************************************************************************/

void
xsg_main_add_init_func(void (*func)(void))
{
	init_list = add_func(init_list, func);
}

void
xsg_main_remove_init_func(void (*func)(void))
{
	init_list = remove_func(init_list, func);
}

/******************************************************************************/

void
xsg_main_add_shutdown_func(void (*func)(void))
{
	shutdown_list = add_func(shutdown_list, func);
}

void
xsg_main_remove_shutdown_func(void (*func)(void))
{
	shutdown_list = remove_func(shutdown_list, func);
}

/******************************************************************************/

void
xsg_main_add_update_func(void (*func)(uint64_t))
{
	update_list = add_func(update_list, (void (*)(void)) func);
}

void
xsg_main_remove_update_func(void (*func)(uint64_t))
{
	update_list = remove_func(update_list, (void (*)(void)) func);
}

/******************************************************************************/

void
xsg_main_add_poll(xsg_main_poll_t *poll)
{
	xsg_list_t *l;

	if (unlikely(poll == NULL)) {
		return;
	}

	for (l = poll_list; l; l = l->next) {
		if (unlikely(poll == l->data)) {
			return;
		}
	}

	poll_list = xsg_list_prepend(poll_list, poll);
}

void
xsg_main_remove_poll(xsg_main_poll_t *poll)
{
	if (likely(poll != NULL)) {
		poll_list = xsg_list_remove(poll_list, poll);
	}
}

/******************************************************************************/

void
xsg_main_add_timeout(xsg_main_timeout_t *timeout)
{
	xsg_list_t *l;

	if (unlikely(timeout == NULL)) {
		return;
	}

	for (l = timeout_list; l; l = l->next) {
		if (unlikely(timeout == l->data)) {
			return;
		}
	}

	timeout_list = xsg_list_prepend(timeout_list, timeout);
}

void
xsg_main_remove_timeout(xsg_main_timeout_t *timeout)
{
	if (likely(timeout != NULL)) {
		timeout_list = xsg_list_remove(timeout_list, timeout);
	}
}

/******************************************************************************/

void
xsg_main_add_signal_handler(void (*func)(int signum))
{
	signal_handler_list = add_func(signal_handler_list,
			(void (*)(void)) func);
}

void
xsg_main_remove_signal_handler(void (*func)(int signum))
{
	signal_handler_list = remove_func(signal_handler_list,
			(void (*)(void)) func);
}

/******************************************************************************/

void
xsg_main_set_time_error(void)
{
	time_error = TRUE;
}

/******************************************************************************/

static void
loop(uint64_t num)
{
	struct timeval time_out;
	struct timeval time_start;
	struct timeval time_now;
	struct timeval time_diff;
	struct timeval time_sleep;
	fd_set read_fds;
	fd_set write_fds;
	fd_set except_fds;
	int fd_count, fd_max;
	xsg_list_t *l;
	flist_t *fl;

	time_out.tv_sec = interval / 1000;
	time_out.tv_usec = (interval % 1000) * 1000;

	xsg_message("starting main loop");

	while (1) {
		if (num != 0 && tick == num) {
			return;
		}

		xsg_gettimeofday(&time_start, 0);

		xsg_debug("tick %"PRIu64, tick);

		for (fl = update_list; fl; fl = fl->next) {
			void (*func)(uint64_t) = (void (*)(uint64_t)) fl->func;
			func(tick);
		}

		while (1) {
			xsg_main_timeout_t *timeout = NULL;

			if (time_error) {
				time_error = FALSE;
				xsg_warning("running all timeout functions due "
						"to time error");
				for (l = timeout_list; l; l = l->next) {
					xsg_main_timeout_t *timeout = l->data;
					timeout->func(timeout->arg, TRUE);
				}
			}

			if (last_received_signum != 0) {
				xsg_message("running signal handler functions...");
				for (fl = signal_handler_list; fl; fl = fl->next) {
					void (*func)(int) = (void (*)(int)) fl->func;
					func(last_received_signum);
				}
				last_received_signum = 0;
			}

			xsg_gettimeofday(&time_now, 0);

			if (unlikely(xsg_timeval_sub(&time_diff, &time_now, &time_start))) {
				time_start.tv_sec = time_now.tv_sec;
				time_start.tv_usec = time_now.tv_usec;
				time_diff.tv_sec = 0;
				time_diff.tv_usec = 0;
			}

			if (unlikely(xsg_timeval_sub(&time_sleep, &time_out, &time_diff))) {
				break; /* timeout */
			}

			for (l = timeout_list; l; l = l->next) {
				xsg_main_timeout_t *t = l->data;
				struct timeval timeout_sleep;

				if (xsg_timeval_sub(&timeout_sleep, &t->tv, &time_now)) {
					t->func(t->arg, FALSE);
				}

				if (xsg_timeval_sub(&timeout_sleep, &t->tv, &time_now)) {
					continue;
				}

				if (timercmp(&timeout_sleep, &time_sleep, <)) {
					time_sleep.tv_sec = timeout_sleep.tv_sec;
					time_sleep.tv_usec = timeout_sleep.tv_usec;
					timeout = t;
				}
			}

			xsg_var_flush_dirty();

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

			xsg_debug("sleeping for %u.%06us",
					(unsigned) time_sleep.tv_sec,
					(unsigned) time_sleep.tv_usec);

			fd_count = select(fd_max + 1, &read_fds, &write_fds,
					&except_fds, &time_sleep);

			if (unlikely(fd_count == -1) && (errno == EINTR)) {
				xsg_debug("interrupted by signal");
				continue;
			}

			if (unlikely(fd_count == -1))
				xsg_error("select: %s", strerror(errno));

			if (fd_count == 0) {
				if (timeout != NULL) {
					timeout->func(timeout->arg, FALSE);
					xsg_var_flush_dirty();
					continue;
				} else {
					break; /* timeout */
				}
			}

			xsg_debug("interrupted by file descriptor");

			for (l = poll_list; l; l = l->next) {
				xsg_main_poll_events_t events = 0;
				xsg_main_poll_t *p = l->data;

				if ((p->events & XSG_MAIN_POLL_READ)
				 && (FD_ISSET(p->fd, &read_fds))) {
					events |= XSG_MAIN_POLL_READ;
				}
				if ((p->events & XSG_MAIN_POLL_WRITE)
				 && (FD_ISSET(p->fd, &write_fds))) {
					events |= XSG_MAIN_POLL_WRITE;
				}
				if ((p->events & XSG_MAIN_POLL_EXCEPT)
				 && (FD_ISSET(p->fd, &except_fds))) {
					events |= XSG_MAIN_POLL_EXCEPT;
				}
				if (events) {
					(p->func)(p->arg, events);
				}
			}
			xsg_var_flush_dirty();
		}
		tick++;
	}
}

/******************************************************************************/

static void
init(void)
{
	flist_t *fl;

	xsg_message("running init functions...");

	for (fl = init_list; fl; fl = fl->next) {
		void (*func)(void) = (void (*)(void)) fl->func;

		func();
	}
}

static void
shutdown(void)
{
	static bool shutdown_active = FALSE;
	flist_t *fl;

	if (shutdown_active) {
		return;
	}

	xsg_message("running shutdown functions...");

	for (fl = shutdown_list; fl; fl = fl->next) {
		void (*func)(void) = (void (*)(void)) fl->func;

		func();
	}

	xsg_message("terminating...");
}

static void
signal_handler(int signum)
{
	last_received_signum = signum;

	switch (signum) {
	case SIGABRT:
		xsg_error("received signal %d: SIGABRT", SIGABRT);
		break;
	case SIGALRM:
		xsg_message("received signal %d: SIGALRM", SIGALRM);
		break;
	case SIGCHLD:
		xsg_message("received signal %d: SIGCHLD", SIGCHLD);
		break;
	case SIGCONT:
		xsg_message("received signal %d: SIGCONT", SIGCONT);
		break;
	case SIGFPE:
		xsg_message("received signal %d: SIGFPE", SIGFPE);
		break;
	case SIGHUP:
		xsg_message("received signal %d: SIGHUP", SIGHUP);
		break;
	case SIGILL:
		xsg_message("received signal %d: SIGILL", SIGILL);
		break;
	case SIGINT:
		xsg_error("received signal %d: SIGINT", SIGINT);
		break;
	case SIGKILL:
		xsg_error("received signal %d: SIGKILL", SIGKILL);
		break;
	case SIGPIPE:
		xsg_warning("received signal %d: SIGPIPE", SIGPIPE);
		break;
	case SIGQUIT:
		xsg_error("received signal %d: SIGQUIT", SIGQUIT);
		break;
	case SIGSEGV:
		xsg_error("received signal %d: SIGSEGV", SIGSEGV);
		break;
	case SIGSTOP:
		xsg_error("received signal %d: SIGSTOP", SIGSTOP);
		break;
	case SIGTERM:
		xsg_error("received signal %d: SIGTERM", SIGTERM);
		break;
	case SIGTSTP:
		xsg_message("received signal %d: SIGTSTP", SIGTSTP);
		break;
	case SIGTTIN:
		xsg_message("received signal %d: SIGTTIN", SIGTTIN);
		break;
	case SIGTTOU:
		xsg_message("received signal %d: SIGTTOU", SIGTTOU);
		break;
	case SIGUSR1:
		xsg_message("received signal %d: SIGUSR1", SIGUSR1);
		break;
	case SIGUSR2:
		xsg_message("received signal %d: SIGUSR2", SIGUSR2);
		break;
	default:
		xsg_message("received signal %d", signum);
		break;
	}
}

static int
ssigaction(int signum, const struct sigaction *act, struct sigaction *oldact)
{
	int n;

	n = sigaction(signum, act, oldact);

	if (n == -1) {
		xsg_warning("sigaction for signal number %d failed: %s",
				signum, strerror(errno));
	}

	return n;
}

void
xsg_main_loop(uint64_t num)
{
	struct sigaction action;

	xsg_message("installing signal handler");

	action.sa_handler = signal_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

	ssigaction(SIGABRT, &action, NULL);
	ssigaction(SIGALRM, &action, NULL);
	ssigaction(SIGCHLD, &action, NULL);
	ssigaction(SIGCONT, &action, NULL);
	ssigaction(SIGFPE, &action, NULL);
	ssigaction(SIGHUP, &action, NULL);
	ssigaction(SIGILL, &action, NULL);
	ssigaction(SIGINT, &action, NULL);
	/* ssigaction(SIGKILL, &action, NULL); */
	ssigaction(SIGPIPE, &action, NULL);
	ssigaction(SIGQUIT, &action, NULL);
	ssigaction(SIGSEGV, &action, NULL);
	/* ssigaction(SIGSTOP, &action, NULL); */
	ssigaction(SIGTERM, &action, NULL);
	ssigaction(SIGTSTP, &action, NULL);
	ssigaction(SIGTTOU, &action, NULL);
	ssigaction(SIGUSR1, &action, NULL);
	ssigaction(SIGUSR2, &action, NULL);

	xsg_message("registering shutdown function");

	atexit(shutdown);

	init();

	loop(num);
}

