/* rpn.c
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
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <alloca.h>

#include "rpn.h"
#include "var.h"
#include "conf.h"
#include "modules.h"

/******************************************************************************/

struct _xsg_rpn_t {
	xsg_list_t *op_list;
	xsg_string_t *stack; /* S=string, N=number, X=both */
};

typedef struct _op_t {
	void (*op)(void);
	double (*num_func)(void *arg);
	char *(*str_func)(void *arg);
	void *arg;
} op_t;

/******************************************************************************/

static xsg_list_t *rpn_list = NULL;

static double *num_stack = NULL;
static xsg_string_t **str_stack = NULL;

static unsigned stack_index;
static unsigned max_stack_size = 0;

/******************************************************************************/

static void
op_lt(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				< num_stack[stack_index] ? 1.0 : 0.0;
	}
	stack_index -= 1;
}

static void
op_le(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				<= num_stack[stack_index] ? 1.0 : 0.0;
	}
	stack_index -= 1;
}

static void
op_gt(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				> num_stack[stack_index] ? 1.0 : 0.0;
	}
	stack_index -= 1;
}

static void
op_ge(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				>= num_stack[stack_index] ? 1.0 : 0.0;
	}
	stack_index -= 1;
}

static void
op_eq(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				== num_stack[stack_index] ? 1.0 : 0.0;
	}
	stack_index -= 1;
}

static void
op_ne(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else {
		num_stack[stack_index - 1] = num_stack[stack_index - 1]
				== num_stack[stack_index] ? 0.0 : 1.0;
	}
	stack_index -= 1;
}

static void
op_un(void)
{
	num_stack[stack_index] = isnan(num_stack[stack_index]) ? 1.0 : 0.0;
}

static void
op_isinf(void)
{
	num_stack[stack_index] = isinf(num_stack[stack_index]) ? 1.0 : 0.0;
}

static void
op_if(void){
	xsg_string_t *str_tmp;
	unsigned index;

	index = num_stack[stack_index - 2] != 0.0
			? stack_index - 1 : stack_index;
	str_tmp = str_stack[stack_index - 2];
	str_stack[stack_index - 2] = str_stack[index];
	str_stack[index] = str_tmp;
	num_stack[stack_index - 2] = num_stack[index];
	stack_index -= 2;
}

static void
op_if_str(void)
{
	xsg_string_t *str_tmp;
	unsigned index;

	index = num_stack[stack_index - 2] != 0.0
			? stack_index - 1 : stack_index;
	str_tmp = str_stack[stack_index - 2];
	str_stack[stack_index - 2] = str_stack[index];
	str_stack[index] = str_tmp;
	stack_index -= 2;
}

static void
op_if_num(void)
{
	num_stack[stack_index - 2] = num_stack[stack_index - 2] != 0.0
			? num_stack[stack_index - 1] : num_stack[stack_index];
	stack_index -= 2;
}

static void
op_min(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else if (num_stack[stack_index - 1] > num_stack[stack_index]) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	}
	stack_index -= 1;
}

static void
op_max(void)
{
	if (isnan(num_stack[stack_index - 1])) {
		;
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	} else if (num_stack[stack_index - 1] < num_stack[stack_index]) {
		num_stack[stack_index - 1] = num_stack[stack_index];
	}
	stack_index -= 1;
}

static void
op_limit(void)
{
	if (isnan(num_stack[stack_index - 2])) {
		;
	} else if (isnan(num_stack[stack_index - 1])) {
		num_stack[stack_index - 2] = num_stack[stack_index - 1];
	} else if (isnan(num_stack[stack_index])) {
		num_stack[stack_index - 2] = num_stack[stack_index];
	} else if (num_stack[stack_index - 2] < num_stack[stack_index - 1]) {
		num_stack[stack_index - 2] = DNAN;
	} else if (num_stack[stack_index - 2] > num_stack[stack_index]) {
		num_stack[stack_index - 2] = DNAN;
	}
	stack_index -= 2;
}

static void
op_unkn(void)
{
	num_stack[stack_index + 1] = DNAN;
	stack_index += 1;
}

static void
op_inf(void)
{
	num_stack[stack_index + 1] = DINF;
	stack_index += 1;
}

static void
op_neginf(void)
{
	num_stack[stack_index + 1] = -DINF;
	stack_index += 1;
}

static void
op_add(void)
{
	num_stack[stack_index - 1] += num_stack[stack_index];
	stack_index -= 1;
}

