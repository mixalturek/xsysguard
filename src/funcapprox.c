/* funcapprox.c
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
#include <math.h>

/******************************************************************************/

typedef struct _value_t {
	uint64_t ms; // microseconds since the Epoche
	double value;
} value_t;

struct _xsg_funcapprox_t {
	uint64_t update;
	xsg_list_t *value_list;
};

/******************************************************************************/

xsg_funcapprox_t *xsg_funcapprox_new(uint64_t update) {
	xsg_funcapprox_t *funcapprox;

	funcapprox = xsg_new(xsg_funcapprox_t, 1);
	funcapprox->update = update;
	funcapprox->value_list = NULL;

	return funcapprox;
}

/******************************************************************************/

void xsg_funcapprox_add(xsg_funcapprox_t *funcapprox, const struct timeval *tv, double value) {
	value_t *v;

	if (unlikely(funcapprox == NULL))
		return;

	if (unlikely(tv == NULL))
		return;

	v = xsg_new(value_t, 1);
	v->ms = tv->tv_sec * 1000 * 1000 + tv->tv_usec;
	v->value = value;

	funcapprox->value_list = xsg_list_prepend(funcapprox->value_list, v);
}

/******************************************************************************/

double xsg_funcapprox_get(xsg_funcapprox_t *funcapprox, uint32_t back) {
	struct timeval now_tv;
	uint64_t now_ms, diff_ms, search_ms;
	value_t *prev_value, *next_value;
	xsg_list_t *l;

	if (unlikely(funcapprox == NULL))
		return DNAN;

	if (unlikely(funcapprox->value_list == NULL))
		return DNAN;

	xsg_gettimeofday(&now_tv, NULL);

	now_ms = now_tv.tv_sec * 1000 * 1000 + now_tv.tv_usec;
	diff_ms = funcapprox->update * xsg_main_get_interval();
	search_ms = now_ms - diff_ms * back;

	prev_value = (value_t *) funcapprox->value_list->data;
	next_value = (value_t *) funcapprox->value_list->data;

	for (l = funcapprox->value_list; l; l = l->next) {
		value_t *value = l->data;

		if (value->ms == search_ms) {
			return value->value;
		} else if (value->ms > search_ms) {
			if (next_value->ms > value->ms)
				next_value = value;
		} else if (value->ms < search_ms) {
			if (prev_value->ms < value->ms)
				prev_value = value;
		}
	}

	if (next_value->ms < search_ms)
		return DNAN;
	if (prev_value->ms > search_ms)
		return DNAN;

	if (isnan(next_value->value) || isinf(next_value->value))
		return DNAN;
	if (isnan(prev_value->value) || isinf(prev_value->value))
		return DNAN;

	return prev_value->value + (((search_ms - prev_value->ms) * ABS(prev_value->value - next_value->value)) / next_value->ms - prev_value->ms);
}


