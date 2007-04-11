/* rpn.c
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
#include <math.h>

#include "rpn.h"
#include "var.h"
#include "modules.h"

/******************************************************************************/

typedef struct _op_t op_t;
typedef struct _rpn_t rpn_t;

struct _op_t {
	double *(*op)(double *stptr);
	double (*func)(void *arg);
	void *arg;
};

struct _rpn_t {
	xsg_list_t *op_list;
	uint32_t stack_size;
};

/******************************************************************************/

static uint32_t rpn_count = 0;
static xsg_list_t *rpn_list = NULL;
static rpn_t **rpn_array = NULL;

/******************************************************************************/

static rpn_t *get_rpn(uint32_t rpn_id) {
	if (unlikely(rpn_array == NULL))
		xsg_error("rpn_array is NULL");

	if (unlikely(rpn_id > rpn_count))
		xsg_error("invalid rpn_id: %"PRIu32, rpn_id);

	return rpn_array[rpn_id];
}

static void build_rpn_array(void) {
	xsg_list_t *l;
	uint32_t rpn_id = 0;

	rpn_array = xsg_new0(rpn_t *, rpn_count);

	for (l = rpn_list; l; l = l->next) {
		rpn_array[rpn_id] = l->data;
		rpn_id++;
	}
}

/******************************************************************************/

static double *op_lt(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] < stptr[0] ? 1.0 : 0.0;
	return stptr - 1;
}

static double *op_le(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] <= stptr[0] ? 1.0 : 0.0;
	return stptr - 1;
}

static double *op_gt(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] > stptr[0] ? 1.0 : 0.0;
	return stptr - 1;
}

static double *op_ge(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] >= stptr[0] ? 1.0 : 0.0;
	return stptr - 1;
}

static double *op_eq(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] == stptr[0] ? 1.0 : 0.0;
	return stptr - 1;
}

static double *op_ne(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else
		stptr[-1] = stptr[-1] == stptr[0] ? 0.0 : 1.0;
	return stptr - 1;
}

static double *op_un(double *stptr) {
	stptr[0] = isnan(stptr[0]) ? 1.0 : 0.0;
	return stptr;
}

static double *op_isinf(double *stptr) {
	stptr[0] = isinf(stptr[0]) ? 1.0 : 0.0;
	return stptr;
}

static double *op_if(double *stptr) {
	stptr[-2] = stptr[-2] != 0.0 ? stptr[-1] : stptr[0];
	return stptr - 2;
}

static double *op_min(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else if (stptr[-1] > stptr[0])
		stptr[-1] = stptr[0];
	return stptr - 1;
}

static double *op_max(double *stptr) {
	if (isnan(stptr[-1]))
		;
	else if (isnan(stptr[0]))
		stptr[-1] = stptr[0];
	else if (stptr[-1] < stptr[0])
		stptr[-1] = stptr[0];
	return stptr - 1;
}

static double *op_limit(double *stptr) {
	if (isnan(stptr[-2]))
		;
	else if (isnan(stptr[-1]))
		stptr[-2] = stptr[-1];
	else if (isnan(stptr[0]))
		stptr[-2] = stptr[0];
	else if (stptr[-2] < stptr[-1])
		stptr[-2] = DNAN;
	else if (stptr[-2] > stptr[0])
		stptr[-2] = DNAN;
	return stptr - 2;
}

static double *op_unkn(double *stptr) {
	stptr[+1] = DNAN;
	return stptr + 1;
}

static double *op_inf(double *stptr) {
	stptr[+1] = DINF;
	return stptr + 1;
}

static double *op_neginf(double *stptr) {
	stptr[+1] = -DINF;
	return stptr + 1;
}

static double *op_add(double *stptr) {
	stptr[-1] += stptr[0];
	return stptr - 1;
}

static double *op_sub(double *stptr) {
	stptr[-1] -= stptr[0];
	return stptr - 1;
}

static double *op_mul(double *stptr) {
	stptr[-1] *= stptr[0];
	return stptr - 1;
}

static double *op_div(double *stptr) {
	stptr[-1] /= stptr[0];
	return stptr - 1;
}

static double *op_mod(double *stptr) {
	stptr[-1] = fmod(stptr[-1], stptr[0]);
	return stptr - 1;
}

static double *op_sin(double *stptr) {
	stptr[0] = sin(stptr[0]);
	return stptr;
}

static double *op_cos(double *stptr) {
	stptr[0] = cos(stptr[0]);
	return stptr;
}

static double *op_log(double *stptr) {
	stptr[0] = log(stptr[0]);
	return stptr;
}

static double *op_exp(double *stptr) {
	stptr[0] = exp(stptr[0]);
	return stptr;
}

static double *op_sqrt(double *stptr) {
	stptr[0] = sqrt(stptr[0]);
	return stptr;
}

