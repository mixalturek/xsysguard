/* tick.c
 *
 * This file is part of xsysguard <http://xsysguard.sf.net>
 * Copyright (C) 2005-2007 Sascha Wessel <sawe@users.sf.net>
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

/******************************************************************************/

static double get_tick(void *arg) {
	uint64_t *add_ticks = arg;
	double d;

	d = xsg_main_get_tick();
	d += *add_ticks;

	xsg_debug("get_tick: %f", d);

	return d;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t **var, double (**num)(void *), char *(**str)(void *), void **arg, uint32_t n) {
	uint64_t *add_ticks;
	uint32_t i;

	add_ticks = xsg_new(uint64_t, n);

	for (i = 0; i < n; i++) {
		add_ticks[i] = i * update;
		arg[i] = (void *) (add_ticks + i);
		num[i] = get_tick;
	}
}

char *info(char **help) {
	return "returns the current tick";
}