static void
op_sub(void)
{
	num_stack[stack_index - 1] -= num_stack[stack_index];
	stack_index -= 1;
}

static void
op_mul(void)
{
	num_stack[stack_index - 1] *= num_stack[stack_index];
	stack_index -= 1;
}

static void
op_div(void)
{
	num_stack[stack_index - 1] /= num_stack[stack_index];
	stack_index -= 1;
}

static void
op_mod(void)
{
	num_stack[stack_index - 1] = fmod(num_stack[stack_index - 1],
			num_stack[stack_index]);
	stack_index -= 1;
}

static void
op_sin(void)
{
	num_stack[stack_index] = sin(num_stack[stack_index]);
}

static void
op_cos(void)
{
	num_stack[stack_index] = cos(num_stack[stack_index]);
}

static void
op_log(void)
{
	num_stack[stack_index] = log(num_stack[stack_index]);
}

static void
op_exp(void)
{
	num_stack[stack_index] = exp(num_stack[stack_index]);
}

static void
op_sqrt(void)
{
	num_stack[stack_index] = sqrt(num_stack[stack_index]);
}

static void
op_atan(void)
{
	num_stack[stack_index] = atan(num_stack[stack_index]);
}

static void
op_atan2(void)
{
	num_stack[stack_index - 1] = atan2(num_stack[stack_index - 1],
			num_stack[stack_index]);
	stack_index -= 1;
}

static void
op_floor(void)
{
	num_stack[stack_index] = floor(num_stack[stack_index]);
}

static void
op_ceil(void)
{
	num_stack[stack_index] = ceil(num_stack[stack_index]);
}

static void
op_deg2rad(void)
{
	num_stack[stack_index] *= 0.0174532952;
}

static void
op_rad2deg(void)
{
	num_stack[stack_index] *= 57.29577951;
}

static void
op_abs(void)
{
	num_stack[stack_index] = fabs(num_stack[stack_index]);
}

static void
op_dup(void)
{
	num_stack[stack_index + 1] = num_stack[stack_index];
	xsg_string_assign(str_stack[stack_index + 1],
			str_stack[stack_index]->str);
	stack_index += 1;
}

static void
op_dup_num(void)
{
	num_stack[stack_index + 1] = num_stack[stack_index];
	stack_index += 1;
}

static void
op_dup_str(void)
{
	xsg_string_assign(str_stack[stack_index + 1],
			str_stack[stack_index]->str);
	stack_index += 1;
}

static void
op_pop(void)
{
	stack_index -= 1;
}

/******************************************************************************/

static void
op_exc_nn(void)
{
	double num_tmp;

	num_tmp = num_stack[stack_index];
	num_stack[stack_index] = num_stack[stack_index - 1];
	num_stack[stack_index - 1] = num_tmp;
}

static void
op_exc_ss(void)
{
	xsg_string_t *str_tmp;

	str_tmp = str_stack[stack_index];
	str_stack[stack_index] = str_stack[stack_index - 1];
	str_stack[stack_index - 1] = str_tmp;
}

static void
op_exc(void)
{
	xsg_string_t *str_tmp;
	double num_tmp;

	str_tmp = str_stack[stack_index];
	str_stack[stack_index] = str_stack[stack_index - 1];
	str_stack[stack_index - 1] = str_tmp;

	num_tmp = num_stack[stack_index];
	num_stack[stack_index] = num_stack[stack_index - 1];
	num_stack[stack_index - 1] = num_tmp;
}

/******************************************************************************/

static void
op_strlen(void)
{
	num_stack[stack_index] = (double) str_stack[stack_index]->len;
}

static void
op_strcmp(void)
{
	num_stack[stack_index - 1] = (double) strcmp(
			str_stack[stack_index - 1]->str,
			str_stack[stack_index]->str);
	stack_index -= 1;
}

static void
op_strcasecmp(void)
{
	num_stack[stack_index - 1] = (double) strcasecmp(
			str_stack[stack_index - 1]->str,
			str_stack[stack_index]->str);
	stack_index -= 1;
}

static void
op_strup(void)
{
	xsg_string_up(str_stack[stack_index]);
}

static void
op_strdown(void)
{
	xsg_string_down(str_stack[stack_index]);
}

static void
op_strreverse(void)
{
	char *h, *t;

	h = str_stack[stack_index]->str;
	t = h + str_stack[stack_index]->len - 1;

	if (*h) {
		while (h < t) {
			char c;

			c = *h;
			*h = *t;
			h++;
			*t = c;
			t--;
		}
	}
}

