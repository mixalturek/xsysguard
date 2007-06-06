/* random.c
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
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>

/******************************************************************************/

/* NOTE: functions are c&p from glib/grand.c */

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

static void rand_set_seed_array(rand_t *rand, const uint32_t *seed, unsigned int seed_length) {
	int i, j, k;

	if (rand == NULL)
		return;

	if (seed_length < 1)
		return;

	i = 1;
	j = 0;
	k = (N > seed_length ? N : seed_length);
	for (; k; k--) {
		rand->mt[i] = (rand->mt[i] ^ ((rand->mt[i-1] ^ (rand->mt[i-1] >> 30)) * 1664525UL)) + seed[j] + j;
		rand->mt[i] &= 0xffffffffUL;
		i++;
		j++;
		if (i >= N) {
			rand->mt[0] = rand->mt[N-1];
			i = 1;
		}
		if (j >= seed_length)
			j = 0;
	}
	for (k = N-1; k; k--) {
		rand->mt[i] = (rand->mt[i] ^ ((rand->mt[i-1] ^ (rand->mt[i-1] >> 30)) * 1566083941UL)) - i;
		rand->mt[i] &= 0xffffffffUL;
		i++;
		if (i >= N) {
			rand->mt[0] = rand->mt[N-1];
			i = 1;
		}
	}
	rand->mt[0] = 0x80000000UL;
}

static rand_t *rand_new_with_seed_array(const uint32_t *seed, unsigned int seed_length) {
	rand_t *rand = xsg_new0(rand_t, 1);

	rand_set_seed_array(rand, seed, seed_length);
	return rand;
}

static rand_t *rand_new() {
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

			if (r != 1)
				dev_urandom_exists = FALSE;

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

static uint32_t rand_int(rand_t *rand) {
	uint32_t y;
	static const uint32_t mag01[2] = { 0x0, MATRIX_A };

	if (rand == NULL)
		return 0;

	if (rand->mti >= N) {
		int kk;

		for (kk = 0; kk < N-M; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		for (; kk < N-1; kk++) {
			y = (rand->mt[kk] & UPPER_MASK) | (rand->mt[kk+1] & LOWER_MASK);
			rand->mt[kk] = rand->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
		}
		y = (rand->mt[N-1] & UPPER_MASK) | (rand->mt[0] & LOWER_MASK);
		rand->mt[N-1] = rand->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];
		rand->mti = 0;
	}
	y = rand->mt[rand->mti++];
	y ^= TEMPERING_SHIFT_U(y);
	y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
	y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
	y ^= TEMPERING_SHIFT_L(y);

	return y;
}

static double rand_double(rand_t *rand) {
	double retval = rand_int(rand) * RAND_DOUBLE_TRANSFORM;
	retval = (retval + rand_int(rand)) * RAND_DOUBLE_TRANSFORM;

	if (retval >= 1.0)
		return rand_double(rand);

	return retval;
}

/******************************************************************************/

static double get_random(void *arg) {
	double d;

	if (!global_random)
		global_random = rand_new();

	d = rand_double(global_random);

	xsg_debug("get_random: %f", d);

	return d;
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
	*n = get_random;
}

char *info() {
	return "random number generator";
}