static double *op_atan(double *stptr) {
	stptr[0] = atan(stptr[0]);
	return stptr;
}

static double *op_atan2(double *stptr) {
	stptr[-1] = atan2(stptr[-1], stptr[0]);
	return stptr - 1;
}

/******************************************************************************/

void xsg_rpn_init(void) {
	build_rpn_array();
}

uint32_t xsg_rpn_parse(uint32_t var_id, uint64_t update) {
	int stack_size = 0;
	int max_stack_size = 0;
	rpn_t *rpn;

	rpn = xsg_new0(rpn_t, 1);
	rpn->op_list = NULL;
	rpn->stack_size = 0;

	do {
		op_t *op;

		op = xsg_new0(op_t, 1);
		op->op = NULL;
		op->func = NULL;
		op->arg = NULL;

		if (xsg_conf_find_command("LT")) {
			op->op = op_lt;
			stack_size -= 1;
		} else if (xsg_conf_find_command("LE")) {
			op->op = op_le;
			stack_size -= 1;
		} else if (xsg_conf_find_command("GT")) {
			op->op = op_gt;
			stack_size -= 1;
		} else if (xsg_conf_find_command("GE")) {
			op->op = op_ge;
			stack_size -= 1;
		} else if (xsg_conf_find_command("EQ")) {
			op->op = op_eq;
			stack_size -= 1;
		} else if (xsg_conf_find_command("NE")) {
			op->op = op_ne;
			stack_size -= 1;
		} else if (xsg_conf_find_command("UN")) {
			op->op = op_un;
		} else if (xsg_conf_find_command("ISINF")) {
			op->op = op_isinf;
		} else if (xsg_conf_find_command("IF")) {
			op->op = op_if;
			stack_size -= 2;
		} else if (xsg_conf_find_command("MIN")) {
			op->op = op_min;
			stack_size -= 1;
		} else if (xsg_conf_find_command("MAX")) {
			op->op = op_max;
			stack_size -= 1;
		} else if (xsg_conf_find_command("LIMIT")) {
			op->op = op_limit;
			stack_size -= 2;
		} else if (xsg_conf_find_command("UNKN")) {
			op->op = op_unkn;
			stack_size += 1;
		} else if (xsg_conf_find_command("INF")) {
			op->op = op_inf;
			stack_size += 1;
		} else if (xsg_conf_find_command("NEGINF")) {
			op->op = op_neginf;
			stack_size += 1;
		} else if (xsg_conf_find_command("ADD")) {
			op->op = op_add;
			stack_size -= 1;
		} else if (xsg_conf_find_command("SUB")) {
			op->op = op_sub;
			stack_size -= 1;
		} else if (xsg_conf_find_command("MUL")) {
			op->op = op_mul;
			stack_size -= 1;
		} else if (xsg_conf_find_command("DIV")) {
			op->op = op_div;
			stack_size -= 1;
		} else if (xsg_conf_find_command("MOD")) {
			op->op = op_mod;
			stack_size -= 1;
		} else if (xsg_conf_find_command("SIN")) {
			op->op = op_sin;
		} else if (xsg_conf_find_command("COS")) {
			op->op = op_cos;
		} else if (xsg_conf_find_command("LOG")) {
			op->op = op_log;
		} else if (xsg_conf_find_command("EXP")) {
			op->op = op_exp;
		} else if (xsg_conf_find_command("SQRT")) {
			op->op = op_sqrt;
		} else if (xsg_conf_find_command("ATAN")) {
			op->op = op_atan;
		} else if (xsg_conf_find_command("ATAN2")) {
			op->op = op_atan2;
			stack_size -= 1;
		} else {
			xsg_modules_parse_double(var_id, update, &op->func, &op->arg);
			stack_size += 1;
		}

		if (stack_size < 1)
			xsg_error("stack_size < 1");

		max_stack_size = MAX(max_stack_size, stack_size);

		rpn->op_list = xsg_list_append(rpn->op_list, op);

	} while (xsg_conf_find_command(","));

	if (stack_size != 1)
		xsg_error("more than one elemant on the stack");

	rpn->stack_size = max_stack_size;

	rpn_list = xsg_list_append(rpn_list, rpn);
	return rpn_count++;
}

double xsg_rpn_calc(uint32_t rpn_id) {
	xsg_list_t *l;
	double *stack;
	double *stptr;
	rpn_t *rpn;

	rpn = get_rpn(rpn_id);

	stack = (double *)alloca(sizeof(double) * rpn->stack_size);
	stptr = stack - 1;

	for (l = rpn->op_list; l; l = l->next) {
		op_t *op = l->data;
		if (op->op) {
			stptr = op->op(stptr);
		} else {
			stptr[+1] = op->func(op->arg);
			stptr++;
		}
	}
	return *stack;
}


