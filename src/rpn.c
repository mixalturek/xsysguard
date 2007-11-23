/* rpn.c
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
	xsg_string_t *stack; // S=string, N=number, X=both
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

static void op_lt(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] < num_stack[stack_index] ? 1.0 : 0.0;
	stack_index -= 1;
}

static void op_le(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] <= num_stack[stack_index] ? 1.0 : 0.0;
	stack_index -= 1;
}

static void op_gt(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] > num_stack[stack_index] ? 1.0 : 0.0;
	stack_index -= 1;
}

static void op_ge(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] >= num_stack[stack_index] ? 1.0 : 0.0;
	stack_index -= 1;
}

static void op_eq(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] == num_stack[stack_index] ? 1.0 : 0.0;
	stack_index -= 1;
}

static void op_ne(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else
		num_stack[stack_index - 1] = num_stack[stack_index - 1] == num_stack[stack_index] ? 0.0 : 1.0;
	stack_index -= 1;
}

static void op_un(void) {
	num_stack[stack_index] = isnan(num_stack[stack_index]) ? 1.0 : 0.0;
}

static void op_isinf(void) {
	num_stack[stack_index] = isinf(num_stack[stack_index]) ? 1.0 : 0.0;
}

static void op_if(void) {
	xsg_string_t *str_tmp;
	unsigned index;

	index = num_stack[stack_index - 2] != 0.0 ? stack_index - 1 : stack_index;
	str_tmp = str_stack[stack_index - 2];
	str_stack[stack_index - 2] = str_stack[index];
	str_stack[index] = str_tmp;
	num_stack[stack_index - 2] = num_stack[index];
	stack_index -= 2;
}

static void op_if_str(void) {
	xsg_string_t *str_tmp;
	unsigned index;

	index = num_stack[stack_index - 2] != 0.0 ? stack_index - 1 : stack_index;
	str_tmp = str_stack[stack_index - 2];
	str_stack[stack_index - 2] = str_stack[index];
	str_stack[index] = str_tmp;
	stack_index -= 2;
}

static void op_if_num(void) {
	num_stack[stack_index - 2] = num_stack[stack_index - 2] != 0.0 ? num_stack[stack_index - 1] : num_stack[stack_index];
	stack_index -= 2;
}

static void op_min(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else if (num_stack[stack_index - 1] > num_stack[stack_index])
		num_stack[stack_index - 1] = num_stack[stack_index];
	stack_index -= 1;
}

static void op_max(void) {
	if (isnan(num_stack[stack_index - 1]))
		;
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 1] = num_stack[stack_index];
	else if (num_stack[stack_index - 1] < num_stack[stack_index])
		num_stack[stack_index - 1] = num_stack[stack_index];
	stack_index -= 1;
}

static void op_limit(void) {
	if (isnan(num_stack[stack_index - 2]))
		;
	else if (isnan(num_stack[stack_index - 1]))
		num_stack[stack_index - 2] = num_stack[stack_index - 1];
	else if (isnan(num_stack[stack_index]))
		num_stack[stack_index - 2] = num_stack[stack_index];
	else if (num_stack[stack_index - 2] < num_stack[stack_index - 1])
		num_stack[stack_index - 2] = DNAN;
	else if (num_stack[stack_index - 2] > num_stack[stack_index])
		num_stack[stack_index - 2] = DNAN;
	stack_index -= 2;
}

static void op_unkn(void) {
	num_stack[stack_index + 1] = DNAN;
	stack_index += 1;
}

static void op_inf(void) {
	num_stack[stack_index + 1] = DINF;
	stack_index += 1;
}

static void op_neginf(void) {
	num_stack[stack_index + 1] = -DINF;
	stack_index += 1;
}

static void op_add(void) {
	num_stack[stack_index - 1] += num_stack[stack_index];
	stack_index -= 1;
}

static void op_sub(void) {
	num_stack[stack_index - 1] -= num_stack[stack_index];
	stack_index -= 1;
}

static void op_mul(void) {
	num_stack[stack_index - 1] *= num_stack[stack_index];
	stack_index -= 1;
}

static void op_div(void) {
	num_stack[stack_index - 1] /= num_stack[stack_index];
	stack_index -= 1;
}

static void op_mod(void) {
	num_stack[stack_index - 1] = fmod(num_stack[stack_index - 1], num_stack[stack_index]);
	stack_index -= 1;
}

static void op_sin(void) {
	num_stack[stack_index] = sin(num_stack[stack_index]);
}

static void op_cos(void) {
	num_stack[stack_index] = cos(num_stack[stack_index]);
}

static void op_log(void) {
	num_stack[stack_index] = log(num_stack[stack_index]);
}

static void op_exp(void) {
	num_stack[stack_index] = exp(num_stack[stack_index]);
}

static void op_sqrt(void) {
	num_stack[stack_index] = sqrt(num_stack[stack_index]);
}

static void op_atan(void) {
	num_stack[stack_index] = atan(num_stack[stack_index]);
}

static void op_atan2(void) {
	num_stack[stack_index - 1] = atan2(num_stack[stack_index - 1], num_stack[stack_index]);
	stack_index -= 1;
}

static void op_floor(void) {
	num_stack[stack_index] = floor(num_stack[stack_index]);
}

static void op_ceil(void) {
	num_stack[stack_index] = ceil(num_stack[stack_index]);
}

static void op_deg2rad(void) {
	num_stack[stack_index] *= 0.0174532952;
}

static void op_rad2deg(void) {
	num_stack[stack_index] *= 57.29577951;
}

static void op_abs(void) {
	num_stack[stack_index] = fabs(num_stack[stack_index]);
}

static void op_dup(void) {
	num_stack[stack_index + 1] = num_stack[stack_index];
	str_stack[stack_index + 1] = xsg_string_assign(str_stack[stack_index + 1], str_stack[stack_index]->str);
	stack_index += 1;
}

static void op_dup_num(void) {
	num_stack[stack_index + 1] = num_stack[stack_index];
	stack_index += 1;
}

static void op_dup_str(void) {
	str_stack[stack_index + 1] = xsg_string_assign(str_stack[stack_index + 1], str_stack[stack_index]->str);
	stack_index += 1;
}

static void op_pop(void) {
	stack_index -= 1;
}

/******************************************************************************/