/******************************************************************************/

static double
get_number(void *arg)
{
	double *d = (double *) arg;
	return *d;
}

static char *
get_string(void *arg)
{
	return (char *) arg;
}

/******************************************************************************/

static bool
pop(xsg_string_t *stack, const char *str)
{
	size_t len, i;

	len = strlen(str);

	if (stack->len < len) {
		return FALSE;
	}

	for (i = 1; i <= len; i++) {
		char c1, c2;

		c1 = stack->str[stack->len - i];
		c2 = str[len - i];

		if (c1 != c2 && c1 != 'X') {
			return FALSE;
		}
	}

	stack->len -= len;
	stack->str[stack->len] = 0;

	return TRUE;
}

/******************************************************************************/

#define PUSH(s) xsg_string_append(rpn[0]->stack, s)
#define POP(s, log) \
	if (!pop(rpn[0]->stack, s)) { \
		xsg_conf_error("RPN: %s: stack was '%s', but '%s' expected", \
				log, rpn[0]->stack->str, s); \
	}

void
xsg_rpn_parse(uint64_t update, xsg_var_t *var, xsg_rpn_t **rpn)
{
	rpn[0] = xsg_new(xsg_rpn_t, 1);
	rpn[0]->op_list = NULL;
	rpn[0]->stack = xsg_string_new(NULL);

	rpn_list = xsg_list_append(rpn_list, rpn[0]);

	do {
		double number;
		char *string;
		op_t *op;

		op = xsg_new0(op_t, 1);

		if (xsg_conf_find_number(&number)) {
			double *numberp = xsg_new(double, 1);
			*numberp = number;
			op->num_func = get_number;
			op->arg = (void *) numberp;
			PUSH("N");
		} else if (xsg_conf_find_string(&string)) {
			op->str_func = get_string;
			op->arg = (void *) string;
			PUSH("S");
		} else if (xsg_conf_find_command("LT")) {
			POP("NN", "LT");
			op->op = op_lt;
			PUSH("N");
		} else if (xsg_conf_find_command("LE")) {
			POP("NN", "LE");
			op->op = op_le;
			PUSH("N");
		} else if (xsg_conf_find_command("GT")) {
			POP("NN", "GT");
			op->op = op_gt;
			PUSH("N");
		} else if (xsg_conf_find_command("GE")) {
			POP("NN", "GE");
			op->op = op_ge;
			PUSH("N");
		} else if (xsg_conf_find_command("EQ")) {
			POP("NN", "EQ");
			op->op = op_eq;
			PUSH("N");
		} else if (xsg_conf_find_command("NE")) {
			POP("NN", "NE");
			op->op = op_ne;
			PUSH("N");
		} else if (xsg_conf_find_command("UN")) {
			POP("N", "UN");
			op->op = op_un;
			PUSH("N");
		} else if (xsg_conf_find_command("ISINF")) {
			POP("N", "ISINF");
			op->op = op_isinf;
			PUSH("N");
		} else if (xsg_conf_find_command("IF")) {
			/* NOTE: "NXX" needs to be first!
			 * ("NNN" and "NSS" will match "NXX" too) */
			if (pop(rpn[0]->stack, "NXX")) {
				op->op = op_if;
				PUSH("X");
			} else if (pop(rpn[0]->stack, "NNN")) {
				op->op = op_if_num;
				PUSH("N");
			} else if (pop(rpn[0]->stack, "NSS")) {
				op->op = op_if_str;
				PUSH("S");
			} else {
				xsg_conf_error("RPN: IF: stack was '%s', but "
						"'NNN' or 'NSS' expected",
						rpn[0]->stack->str);
			}
		} else if (xsg_conf_find_command("MIN")) {
			POP("NN", "MIN");
			op->op = op_min;
			PUSH("N");
		} else if (xsg_conf_find_command("MAX")) {
			POP("NN", "MAX");
			op->op = op_max;
			PUSH("N");
		} else if (xsg_conf_find_command("LIMIT")) {
			POP("NNN", "LIMIT");
			op->op = op_limit;
			PUSH("N");
		} else if (xsg_conf_find_command("UNKN")) {
			op->op = op_unkn;
			PUSH("N");
		} else if (xsg_conf_find_command("INF")) {
			op->op = op_inf;
			PUSH("N");
		} else if (xsg_conf_find_command("NEGINF")) {
			op->op = op_neginf;
			PUSH("N");
		} else if (xsg_conf_find_command("ADD")) {
			POP("NN", "ADD");
			op->op = op_add;
			PUSH("N");
		} else if (xsg_conf_find_command("SUB")) {
			POP("NN", "SUB");
			op->op = op_sub;
			PUSH("N");
		} else if (xsg_conf_find_command("MUL")) {
			POP("NN", "MUL");
			op->op = op_mul;
			PUSH("N");
		} else if (xsg_conf_find_command("DIV")) {
			POP("NN", "DIV");
			op->op = op_div;
			PUSH("N");
		} else if (xsg_conf_find_command("MOD")) {
			POP("NN", "MOD");
			op->op = op_mod;
			PUSH("N");
		} else if (xsg_conf_find_command("SIN")) {
			POP("N", "SIN");
			op->op = op_sin;
			PUSH("N");
		} else if (xsg_conf_find_command("COS")) {
			POP("N", "COS");
			op->op = op_cos;
			PUSH("N");
		} else if (xsg_conf_find_command("LOG")) {
			POP("N", "LOG");
			op->op = op_log;
			PUSH("N");
		} else if (xsg_conf_find_command("EXP")) {
			POP("N", "EXP");
			op->op = op_exp;
			PUSH("N");
		} else if (xsg_conf_find_command("SQRT")) {
			POP("N", "SQRT");
			op->op = op_sqrt;
			PUSH("N");
		} else if (xsg_conf_find_command("ATAN")) {
			POP("N", "ATAN");
			op->op = op_atan;
			PUSH("N");
		} else if (xsg_conf_find_command("ATAN2")) {
			POP("NN", "ATAN2");
			op->op = op_atan2;
			PUSH("N");
		} else if (xsg_conf_find_command("FLOOR")) {
			POP("N", "FLOOR");
			op->op = op_floor;
			PUSH("N");
		} else if (xsg_conf_find_command("CEIL")) {
			POP("N", "CEIL");
			op->op = op_ceil;
			PUSH("N");
		} else if (xsg_conf_find_command("DEG2RAD")) {
			POP("N", "DEG2RAD");
			op->op = op_deg2rad;
			PUSH("N");
		} else if (xsg_conf_find_command("RAD2DEG")) {
			POP("N", "RAD2DEG");
			op->op = op_rad2deg;
			PUSH("N");
		} else if (xsg_conf_find_command("ABS")) {
			POP("N", "ABS");
			op->op = op_abs;
			PUSH("N");
		} else if (xsg_conf_find_command("DUP")) {
			/* NOTE: "X" needs to be first!
			 * ("N" and "S" will match "X" too) */
			if (pop(rpn[0]->stack, "X")) {
				op->op = op_dup;
				PUSH("XX");
			} else if (pop(rpn[0]->stack, "N")) {
				op->op = op_dup_num;
				PUSH("NN");
			} else if (pop(rpn[0]->stack, "S")) {
				op->op = op_dup_str;
				PUSH("SS");
			} else {
				xsg_conf_error("RPN: DUP: no element on the "
						"stack");
			}
		} else if (xsg_conf_find_command("POP")) {
			if (pop(rpn[0]->stack, "X")) {
				op->op = op_pop;
			} else if (pop(rpn[0]->stack, "N")) {
				op->op = op_pop;
			} else if (pop(rpn[0]->stack, "S")) {
				op->op = op_pop;
			} else {
				xsg_conf_error("RPN: POP: no element on the "
						"stack");
			}
		} else if (xsg_conf_find_command("EXC")) {
			/* NOTE: order is important! "XX" needs to be first!
			 * then "XN", "XS", "NX", "SX" */
			if (pop(rpn[0]->stack, "XX")) {
				op->op = op_exc;
				PUSH("XX");
			} else if (pop(rpn[0]->stack, "XN")) {
				op->op = op_exc;
				PUSH("NX");
			} else if (pop(rpn[0]->stack, "XS")) {
				op->op = op_exc;
				PUSH("SX");
			} else if (pop(rpn[0]->stack, "NX")) {
				op->op = op_exc;
				PUSH("XN");
			} else if (pop(rpn[0]->stack, "SX")) {
				op->op = op_exc;
				PUSH("XS");
			} else if (pop(rpn[0]->stack, "NS")) {
				op->op = op_exc;
				PUSH("SN");
			} else if (pop(rpn[0]->stack, "SN")) {
				op->op = op_exc;
				PUSH("NS");
			} else if (pop(rpn[0]->stack, "NN")) {
				op->op = op_exc_nn;
				PUSH("NN");
			} else if (pop(rpn[0]->stack, "SS")) {
				op->op = op_exc_ss;
				PUSH("SS");
			} else {
				xsg_conf_error("RPN: EXC: no two elements on "
						"the stack");
			}
		} else if (xsg_conf_find_command("STRLEN")) {
			POP("S", "STRLEN");
			op->op = op_strlen;
			PUSH("N");
		} else if (xsg_conf_find_command("STRCMP")) {
			POP("SS", "STRCMP");
			op->op = op_strcmp;
			PUSH("N");
		} else if (xsg_conf_find_command("STRCASECMP")) {
			POP("SS", "STRCASECMP");
			op->op = op_strcasecmp;
			PUSH("N");
		} else if (xsg_conf_find_command("STRUP")) {
			POP("S", "STRUP");
			op->op = op_strup;
			PUSH("S");
		} else if (xsg_conf_find_command("STRDOWN")) {
			POP("S", "STRDOWN");
			op->op = op_strdown;
			PUSH("S");
		} else if (xsg_conf_find_command("STRREVERSE")) {
			POP("S", "STRREVERSE");
			op->op = op_strreverse;
			PUSH("S");
		} else {
			double (*num)(void *);
			char *(*str)(void *);
			void *arg;

			if (!xsg_modules_parse(update, var, &num, &str, &arg)) {
				xsg_conf_error("number, string, module name, "
						"LT, LE, GT, GE, EQ, NE, UN, "
						"ISINF, IF, MIN, MAX, "
						"LIMIT, UNKN, INF, NEGINF, "
						"ADD, SUB, MUL, DIV, MOD, "
						"SIN, COS, LOG, EXP, SQRT, "
						"ATAN, ATAN2, FLOOR, CEIL, "
						"DEG2RAD, RAD2DEG, ABS, DUP, "
						"POP, EXC, STRLEN, STRCMP, "
						"STRCASECMP, STRUP, STRDOWN "
						"or STRREVERSE expected");
			}

			op->num_func = num;
			op->str_func = str;
			op->arg = arg;

			if (num != NULL && str != NULL) {
				PUSH("X");
			} else if (num != NULL) {
				PUSH("N");
			} else if (str != NULL) {
				PUSH("S");
			}
		}

		if (rpn[0]->stack->len > max_stack_size) {
			size_t j;

			num_stack = xsg_renew(double, num_stack,
					rpn[0]->stack->len);
			str_stack = xsg_renew(xsg_string_t *, str_stack,
					rpn[0]->stack->len);

			for (j = max_stack_size; j < rpn[0]->stack->len; j++)
				str_stack[j] = xsg_string_new(NULL);

			max_stack_size = rpn[0]->stack->len;
		}

		rpn[0]->op_list = xsg_list_append(rpn[0]->op_list, op);

	} while (xsg_conf_find_comma());

	if (rpn[0]->stack->len < 1) {
		xsg_conf_error("RPN: no element on the stack");
	}
}

