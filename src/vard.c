/* vard.c
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
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <alloca.h>

#include "vard.h"
#include "rpn.h"
#include "modules.h"
#include "writebuffer.h"

/******************************************************************************/

#define LAST_ALIVE_TIMEOUT 8

/******************************************************************************/

struct _xsg_var_t {
	uint64_t update;
	uint32_t remote_id;

	bool dirty;

	enum {
		END = 0x00,
		NUM = 0x01,
		STR = 0x02
	} type;

	xsg_string_t *str;
	double num;

	xsg_rpn_t *rpn;
};

/******************************************************************************/

static xsg_list_t *var_list = NULL;

static bool dirty = FALSE;

/******************************************************************************/

static uint64_t last_alive_tick = 0;
static uint64_t last_alive_timeout = LAST_ALIVE_TIMEOUT;

/******************************************************************************/

void xsg_vard_queue_vars(void) {
	xsg_list_t *l;

	if (!dirty)
		return;

	if (!xsg_writebuffer_ready())
		return;

	for (l = var_list; l; l = l->next) {
		xsg_var_t *var = l->data;

		if (!var->dirty)
			continue;

		if (var->type == NUM)
			xsg_writebuffer_queue_num(var->remote_id, var->num);
		else if (var->type == STR)
			xsg_writebuffer_queue_str(var->remote_id, var->str);
		else
			xsg_error("invalid var type");

		var->dirty = FALSE;
	}

	dirty = FALSE;

	xsg_writebuffer_flush();
}

/******************************************************************************/

static void update_var(xsg_var_t *var) {
	if (var->type == NUM) {
		double num;

		num = xsg_rpn_get_num(var->rpn);

		if (num != var->num) {
			var->num = num;
			var->dirty = TRUE;
			dirty = TRUE;
		}
	} else if (var->type == STR) {
		char *str;

		str = xsg_rpn_get_str(var->rpn);

		if (strcmp(str, var->str->str) != 0) {
			var->str = xsg_string_assign(var->str, str);
			var->dirty = TRUE;
			dirty = TRUE;
		}
	} else {
		xsg_error("invalid var type");
	}
}

/******************************************************************************/

static void update(uint64_t tick) {
	xsg_list_t *l;

	if (tick - last_alive_tick > last_alive_timeout)
		xsg_error("No alive message received for %"PRIu64" ticks", last_alive_timeout);

	xsg_writebuffer_queue_alive();

	dirty = TRUE;

	for (l = var_list; l; l = l->next) {
		xsg_var_t *var = l->data;
		if ((var->update != 0) && (tick % var->update) == 0)
			update_var(var);
	}

	xsg_vard_queue_vars();
}

/******************************************************************************/

void xsg_var_dirty(xsg_var_t **var, uint32_t n) {
	uint32_t i;

	if (unlikely(var == NULL))
		return;

	for (i = 0; i < n; i++)
		if (likely(var[i] != NULL))
			update_var(var[i]);
}

void xsg_var_flush_dirty(void) {
	xsg_vard_queue_vars();
}

/******************************************************************************/

void xsg_vard_parse(uint8_t type, uint32_t remote_id, uint32_t n, uint64_t update) {
	xsg_rpn_t **rpn;
	xsg_var_t **var;
	xsg_var_t *mem;
	uint32_t i;

	xsg_debug("parse: type=%d, remote_id=%"PRIu32", n=%"PRIu32", update=%"PRIu64, (int) type, remote_id, n, update);

	mem = xsg_new(xsg_var_t, n);

	var = alloca(sizeof(xsg_var_t **) * n);
	rpn = alloca(sizeof(xsg_rpn_t **) * n);

	for (i = 0; i < n; i++)
		var[i] = mem + i;

	xsg_rpn_parse(update, var, rpn, n);

	for (i = 0; i < n; i++) {
		var[i]->update = update;
		var[i]->remote_id = remote_id;
		var[i]->dirty = FALSE;
		var[i]->type = type;
		var[i]->rpn = rpn[i];
		var[i]->num = DNAN;

		if (type == STR)
			var[i]->str = xsg_string_new(NULL);
		else if (type == NUM)
			var[i]->str = NULL;
		else
			xsg_error("invalid var type");
	}

	if (n == 1)
		var_list = xsg_list_append(var_list, var[0]);
}

/******************************************************************************/

static void read_alive(void *arg, xsg_main_poll_events_t events) {
	char buffer;
	ssize_t n;

	n = read(STDIN_FILENO, &buffer, 1);

	if (n == -1 && errno != EINTR)
		xsg_error("Read from stdin failed: %s", strerror(errno));

	if (n == 0)
		xsg_error("Read from stdin returned EOF");

	xsg_debug("Received alive message");

	last_alive_tick = xsg_main_get_tick();
}

static xsg_main_poll_t alive_poll = { STDIN_FILENO, XSG_MAIN_POLL_READ, read_alive, NULL };

/******************************************************************************/

void xsg_var_init() {
	char *timeout;

	timeout = xsg_getenv("XSYSGUARD_DAEMON_TIMEOUT");

	if (timeout != NULL)
		last_alive_timeout = atoll(timeout);

	xsg_main_add_poll(&alive_poll);
	xsg_main_add_update_func(update);
}

