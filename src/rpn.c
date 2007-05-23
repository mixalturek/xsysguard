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
#include <string.h>

#include "rpn.h"
#include "var.h"
#include "modules.h"

/******************************************************************************/

typedef struct _op_t op_t;
typedef struct _rpn_t rpn_t;

struct _op_t {
	void (*op)(void);
	double (*num_func)(void *arg);
	char *(*str_func)(void *arg);
	void *arg;
};

struct _rpn_t {
	xsg_list_t *op_list;
	unsigned num_stack_size;
	unsigned str_stack_size;
};

/******************************************************************************/

static uint32_t rpn_count = 0;
static xsg_list_t *rpn_list = NULL;
static rpn_t **rpn_array = NULL;

static double *num_stack = NULL;
static double *num_stptr = NULL;
static xsg_string_t **str_stack = NULL;
static xsg_string_t **str_stptr = NULL;

static unsigned max_num_stack_size = 0;
static unsigned max_str_stack_size = 0;

/******************************************************************************/

static rpn_t *get_rpn(uint32_t rpn_id) {
	if (unlikely(rpn_array == NULL))
		xsg_error("rpn_array is NULL");

	if (unlikely(rpn_id >= rpn_count))
		xsg_error("invalid rpn_id: %"PRIu32, rpn_id);

	return rpn_array[rpn_id];
}

static void build_rpn_array(void) {
	xsg_list_t *l;
	uint32_t rpn_id = 0;

	xsg_debug("Building rpn_array (rpn_count=%u)", rpn_count);

	rpn_array = xsg_new0(rpn_t *, rpn_count);

	for (l = rpn_list; l; l = l->next) {
		rpn_array[rpn_id] = l->data;
		rpn_id++;
	}
}

static void build_stacks(void) {
	unsigned i;

	num_stack = xsg_new(double, max_num_stack_size);
	str_stack = xsg_new(xsg_string_t *, max_str_stack_size);

	for (i = 0; i < max_str_stack_size; i++)
		str_stack[i] = xsg_string_new(NULL);
}

/******************************************************************************/

static void op_lt(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] < num_stptr[0] ? 1.0 : 0.0;
	num_stptr -= 1;
}

static void op_le(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] <= num_stptr[0] ? 1.0 : 0.0;
	num_stptr -= 1;
}

static void op_gt(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] > num_stptr[0] ? 1.0 : 0.0;
	num_stptr -= 1;
}

static void op_ge(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] >= num_stptr[0] ? 1.0 : 0.0;
	num_stptr -= 1;
}

static void op_eq(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] == num_stptr[0] ? 1.0 : 0.0;
	num_stptr -= 1;
}

static void op_ne(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else
		num_stptr[-1] = num_stptr[-1] == num_stptr[0] ? 0.0 : 1.0;
	num_stptr -= 1;
}

static void op_un(void) {
	num_stptr[0] = isnan(num_stptr[0]) ? 1.0 : 0.0;
}

static void op_isinf(void) {
	num_stptr[0] = isinf(num_stptr[0]) ? 1.0 : 0.0;
}

static void op_if(void) {
	num_stptr[-2] = num_stptr[-2] != 0.0 ? num_stptr[-1] : num_stptr[0];
	num_stptr -= 2;
}

static void op_min(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else if (num_stptr[-1] > num_stptr[0])
		num_stptr[-1] = num_stptr[0];
	num_stptr -= 1;
}

static void op_max(void) {
	if (isnan(num_stptr[-1]))
		;
	else if (isnan(num_stptr[0]))
		num_stptr[-1] = num_stptr[0];
	else if (num_stptr[-1] < num_stptr[0])
		num_stptr[-1] = num_stptr[0];
	num_stptr -= 1;
}

static void op_limit(void) {
	if (isnan(num_stptr[-2]))
		;
	else if (isnan(num_stptr[-1]))
		num_stptr[-2] = num_stptr[-1];
	else if (isnan(num_stptr[0]))
		num_stptr[-2] = num_stptr[0];
	else if (num_stptr[-2] < num_stptr[-1])
		num_stptr[-2] = DNAN;
	else if (num_stptr[-2] > num_stptr[0])
		num_stptr[-2] = DNAN;
	num_stptr -= 2;
}

