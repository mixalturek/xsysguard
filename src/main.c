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

#include "main.h"

#include <glib.h> // FIXME

/******************************************************************************/

static xsg_list_t *init_list = NULL;
static xsg_list_t *update_list = NULL;
static xsg_list_t *shutdown_list = NULL;

static uint64_t counter = 0;
static uint64_t interval = 1000;

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
	update_list = xsg_list_append(update_list, (gpointer) func);
}

void xsg_main_add_shutdown_func(void (*func)(void)) {
	shutdown_list = xsg_list_append(shutdown_list, (gpointer) func);
}

/******************************************************************************/

static bool update(void *data) {
	xsg_list_t *l;

	counter++;
	xsg_message("Tick %" PRIu64, counter);
	for (l = update_list; l; l = l->next) {
		void (*func)(uint64_t) = l->data;
		func(counter);
	}

	return TRUE; /* never remove this source */
}

void xsg_main_loop() {
	GMainLoop *loop;
	GSource *timeout;
	xsg_list_t *l;

	timeout = g_timeout_source_new(interval);
	g_source_set_callback(timeout, update, NULL, NULL);
	g_source_attach(timeout, NULL);

	for (l = init_list; l; l = l->next) {
		void (*func)(void) = l->data;
		func();
	}

	loop = g_main_loop_new(NULL, FALSE);

	g_main_loop_run(loop);

	for (l = shutdown_list; l; l = l->next) {
		void (*func)(void) = l->data;
		func();
	}
}

