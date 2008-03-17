/* random.c
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

/*
 * This code is based on code from GLIB released under the GNU Lesser
 * General Public License.
 *
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * ftp://ftp.gtk.org/pub/gtk/
 *
 */

#include <xsysguard.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

/******************************************************************************/

#define N 624
#define M 397
#define MATRIX_A 0x9908b0df
#define UPPER_MASK 0x80000000
#define LOWER_MASK 0x7fffffff

#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

#define RAND_DOUBLE_TRANSFORM 2.3283064365386962890625e-10

typedef struct _rand_t rand_t;

struct _rand_t {
	uint32_t mt[N];
	unsigned int mti;
};

static rand_t *global_random = NULL;

/******************************************************************************/

static void
rand_set_seed_array(
	rand_t *r,
	const uint32_t *seed,
	unsigned int seed_length
)
{
	int i, j, k;

	if (r == NULL) {
		return;
	}

	if (seed_length < 1) {
		return;
	}

	i = 1;
	j = 0;
	k = (N > seed_length ? N : seed_length);
	for (; k; k--) {
		r->mt[i] = (r->mt[i] ^ ((r->mt[i-1]
			^ (r->mt[i-1] >> 30)) * 1664525UL)) + seed[j] + j;
		r->mt[i] &= 0xffffffffUL;
		i++;
		j++;
		if (i >= N) {
			r->mt[0] = r->mt[N-1];
			i = 1;
		}
		if (j >= seed_length)
			j = 0;
	}
	for (k = N-1; k; k--) {
		r->mt[i] = (r->mt[i] ^ ((r->mt[i-1]
				^ (r->mt[i-1] >> 30)) * 1566083941UL)) - i;
		r->mt[i] &= 0xffffffffUL;
		i++;
		if (i >= N) {
			r->mt[0] = r->mt[N-1];
			i = 1;
		}
	}
	r->mt[0] = 0x80000000UL;
}

static rand_t *
rand_new_with_seed_array(const uint32_t *seed, unsigned int seed_length)
{
	rand_t *r = xsg_new0(rand_t, 1);

	rand_set_seed_array(r, seed, seed_length);

	return r;
}

static rand_t *
rand_new(void)
{
	uint32_t seed[4];
	struct timeval now;
	static bool dev_urandom_exists = TRUE;

	if (dev_urandom_exists) {
		FILE *dev_urandom;

		do {
			errno = 0;
			dev_urandom = fopen("/dev/urandom", "rb");
		} while (errno == EINTR);

		if (dev_urandom) {
			int r;

			do {
				errno = 0;
				r = fread(seed, sizeof(seed), 1, dev_urandom);
			} while (errno == EINTR);

			if (r != 1) {
				dev_urandom_exists = FALSE;
			}

			fclose(dev_urandom);
		} else {
			dev_urandom_exists = FALSE;
		}
	}

	if (!dev_urandom_exists) {
		gettimeofday(&now, NULL);
		seed[0] = now.tv_sec;
		seed[1] = now.tv_usec;
		seed[2] = getpid();
		seed[3] = getppid();
	}

	return rand_new_with_seed_array(seed, 4);
}

static uint32_t
rand_int(rand_t *r)
{
	uint32_t y;
	static const uint32_t mag01[2] = { 0x0, MATRIX_A };

	if (r == NULL) {
		return 0;
	}

	if (r->mti >= N) {
		int kk;

		for (kk = 0; kk < N-M; kk++) {
			y = (r->mt[kk] & UPPER_MASK)
				| (r->mt[kk+1] & LOWER_MASK);
			r->mt[kk] = r->mt[kk+M] ^ (y >> 1)
				^ mag01[y & 0x1];
		}
		for (; kk < N-1; kk++) {
			y = (r->mt[kk] & UPPER_MASK)
				| (r->mt[kk+1] & LOWER_MASK);
			r->mt[kk] = r->mt[kk+(M-N)]
				^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (r->mt[N-1] & UPPER_MASK) | (r->mt[0] & LOWER_MASK);
		r->mt[N-1] = r->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
		r->mti = 0;
	}
	y = r->mt[r->mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y;
}

static double
rand_double(rand_t *r)
{
	double retval = rand_int(r) * RAND_DOUBLE_TRANSFORM;
	retval = (retval + rand_int(r)) * RAND_DOUBLE_TRANSFORM;

	if (retval >= 1.0) {
		return rand_double(r);
	}

	return retval;
}

/******************************************************************************/

typedef struct _random_t {
	uint64_t update;
	double value;
} random_t;

/******************************************************************************/

static xsg_list_t *random_list = NULL;

/******************************************************************************/

static void
init_random(void)
{
	global_random = rand_new();
}

static void
update_random(uint64_t tick)
{
	xsg_list_t *l;

	for (l = random_list; l; l = l->next) {
		random_t *random = l->data;

		if (tick == 0 || tick % random->update == 0) {
			random->value = rand_double(global_random);
		}
	}
}

/******************************************************************************/

static double
get_random(void *arg)
{
	random_t *random = arg;

	xsg_debug("get_random: %f", random->value);

	return random->value;
}

/******************************************************************************/

static void
parse_random(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	random_t *random;

	random = xsg_new(random_t, 1);
	random->update = update;
	random->value = DNAN;

	*arg = (void *) random;
	*num = get_random;

	random_list = xsg_list_append(random_list, random);

	xsg_main_add_update_func(update_random);
	xsg_main_add_init_func(init_random);
}

static const char *
help_random(void)
{
	static xsg_string_t *string = NULL;

	if (string != NULL) {
		return string->str;
	}

	string = xsg_string_new(NULL);

	xsg_string_append_printf(string, "N %s\n", XSG_MODULE_NAME);

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_random, help_random, "random number generator");

