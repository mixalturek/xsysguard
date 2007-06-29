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
#include <alloca.h>

#include "rpn.h"
#include "var.h"
#include "conf.h"
#include "modules.h"

/******************************************************************************/

typedef struct _op_t op_t;

/******************************************************************************/

struct _op_t {
	void (*op)(void);
	double (*num_func)(void *arg);
	char *(*str_func)(void *arg);
	void *arg;
};

struct _xsg_rpn_t {
	xsg_list_t *op_list;
	unsigned num_stack_size;
	unsigned str_stack_size;
};

/******************************************************************************/

static xsg_list_t *rpn_list = NULL;

static double *num_stack = NULL;
static double *num_stptr = NULL;
static xsg_string_t **str_stack = NULL;
static xsg_string_t **str_stptr = NULL;

static unsigned max_num_stack_size = 0;
static unsigned max_str_stack_size = 0;

/******************************************************************************/

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

/******************************************************************************/

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

static void op_strcasecmp(void) {
	num_stptr[+1] = (double) strcasecmp(str_stptr[-1]->str, str_stptr[0]->str);
	num_stptr += 1;
	str_stptr -= 2;
}

static void op_strup(void) {
	char *s = str_stptr[0]->str;

	while (*s) {
		if (islower(*s))
			*s = toupper(*s);
		s++;
	}
}

static void op_strdown(void) {
	char *s = str_stptr[0]->str;

	while (*s) {
		if (isupper(*s))
			*s = tolower(*s);
		s++;
	}
}