static void op_unkn(void) {
	num_stptr[+1] = DNAN;
	num_stptr += 1;
}

static void op_inf(void) {
	num_stptr[+1] = DINF;
	num_stptr += 1;
}

static void op_neginf(void) {
	num_stptr[+1] = -DINF;
	num_stptr += 1;
}

static void op_add(void) {
	num_stptr[-1] += num_stptr[0];
	num_stptr -= 1;
}

static void op_sub(void) {
	num_stptr[-1] -= num_stptr[0];
	num_stptr -= 1;
}

static void op_mul(void) {
	num_stptr[-1] *= num_stptr[0];
	num_stptr -= 1;
}

static void op_div(void) {
	num_stptr[-1] /= num_stptr[0];
	num_stptr -= 1;
}

static void op_mod(void) {
	num_stptr[-1] = fmod(num_stptr[-1], num_stptr[0]);
	num_stptr -= 1;
}

static void op_sin(void) {
	num_stptr[0] = sin(num_stptr[0]);
}

static void op_cos(void) {
	num_stptr[0] = cos(num_stptr[0]);
}

static void op_log(void) {
	num_stptr[0] = log(num_stptr[0]);
}

static void op_exp(void) {
	num_stptr[0] = exp(num_stptr[0]);
}

static void op_sqrt(void) {
	num_stptr[0] = sqrt(num_stptr[0]);
}

static void op_atan(void) {
	num_stptr[0] = atan(num_stptr[0]);
}

static void op_atan2(void) {
	num_stptr[-1] = atan2(num_stptr[-1], num_stptr[0]);
	num_stptr -= 1;
}

static void op_floor(void) {
	num_stptr[0] = floor(num_stptr[0]);
}

static void op_ceil(void) {
	num_stptr[0] = ceil(num_stptr[0]);
}

static void op_deg2rad(void) {
	num_stptr[0] *= 0.0174532952;
}

static void op_rad2deg(void) {
	num_stptr[0] *= 57.29577951;
}

static void op_abs(void) {
	num_stptr[0] = fabs(num_stptr[0]);
}

static void op_dup(void) {
	num_stptr[+1] = num_stptr[0];
	num_stptr += 1;
}

static void op_pop(void) {
	num_stptr -= 1;
}

static void op_exc(void) {
	double tmp;
	tmp = num_stptr[0];
	num_stptr[0] = num_stptr[-1];
	num_stptr[-1] = tmp;
}

static void op_strdup(void) {
	str_stptr[+1] = xsg_string_assign(str_stptr[+1], str_stptr[0]->str);
	str_stptr += 1;
}

static void op_strpop(void) {
	str_stptr -= 1;
}

static void op_strexc(void) {
	xsg_string_t *tmp;
	tmp = str_stptr[0];
	str_stptr[0] = str_stptr[-1];
	str_stptr[-1] = tmp;
}

static void op_strlen(void) {
	num_stptr[+1] = (double) strlen(str_stptr[0]->str);
	num_stptr += 1;
	str_stptr -= 1;
}

static void op_strcmp(void) {
	num_stptr[+1] = (double) strcmp(str_stptr[-1]->str, str_stptr[0]->str);
	num_stptr += 1;
	str_stptr -= 2;
}

/******************************************************************************/

void xsg_rpn_init(void) {
	build_rpn_array();
	build_stacks();
}

/******************************************************************************/