/******************************************************************************/

static void
calc(xsg_rpn_t *rpn)
{
	xsg_list_t *l;

	stack_index = -1;

	for (l = rpn->op_list; l; l = l->next) {
		op_t *op = (op_t *) l->data;

		if (op->op) {
			op->op();
		} else {
			stack_index++;
			if (op->num_func) {
				num_stack[stack_index] = op->num_func(op->arg);
			}
			if (op->str_func) {
				xsg_string_assign(str_stack[stack_index],
						op->str_func(op->arg));
			}
		}
	}
}

double
xsg_rpn_get_num(xsg_rpn_t *rpn)
{
	double num;
	char type;

	type = rpn->stack->str[rpn->stack->len - 1];

	if (unlikely(type != 'N') && unlikely(type != 'X')) {
		xsg_warning("RPN: no number on the stack");
		return DNAN;
	}

	calc(rpn);

	num = num_stack[stack_index];

	return num;
}

char *
xsg_rpn_get_str(xsg_rpn_t *rpn)
{
	char *str;
	char type;

	type = rpn->stack->str[rpn->stack->len - 1];

	if (unlikely(type != 'S') && unlikely(type != 'X')) {
		xsg_warning("RPN: no string on the stack");
		return NULL;
	}

	calc(rpn);

	str = str_stack[stack_index]->str;

	return str;
}