static void op_exc_nn(void) {
	double num_tmp;

	num_tmp = num_stack[stack_index];
	num_stack[stack_index] = num_stack[stack_index - 1];
	num_stack[stack_index - 1] = num_tmp;
}

static void op_exc_ss(void) {
	xsg_string_t *str_tmp;

	str_tmp = str_stack[stack_index];
	str_stack[stack_index] = str_stack[stack_index - 1];
	str_stack[stack_index - 1] = str_tmp;
}

static void op_exc(void) {
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

static void op_strlen(void) {
	num_stack[stack_index] = (double) str_stack[stack_index]->len;
}

static void op_strcmp(void) {
	num_stack[stack_index - 1] = (double) strcmp(str_stack[stack_index - 1]->str, str_stack[stack_index]->str);
	stack_index -= 1;
}

static void op_strcasecmp(void) {
	num_stack[stack_index - 1] = (double) strcasecmp(str_stack[stack_index - 1]->str, str_stack[stack_index]->str);
	stack_index -= 1;
}

static void op_strup(void) {
	str_stack[stack_index] = xsg_string_up(str_stack[stack_index]);
}

static void op_strdown(void) {
	str_stack[stack_index] = xsg_string_down(str_stack[stack_index]);
}

static void op_strreverse(void) {
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

static double get_number(void *arg) {
	double *d = (double *) arg;
	return *d;
}

static char *get_string(void *arg) {
	return (char *) arg;
}

/******************************************************************************/

static bool pop(xsg_string_t *stack, const char *str) {
	size_t len, i;

	len = strlen(str);

	if (stack->len < len)
		return FALSE;

	for (i = 1; i <= len; i++) {
		char c1, c2;

		c1 = stack->str[stack->len - i];
		c2 = str[len - i];

		if (c1 != c2 && c1 != 'X')
			return FALSE;
	}

	stack->len -= len;
	stack->str[stack->len] = 0;

	return TRUE;
}

#define PUSH(s) rpn[i]->stack = xsg_string_append(rpn[i]->stack, s)
#define POP(s, log) if (!pop(rpn[i]->stack, s)) { \
	xsg_conf_error("RPN: %s: stack was '%s', but '%s' expected", log, rpn[i]->stack->str, s); }

void xsg_rpn_parse(uint64_t update, xsg_var_t **var, xsg_rpn_t **rpn, uint32_t n) {
	xsg_rpn_t *mem;
	uint32_t i;

	mem = xsg_new(xsg_rpn_t, n);

	for (i = 0; i < n; i++)
		rpn[i] = mem + i;

	for (i = 0; i < n; i++) {
		rpn[i]->op_list = NULL;
		rpn[i]->stack = xsg_string_new(NULL);

		rpn_list = xsg_list_append(rpn_list, rpn[i]);
	}

	do {
		double number;
		char *string;
		op_t *op;

		op = xsg_new0(op_t, n);

		if (xsg_conf_find_number(&number)) {
			for (i = 0; i < n; i++) {
				double *numberp = xsg_new(double, 1);
				*numberp = number;
				op[i].num_func = get_number;
				op[i].arg = (void *) numberp;
				PUSH("N");
			}
		} else if (xsg_conf_find_string(&string)) {
			for (i = 0; i < n; i++) {
				op[i].str_func = get_string;
				op[i].arg = (void *) string;
				PUSH("S");
			}
		} else if (xsg_conf_find_command("LT")) {
			for (i = 0; i < n; i++) {
				POP("NN", "LT");
				op[i].op = op_lt;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("LE")) {
			for (i = 0; i < n; i++) {
				POP("NN", "LE");
				op[i].op = op_le;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("GT")) {
			for (i = 0; i < n; i++) {
				POP("NN", "GT");
				op[i].op = op_gt;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("GE")) {
			for (i = 0; i < n; i++) {
				POP("NN", "GE");
				op[i].op = op_ge;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("EQ")) {
			for (i = 0; i < n; i++) {
				POP("NN", "EQ");
				op[i].op = op_eq;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("NE")) {
			for (i = 0; i < n; i++) {
				POP("NN", "NE");
				op[i].op = op_ne;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("UN")) {
			for (i = 0; i < n; i++) {
				POP("N", "UN");
				op[i].op = op_un;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("ISINF")) {
			for (i = 0; i < n; i++) {
				POP("N", "ISINF");
				op[i].op = op_isinf;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("IF")) {
			for (i = 0; i < n; i++) {
				// NOTE: "NXX" needs to be first! ("NNN" and "NSS" will match "NXX" too)
				if (pop(rpn[i]->stack, "NXX")) {
					op[i].op = op_if;
					PUSH("X");
				} else if (pop(rpn[i]->stack, "NNN")) {
					op[i].op = op_if_num;
					PUSH("N");
				} else if (pop(rpn[i]->stack, "NSS")) {
					op[i].op = op_if_str;
					PUSH("S");
				} else {
					xsg_conf_error("RPN: IF: stack was '%s', but 'NNN' or 'NSS' expected",
							rpn[i]->stack->str);
				}
			}
		} else if (xsg_conf_find_command("MIN")) {
			for (i = 0; i < n; i++) {
				POP("NN", "MIN");
				op[i].op = op_min;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("MAX")) {
			for (i = 0; i < n; i++) {
				POP("NN", "MAX");
				op[i].op = op_max;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("LIMIT")) {
			for (i = 0; i < n; i++) {
				POP("NNN", "LIMIT");
				op[i].op = op_limit;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("UNKN")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_unkn;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("INF")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_inf;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("NEGINF")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_neginf;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("ADD")) {
			for (i = 0; i < n; i++) {
				POP("NN", "ADD");
				op[i].op = op_add;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("SUB")) {
			for (i = 0; i < n; i++) {
				POP("NN", "SUB");
				op[i].op = op_sub;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("MUL")) {
			for (i = 0; i < n; i++) {
				POP("NN", "MUL");
				op[i].op = op_mul;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("DIV")) {
			for (i = 0; i < n; i++) {
				POP("NN", "DIV");
				op[i].op = op_div;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("MOD")) {
			for (i = 0; i < n; i++) {
				POP("NN", "MOD");
				op[i].op = op_mod;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("SIN")) {
			for (i = 0; i < n; i++) {
				POP("N", "SIN");
				op[i].op = op_sin;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("COS")) {
			for (i = 0; i < n; i++) {
				POP("N", "COS");
				op[i].op = op_cos;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("LOG")) {
			for (i = 0; i < n; i++) {
				POP("N", "LOG");
				op[i].op = op_log;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("EXP")) {
			for (i = 0; i < n; i++) {
				POP("N", "EXP");
				op[i].op = op_exp;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("SQRT")) {
			for (i = 0; i < n; i++) {
				POP("N", "SQRT");
				op[i].op = op_sqrt;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("ATAN")) {
			for (i = 0; i < n; i++) {
				POP("N", "ATAN");
				op[i].op = op_atan;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("ATAN2")) {
			for (i = 0; i < n; i++) {
				POP("NN", "ATAN2");
				op[i].op = op_atan2;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("FLOOR")) {
			for (i = 0; i < n; i++) {
				POP("N", "FLOOR");
				op[i].op = op_floor;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("CEIL")) {
			for (i = 0; i < n; i++) {
				POP("N", "CEIL");
				op[i].op = op_ceil;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("DEG2RAD")) {
			for (i = 0; i < n; i++) {
				POP("N", "DEG2RAD");
				op[i].op = op_deg2rad;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("RAD2DEG")) {
			for (i = 0; i < n; i++) {
				POP("N", "RAD2DEG");
				op[i].op = op_rad2deg;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("ABS")) {
			for (i = 0; i < n; i++) {
				POP("N", "ABS");
				op[i].op = op_abs;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("DUP")) {
			for (i = 0; i < n; i++) {
				// NOTE: "X" needs to be first! ("N" and "S" will match "X" too)
				if (pop(rpn[i]->stack, "X")) {
					op[i].op = op_dup;
					PUSH("XX");
				} else if (pop(rpn[i]->stack, "N")) {
					op[i].op = op_dup_num;
					PUSH("NN");
				} else if (pop(rpn[i]->stack, "S")) {
					op[i].op = op_dup_str;
					PUSH("SS");
				} else {
					xsg_conf_error("RPN: DUP: no element on the stack");
				}
			}
		} else if (xsg_conf_find_command("POP")) {
			for (i = 0; i < n; i++) {
				if (pop(rpn[i]->stack, "X")) {
					op[i].op = op_pop;
				} else if (pop(rpn[i]->stack, "N")) {
					op[i].op = op_pop;
				} else if (pop(rpn[i]->stack, "S")) {
					op[i].op = op_pop;
				} else {
					xsg_conf_error("RPN: POP: no element on the stack");
				}
			}
		} else if (xsg_conf_find_command("EXC")) {
			for (i = 0; i < n; i++) {
				// NOTE: order is important! "XX" needs to be first! then "XN", "XS", "NX", "SX"
				if (pop(rpn[i]->stack, "XX")) {
					op[i].op = op_exc;
					PUSH("XX");
				} else if (pop(rpn[i]->stack, "XN")) {
					op[i].op = op_exc;
					PUSH("NX");
				} else if (pop(rpn[i]->stack, "XS")) {
					op[i].op = op_exc;
					PUSH("SX");
				} else if (pop(rpn[i]->stack, "NX")) {
					op[i].op = op_exc;
					PUSH("XN");
				} else if (pop(rpn[i]->stack, "SX")) {
					op[i].op = op_exc;
					PUSH("XS");
				} else if (pop(rpn[i]->stack, "NS")) {
					op[i].op = op_exc;
					PUSH("SN");
				} else if (pop(rpn[i]->stack, "SN")) {
					op[i].op = op_exc;
					PUSH("NS");
				} else if (pop(rpn[i]->stack, "NN")) {
					op[i].op = op_exc_nn;
					PUSH("NN");
				} else if (pop(rpn[i]->stack, "SS")) {
					op[i].op = op_exc_ss;
					PUSH("SS");
				} else {
					xsg_conf_error("RPN: EXC: no two elements on the stack");
				}
			}
		} else if (xsg_conf_find_command("STRLEN")) {
			for (i = 0; i < n; i++) {
				POP("S", "STRLEN");
				op[i].op = op_strlen;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("STRCMP")) {
			for (i = 0; i < n; i++) {
				POP("SS", "STRCMP");
				op[i].op = op_strcmp;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("STRCASECMP")) {
			for (i = 0; i < n; i++) {
				POP("SS", "STRCASECMP");
				op[i].op = op_strcasecmp;
				PUSH("N");
			}
		} else if (xsg_conf_find_command("STRUP")) {
			for (i = 0; i < n; i++) {
				POP("S", "STRUP");
				op[i].op = op_strup;
				PUSH("S");
			}
		} else if (xsg_conf_find_command("STRDOWN")) {
			for (i = 0; i < n; i++) {
				POP("S", "STRDOWN");
				op[i].op = op_strdown;
				PUSH("S");
			}
		} else if (xsg_conf_find_command("STRREVERSE")) {
			for (i = 0; i < n; i++) {
				POP("S", "STRREVERSE");
				op[i].op = op_strreverse;
				PUSH("S");
			}
		} else {
			double (**num)(void *) = alloca(sizeof(void *) * n);
			char *(**str)(void *) = alloca(sizeof(void *) * n);
			void **arg = alloca(sizeof(void *) * n);

			for (i = 0; i < n; i++) {
				num[i] = NULL;
				str[i] = NULL;
				arg[i] = NULL;
			}

			if (!xsg_modules_parse(update, var, num, str, arg, n))
				xsg_conf_error("number, string, module name, LT, LE, GT, GE, EQ, NE, UN, ISINF, IF, MIN, MAX, "
						"LIMIT, UNKN, INF, NEGINF, ADD, SUB, MUL, DIV, MOD, SIN, COS, "
						"LOG, EXP, SQRT, ATAN, ATAN2, FLOOR, CEIL, DEG2RAD, RAD2DEG, "
						"ABS, DUP, POP, EXC, STRLEN, STRCMP, "
						"STRCASECMP, STRUP, STRDOWN or STRREVERSE expected");

			for (i = 0; i < n; i++) {
				op[i].num_func = num[i];
				op[i].str_func = str[i];
				op[i].arg = arg[i];

				if (num[i] != NULL && str[i] != NULL)
					PUSH("X");
				else if (num[i] != NULL)
					PUSH("N");
				else if (str[i] != NULL)
					PUSH("S");
			}
		}

		for (i = 0; i < n; i++) {
			if (rpn[i]->stack->len > max_stack_size) {
				size_t j;

				num_stack = xsg_renew(double, num_stack, rpn[i]->stack->len);
				str_stack = xsg_renew(xsg_string_t *, str_stack, rpn[i]->stack->len);

				for (j = max_stack_size; j < rpn[i]->stack->len; j++)
					str_stack[j] = xsg_string_new(NULL);

				max_stack_size = rpn[i]->stack->len;
			}
			rpn[i]->op_list = xsg_list_append(rpn[i]->op_list, op + i);
		}
	} while (xsg_conf_find_comma());

	for (i = 0; i < n; i++)
		if (rpn[i]->stack->len < 1)
			xsg_conf_error("RPN: no element on the stack");
}

/******************************************************************************/

static void calc(xsg_rpn_t *rpn) {
	xsg_list_t *l;

	stack_index = -1;

	for (l = rpn->op_list; l; l = l->next) {
		op_t *op = (op_t *) l->data;

		if (op->op) {
			op->op();
		} else {
			stack_index++;
			if (op->num_func)
				num_stack[stack_index] = op->num_func(op->arg);
			if (op->str_func)
				str_stack[stack_index] = xsg_string_assign(str_stack[stack_index], op->str_func(op->arg));
		}
	}
}

double xsg_rpn_get_num(xsg_rpn_t *rpn) {
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

char *xsg_rpn_get_str(xsg_rpn_t *rpn) {
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


