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

#include "main.h"

/******************************************************************************/

typedef struct _poll_t poll_t;

struct _poll_t {
	int fd;
	xsg_main_poll_t events;
	void (*func)(void *, xsg_main_poll_t);
	void *arg;
};

/******************************************************************************/

static xsg_list_t *init_list = NULL;
static xsg_list_t *update_list = NULL;
static xsg_list_t *shutdown_list = NULL;
static xsg_list_t *poll_list = NULL;

static uint64_t counter = 0;
static uint64_t interval = 1000;

/******************************************************************************/

static int timeval_sub(struct timeval *result, struct timeval *x, struct timeval *y) {
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	return x->tv_sec < y->tv_sec;
}

static int sgettimeofday(struct timeval *tv, void *tz) {
	int ret;

	ret = gettimeofday(tv, tz);

	if (unlikely(ret))
		xsg_error("gettimeofday(%p, %p) failed", tv, tz);

	return ret;
}

/******************************************************************************/

void xsg_main_set_interval(uint64_t i) {
	interval = i;
}

uint64_t xsg_main_get_counter(void) {
	return counter;
}

void xsg_main_add_init_func(void (*func)(void)) {
	init_list = xsg_list_append(init_list, (void *) func);
}

void xsg_main_add_update_func(void (*func)(uint64_t)) {
	update_list = xsg_list_append(update_list, func);
}

void xsg_main_add_shutdown_func(void (*func)(void)) {
	shutdown_list = xsg_list_append(shutdown_list, func);
}

void xsg_main_add_poll_func(int fd, void (*func)(void *, xsg_main_poll_t), void *arg, xsg_main_poll_t events) {
	poll_t *p;

	p = xsg_new0(poll_t, 1);
	p->fd = fd;
	p->events = events;
	p->func = func;
	p->arg = arg;

	poll_list = xsg_list_append(poll_list, p);
}

void xsg_main_remove_poll_func(int fd, void (*func)(void *, xsg_main_poll_t), void *arg, xsg_main_poll_t events) {
	xsg_list_t *l, *old;
	poll_t *p;

	l = poll_list;
	while (l) {
		old = l;
		p = l->data;
		l = l->next;

		if (fd >= 0 && fd != p->fd)
			continue;
		if (func != NULL && func != p->func)
			continue;
		if (arg != NULL && arg != p->arg)
			continue;
		if (events != 0 && events != p->events)
			continue;

		poll_list = xsg_list_delete_link(poll_list, old);
	}
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

	while (1) {
		sgettimeofday(&time_start, 0);

		xsg_message("Tick %"PRIu64, counter);
		for (l = update_list; l; l = l->next) {
			void (*func)(uint64_t) = l->data;
			func(counter);
		}

		while (1) {
			sgettimeofday(&time_end, 0);
			timeval_sub(&time_diff, &time_end, &time_start);

			if (timeval_sub(&time_sleep, &time_out, &time_diff))
				break; // timeout

			fd_max = 0;
			FD_ZERO(&read_fds);
			FD_ZERO(&write_fds);
			FD_ZERO(&except_fds);

			for (l = poll_list; l; l = l->next) {
				poll_t *p;

				p = l->data;
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

			fd_count = select(fd_max + 1, &read_fds, &write_fds, &except_fds, &time_sleep);

			if (fd_count == 0)
				break; // timeout

			for (l = poll_list; l; l = l->next) {
				poll_t *p;

				p = l->data;
				if ((p->events & XSG_MAIN_POLL_READ) && (FD_ISSET(p->fd, &read_fds)))
					(p->func)(p->arg, p->events);
				else if ((p->events & XSG_MAIN_POLL_WRITE) && (FD_ISSET(p->fd, &write_fds)))
					(p->func)(p->arg, p->events);
				else if ((p->events & XSG_MAIN_POLL_EXCEPT) && (FD_ISSET(p->fd, &except_fds)))
					(p->func)(p->arg, p->events);
			}
		}
		counter++;
	}
}

void xsg_main_loop() {
	xsg_list_t *l;
	void (*func)(void);

	for (l = init_list; l; l = l->next) {
		func = l->data;
		func();
	}

	loop();

	for (l = shutdown_list; l; l = l->next) {
		func = l->data;
		func();
	}
}