static void op_strreverse(void) {
	char *h, *t;

	h = str_stptr[0]->str;
	t = h + str_stptr[0]->len - 1;

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

static void op_strinsert(void) {
	ssize_t pos = num_stptr[-1];
	ssize_t len = num_stptr[0];

	str_stptr[-1] = xsg_string_insert_len(str_stptr[-1], pos, str_stptr[0]->str, len);

	num_stptr -= 2;
	str_stptr -= 1;
}

/******************************************************************************/

static void op_dump(void) {
	int num_stack_size, str_stack_size, i;
	num_stack_size = (num_stptr + 1 - num_stack);
	str_stack_size = (str_stptr + 1 - str_stack);
	xsg_message("RPN: STACKPOINTER:  NUMBER=%02u/%02u  STRING=%02u/%02u",
			num_stack_size, max_num_stack_size, str_stack_size, max_str_stack_size);
	for (i = 0; i < num_stack_size; i++)
		xsg_message("RPN: NUMBER[%i]: % 12.6f  %e", i, num_stack[i], num_stack[i]);
	for (i = 0; i < str_stack_size; i++)
		xsg_message("RPN: STRING[%i]: \"%s\"", i, str_stack[i]->str);
}

/******************************************************************************/

void xsg_rpn_init(void) {
	build_stacks();
}

/******************************************************************************/

#define CHECK_NUM_STACK_SIZE(command, size) \
	if (rpn[i]->num_stack_size < size) xsg_error("RPN: %s: number stack size smaller than %s", command, #size)
#define CHECK_STR_STACK_SIZE(command, size) \
	if (rpn[i]->str_stack_size < size) xsg_error("RPN: %s: string stack size smaller than %s", command, #size)

void xsg_rpn_parse(uint64_t update, xsg_var_t *const *var, xsg_rpn_t **rpn, uint32_t n) {
	xsg_rpn_t *mem;
	uint32_t i;

	mem = xsg_new(xsg_rpn_t, n);

	for (i = 0; i < n; i++)
		rpn[i] = mem + i;

	for (i = 0; i < n; i++) {
		rpn[i]->op_list = NULL;
		rpn[i]->num_stack_size = 0;
		rpn[i]->str_stack_size = 0;

		rpn_list = xsg_list_append(rpn_list, rpn[i]);
	}

	do {
		op_t *op;

		op = xsg_new0(op_t, n);

		if (xsg_conf_find_command("LT")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("LT", 2);
				op[i].op = op_lt;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("LE")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("LE", 2);
				op[i].op = op_le;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("GT")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("GT", 2);
				op[i].op = op_gt;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("GE")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("GE", 2);
				op[i].op = op_ge;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("EQ")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("EQ", 2);
				op[i].op = op_eq;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("NE")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("NE", 2);
				op[i].op = op_ne;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("UN")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("UN", 1);
				op[i].op = op_un;
			}
		} else if (xsg_conf_find_command("ISINF")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("ISINF", 1);
				op[i].op = op_isinf;
			}
		} else if (xsg_conf_find_command("IF")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("IF", 3);
				op[i].op = op_if;
				rpn[i]->num_stack_size -= 2;
			}
		} else if (xsg_conf_find_command("MIN")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("MIN", 2);
				op[i].op = op_min;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("MAX")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("MAX", 2);
				op[i].op = op_max;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("LIMIT")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("LIMIT", 3);
				op[i].op = op_limit;
				rpn[i]->num_stack_size -= 2;
			}
		} else if (xsg_conf_find_command("UNKN")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_unkn;
				rpn[i]->num_stack_size += 1;
			}
		} else if (xsg_conf_find_command("INF")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_inf;
				rpn[i]->num_stack_size += 1;
			}
		} else if (xsg_conf_find_command("NEGINF")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_neginf;
				rpn[i]->num_stack_size += 1;
			}
		} else if (xsg_conf_find_command("ADD")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("ADD", 2);
				op[i].op = op_add;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("SUB")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("SUB", 2);
				op[i].op = op_sub;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("MUL")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("MUL", 2);
				op[i].op = op_mul;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("DIV")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("DIV", 2);
				op[i].op = op_div;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("MOD")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("MOD", 2);
				op[i].op = op_mod;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("SIN")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("SIN", 1);
				op[i].op = op_sin;
			}
		} else if (xsg_conf_find_command("COS")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("COS", 1);
				op[i].op = op_cos;
			}
		} else if (xsg_conf_find_command("LOG")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("LOG", 1);
				op[i].op = op_log;
			}
		} else if (xsg_conf_find_command("EXP")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("EXP", 1);
				op[i].op = op_exp;
			}
		} else if (xsg_conf_find_command("SQRT")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("SQRT", 1);
				op[i].op = op_sqrt;
			}
		} else if (xsg_conf_find_command("ATAN")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("ATAN", 1);
				op[i].op = op_atan;
			}
		} else if (xsg_conf_find_command("ATAN2")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("ATAN2", 2);
				op[i].op = op_atan2;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("FLOOR")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("FLOOR", 1);
				op[i].op = op_floor;
			}
		} else if (xsg_conf_find_command("CEIL")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("CEIL", 1);
				op[i].op = op_ceil;
			}
		} else if (xsg_conf_find_command("DEG2RAD")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("DEG2RAD", 1);
				op[i].op = op_deg2rad;
			}
		} else if (xsg_conf_find_command("RAD2DEG")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("RAD2DEG", 1);
				op[i].op = op_rad2deg;
			}
		} else if (xsg_conf_find_command("ABS")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("ABS", 1);
				op[i].op = op_abs;
			}
		} else if (xsg_conf_find_command("DUP")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("DUP", 1);
				op[i].op = op_dup;
				rpn[i]->num_stack_size += 1;
			}
		} else if (xsg_conf_find_command("POP")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("POP", 1);
				op[i].op = op_pop;
				rpn[i]->num_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("EXC")) {
			for (i = 0; i < n; i++) {
				CHECK_NUM_STACK_SIZE("EXC", 2);
				op[i].op = op_exc;
			}
		} else if (xsg_conf_find_command("STRDUP")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRDUP", 1);
				op[i].op = op_strdup;
				rpn[i]->str_stack_size += 1;
			}
		} else if (xsg_conf_find_command("STRPOP")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRPOP", 1);
				op[i].op = op_strpop;
				rpn[i]->str_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("STREXC")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STREXC", 2);
				op[i].op = op_strexc;
			}
		} else if (xsg_conf_find_command("STRLEN")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRLEN", 1);
				op[i].op = op_strlen;
				rpn[i]->num_stack_size += 1;
				rpn[i]->str_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("STRCMP")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRCMP", 2);
				op[i].op = op_strcmp;
				rpn[i]->num_stack_size += 1;
				rpn[i]->str_stack_size -= 2;
			}
		} else if (xsg_conf_find_command("STRCASECMP")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRCASECMP", 2);
				op[i].op = op_strcasecmp;
				rpn[i]->num_stack_size += 1;
				rpn[i]->str_stack_size -= 2;
			}
		} else if (xsg_conf_find_command("STRUP")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRUP", 1);
				op[i].op = op_strup;
			}
		} else if (xsg_conf_find_command("STRDOWN")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRDOWN", 1);
				op[i].op = op_strdown;
			}
		} else if (xsg_conf_find_command("STRREVERSE")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRREVERSE", 1);
				op[i].op = op_strreverse;
			}
		} else if (xsg_conf_find_command("STRINSERT")) {
			for (i = 0; i < n; i++) {
				CHECK_STR_STACK_SIZE("STRINSERT", 2);
				CHECK_NUM_STACK_SIZE("STRINSERT", 2);
				op[i].op = op_strinsert;
				rpn[i]->num_stack_size -= 2;
				rpn[i]->str_stack_size -= 1;
			}
		} else if (xsg_conf_find_command("DUMP")) {
			for (i = 0; i < n; i++) {
				op[i].op = op_dump;
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
				xsg_conf_error("module name, LT, LE, GT, GE, EQ, NE, UN, ISINF, IF, MIN, MAX, "
						"LIMIT, UNKN, INF, NEGINF, ADD, SUB, MUL, DIV, MOD, SIN, COS, "
						"LOG, EXP, SQRT, ATAN, ATAN2, FLOOR, CEIL, DEG2RAD, RAD2DEG, "
						"ABS, DUP, POP, EXC, STRDUP, STRPOP, STREXC, STRLEN, STRCMP, "
						"STRCASECMP, STRUP, STRDOWN, STRREVERSE, STRINSERT or DUMP");

			for (i = 0; i < n; i++) {
				op[i].num_func = num[i];
				op[i].str_func = str[i];
				op[i].arg = arg[i];

				if (num[i] != NULL)
					rpn[i]->num_stack_size += 1;
				if (str[i] != NULL)
					rpn[i]->str_stack_size += 1;
			}
		}

		for (i = 0; i < n; i++) {
			max_num_stack_size = MAX(max_num_stack_size, rpn[i]->num_stack_size);
			max_str_stack_size = MAX(max_str_stack_size, rpn[i]->str_stack_size);

			rpn[i]->op_list = xsg_list_append(rpn[i]->op_list, op + i);
		}

	} while (xsg_conf_find_comma());
}

static void calc(xsg_rpn_t *rpn) {
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

double xsg_rpn_get_num(xsg_rpn_t *rpn) {
	double num;

	if (unlikely(rpn->num_stack_size < 1)) {
		xsg_warning("RPN: no number left on stack");
		return DNAN;
	}

	calc(rpn);

	num = num_stptr[0];

	return num;
}

char *xsg_rpn_get_str(xsg_rpn_t *rpn) {
	char *str;

	if (unlikely(rpn->str_stack_size < 1)) {
		xsg_warning("RPN: no string left on stack");
		return NULL;
	}

	calc(rpn);

	str = str_stptr[0]->str;

	return str;
}