#define CHECK_NUM_STACK_SIZE(command, size) \
	if (num_stack_size < size) xsg_error("RPN: " command ": number stack size smaller than " #size)
#define CHECK_STR_STACK_SIZE(command, size) \
	if (str_stack_size < size) xsg_error("RPN: " command ": string stack size smaller than " #size)

uint32_t xsg_rpn_parse(uint32_t var_id, uint64_t update) {
	int num_stack_size = 0;
	int str_stack_size = 0;
	rpn_t *rpn;

	rpn = xsg_new0(rpn_t, 1);
	rpn->op_list = NULL;
	rpn->num_stack_size = 0;
	rpn->str_stack_size = 0;

	do {
		op_t *op;

		op = xsg_new0(op_t, 1);
		op->op = NULL;
		op->num_func = NULL;
		op->str_func = NULL;
		op->arg = NULL;

		if (xsg_conf_find_command("LT")) {
			CHECK_NUM_STACK_SIZE("LT", 2);
			op->op = op_lt;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("LE")) {
			CHECK_NUM_STACK_SIZE("LE", 2);
			op->op = op_le;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("GT")) {
			CHECK_NUM_STACK_SIZE("GT", 2);
			op->op = op_gt;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("GE")) {
			CHECK_NUM_STACK_SIZE("GE", 2);
			op->op = op_ge;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("EQ")) {
			CHECK_NUM_STACK_SIZE("EQ", 2);
			op->op = op_eq;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("NE")) {
			CHECK_NUM_STACK_SIZE("NE", 2);
			op->op = op_ne;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("UN")) {
			CHECK_NUM_STACK_SIZE("UN", 1);
			op->op = op_un;
		} else if (xsg_conf_find_command("ISINF")) {
			CHECK_NUM_STACK_SIZE("ISINF", 1);
			op->op = op_isinf;
		} else if (xsg_conf_find_command("IF")) {
			CHECK_NUM_STACK_SIZE("IF", 3);
			op->op = op_if;
			num_stack_size -= 2;
		} else if (xsg_conf_find_command("MIN")) {
			CHECK_NUM_STACK_SIZE("MIN", 2);
			op->op = op_min;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("MAX")) {
			CHECK_NUM_STACK_SIZE("MAX", 2);
			op->op = op_max;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("LIMIT")) {
			CHECK_NUM_STACK_SIZE("LIMIT", 3);
			op->op = op_limit;
			num_stack_size -= 2;
		} else if (xsg_conf_find_command("UNKN")) {
			op->op = op_unkn;
			num_stack_size += 1;
		} else if (xsg_conf_find_command("INF")) {
			op->op = op_inf;
			num_stack_size += 1;
		} else if (xsg_conf_find_command("NEGINF")) {
			op->op = op_neginf;
			num_stack_size += 1;
		} else if (xsg_conf_find_command("ADD")) {
			CHECK_NUM_STACK_SIZE("ADD", 2);
			op->op = op_add;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("SUB")) {
			CHECK_NUM_STACK_SIZE("SUB", 2);
			op->op = op_sub;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("MUL")) {
			CHECK_NUM_STACK_SIZE("MUL", 2);
			op->op = op_mul;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("DIV")) {
			CHECK_NUM_STACK_SIZE("DIV", 2);
			op->op = op_div;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("MOD")) {
			CHECK_NUM_STACK_SIZE("MOD", 2);
			op->op = op_mod;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("SIN")) {
			CHECK_NUM_STACK_SIZE("SIN", 1);
			op->op = op_sin;
		} else if (xsg_conf_find_command("COS")) {
			CHECK_NUM_STACK_SIZE("COS", 1);
			op->op = op_cos;
		} else if (xsg_conf_find_command("LOG")) {
			CHECK_NUM_STACK_SIZE("LOG", 1);
			op->op = op_log;
		} else if (xsg_conf_find_command("EXP")) {
			CHECK_NUM_STACK_SIZE("EXP", 1);
			op->op = op_exp;
		} else if (xsg_conf_find_command("SQRT")) {
			CHECK_NUM_STACK_SIZE("SQRT", 1);
			op->op = op_sqrt;
		} else if (xsg_conf_find_command("ATAN")) {
			CHECK_NUM_STACK_SIZE("ATAN", 1);
			op->op = op_atan;
		} else if (xsg_conf_find_command("ATAN2")) {
			CHECK_NUM_STACK_SIZE("ATAN2", 2);
			op->op = op_atan2;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("FLOOR")) {
			CHECK_NUM_STACK_SIZE("FLOOR", 1);
			op->op = op_floor;
		} else if (xsg_conf_find_command("CEIL")) {
			CHECK_NUM_STACK_SIZE("CEIL", 1);
			op->op = op_ceil;
		} else if (xsg_conf_find_command("DEG2RAD")) {
			CHECK_NUM_STACK_SIZE("DEG2RAD", 1);
			op->op = op_deg2rad;
		} else if (xsg_conf_find_command("RAD2DEG")) {
			CHECK_NUM_STACK_SIZE("RAD2DEG", 1);
			op->op = op_rad2deg;
		} else if (xsg_conf_find_command("ABS")) {
			CHECK_NUM_STACK_SIZE("ABS", 1);
			op->op = op_abs;
		} else if (xsg_conf_find_command("DUP")) {
			CHECK_NUM_STACK_SIZE("DUP", 1);
			op->op = op_dup;
			num_stack_size += 1;
		} else if (xsg_conf_find_command("POP")) {
			CHECK_NUM_STACK_SIZE("POP", 1);
			op->op = op_pop;
			num_stack_size -= 1;
		} else if (xsg_conf_find_command("EXC")) {
			CHECK_NUM_STACK_SIZE("EXC", 2);
			op->op = op_exc;
		} else if (xsg_conf_find_command("STRDUP")) {
			CHECK_STR_STACK_SIZE("STRDUP", 1);
			op->op = op_strdup;
			str_stack_size += 1;
		} else if (xsg_conf_find_command("STRPOP")) {
			CHECK_STR_STACK_SIZE("STRPOP", 1);
			op->op = op_strpop;
			str_stack_size -= 1;
		} else if (xsg_conf_find_command("STREXC")) {
			CHECK_STR_STACK_SIZE("STREXC", 2);
			op->op = op_strexc;
		} else if (xsg_conf_find_command("STRLEN")) {
			CHECK_STR_STACK_SIZE("STRLEN", 1);
			op->op = op_strlen;
			num_stack_size += 1;
			str_stack_size -= 1;
		} else if (xsg_conf_find_command("STRCMP")) {
			CHECK_STR_STACK_SIZE("STRCMP", 1);
			op->op = op_strcmp;
			num_stack_size += 1;
			str_stack_size -= 2;
		} else {
			xsg_modules_parse(var_id, update, &op->num_func, &op->str_func, &op->arg);
			if (op->num_func != NULL)
				num_stack_size += 1;
			if (op->str_func != NULL)
				str_stack_size += 1;
		}

		max_num_stack_size = MAX(max_num_stack_size, num_stack_size);
		max_str_stack_size = MAX(max_str_stack_size, str_stack_size);

		rpn->op_list = xsg_list_append(rpn->op_list, op);

	} while (xsg_conf_find_command(","));

	rpn->num_stack_size = num_stack_size;
	rpn->str_stack_size = str_stack_size;

	rpn_list = xsg_list_append(rpn_list, rpn);
	return rpn_count++;
}

static void calc(rpn_t *rpn) {
	xsg_list_t *l;

	num_stptr = num_stack - 1;
	str_stptr = str_stack - 1;

	for (l = rpn->op_list; l; l = l->next) {
		op_t *op = l->data;
		if (op->op) {
			op->op();
		} else {
			if (op->num_func) {
				num_stptr[+1] = op->num_func(op->arg);
				num_stptr++;
			}
			if (op->str_func) {
				str_stptr[+1] = xsg_string_assign(str_stptr[+1], op->str_func(op->arg));
				str_stptr++;
			}
		}
	}
}

double xsg_rpn_get_num(uint32_t rpn_id) {
	double num;
	rpn_t *rpn;

	rpn = get_rpn(rpn_id);

	if (unlikely(rpn->num_stack_size < 1)) {
		xsg_warning("RPN(%"PRIu32"): no number left on stack");
		return DNAN;
	}

	calc(rpn);

	num = num_stptr[0];

	xsg_debug("RPN(%"PRIu32"): %f", rpn_id, num);

	return num;
}

char *xsg_rpn_get_str(uint32_t rpn_id) {
	char *str;
	rpn_t *rpn;

	rpn = get_rpn(rpn_id);

	if (unlikely(rpn->str_stack_size < 1)) {
		xsg_warning("RPN(%"PRIu32"): no string left on stack");
		return NULL;
	}

	calc(rpn);

	str = str_stptr[0]->str;

	xsg_debug("RPN(%"PRIu32"): \"%s\"", rpn_id, str);

	return str;
}


