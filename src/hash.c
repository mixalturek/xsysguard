/* hash.c
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
#include <string.h>

/******************************************************************************/

#define HASH_TABLE_MIN_SIZE 11
#define HASH_TABLE_MAX_SIZE 13845163

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/******************************************************************************/

typedef struct _xsg_hash_node_t xsg_hash_node_t;

struct _xsg_hash_node_t {
	void *key;
	void *value;
	xsg_hash_node_t *next;
};

struct _xsg_hash_table_t {
	int size;
	int nnodes;
	xsg_hash_node_t **nodes;
	unsigned (*hash_func)(const void *key);
	int (*key_equal_func)(const void *a, const void *b);
	volatile unsigned ref_count;
	void (*key_destroy_func)(void *data);
	void (*value_destroy_func)(void *data);
};

/******************************************************************************/

bool xsg_direct_equal(const void *v1, const void *v2) {
	return v1 == v2;
}

unsigned xsg_direct_hash(const void *v) {
	return (unsigned) v;
}

/******************************************************************************/

bool xsg_int_equal(const void *v1, const void *v2) {
	return *((const int*) v1) == *((const int *) v2);
}

unsigned xsg_int_hash(const void *v) {
	return *(const int *)v;
}

/******************************************************************************/

bool xsg_str_equal(const void * v1, const void *v2) {
	const char *string1 = v1;
	const char *string2 = v2;

	return strcmp(string1, string2) == 0;
}

unsigned xsg_str_hash(const void *v) {
	const signed char *p = v;
	uint32_t h = *p;

	if (h)
		for (p += 1; *p != '\0'; p++)
			h = (h << 5) - h + *p;

	return h;
}

/******************************************************************************/

xsg_hash_table_t *xsg_hash_table_new_full(unsigned (*hash_func)(const void *key),
					int (*key_equal_func)(const void *a, const void *b),
					void (*key_destroy_func)(void *data),
					void (*value_destroy_func)(void *data)) {
	xsg_hash_table_t *hash_table;

	hash_table = xsg_new(xsg_hash_table_t, 1);
	hash_table->size = HASH_TABLE_MIN_SIZE;
	hash_table->nnodes = 0;
	hash_table->hash_func = hash_func ? hash_func : xsg_direct_hash;
	hash_table->key_equal_func = key_equal_func;
	hash_table->ref_count = 1;
	hash_table->key_destroy_func = key_destroy_func;
	hash_table->value_destroy_func = value_destroy_func;
	hash_table->nodes = xsg_new0(xsg_hash_node_t*, hash_table->size);

	return hash_table;
}

xsg_hash_table_t *xsg_hash_table_new(unsigned (*hash_func)(const void *key),
					int (*key_equal_func)(const void *a, const void *b)) {
	return xsg_hash_table_new_full(hash_func, key_equal_func, NULL, NULL);
}

/******************************************************************************/

static xsg_hash_node_t *xsg_hash_node_new(void *key, void *value) {
	xsg_hash_node_t *hash_node = xsg_new(xsg_hash_node_t, 1);

	hash_node->key = key;
	hash_node->value = value;
	hash_node->next = NULL;

	return hash_node;
}

static void xsg_hash_node_destroy(xsg_hash_node_t *hash_node,
					void (*key_destroy_func)(void *data),
					void (*value_destroy_func)(void *data)) {
	if (key_destroy_func)
		key_destroy_func(hash_node->key);
	if (value_destroy_func)
		value_destroy_func(hash_node->value);
	xsg_free(hash_node);
}

static void xsg_hash_nodes_destroy(xsg_hash_node_t *hash_node,
					void (*key_destroy_func)(void *data),
					void (*value_destroy_func)(void *data)) {
	while (hash_node) {
		xsg_hash_node_t *next = hash_node->next;

		if (key_destroy_func)
			key_destroy_func(hash_node->key);
		if (value_destroy_func)
			value_destroy_func(hash_node->value);
		xsg_free(hash_node);
		hash_node = next;
	}
}

/******************************************************************************/

static unsigned xsg_spaced_primes_closest(unsigned num) {
	int i;

	static const unsigned primes[] = { 11, 19, 37, 73, 109, 163, 251, 367, 557, 823, 1237, 1861, 2777,
		4177, 6247, 9371, 14057, 21089, 31627, 47431, 71143, 106721, 160073, 240101, 360163,
		540217, 810343, 1215497, 1823231, 2734867, 4102283, 6153409, 9230113, 13845163 };

	for (i = 0; i < (sizeof(primes) / sizeof(primes[0])); i++)
		if (primes[i] > num)
			return primes[i];
	return primes[(sizeof(primes) / sizeof(primes[0])) - 1];
}


static void xsg_hash_table_resize(xsg_hash_table_t *hash_table) {
	if ((hash_table->size >= 3 * hash_table->nnodes && hash_table->size > HASH_TABLE_MIN_SIZE) ||
			(3 * hash_table->size <= hash_table->nnodes && hash_table->size < HASH_TABLE_MAX_SIZE)) {
		xsg_hash_node_t **new_nodes;
		xsg_hash_node_t *node;
		xsg_hash_node_t *next;
		unsigned hash_val;
		int new_size;
		int i;

		new_size = xsg_spaced_primes_closest(hash_table->nnodes);
		new_size = CLAMP(new_size, HASH_TABLE_MIN_SIZE, HASH_TABLE_MAX_SIZE);

		new_nodes = xsg_new0(xsg_hash_node_t *, new_size);

		for (i = 0; i < hash_table->size; i++) {
			for (node = hash_table->nodes[i]; node; node = next) {
				next = node->next;

				hash_val = (*hash_table->hash_func)(node->key) % new_size;

				node->next = new_nodes[hash_val];
				new_nodes[hash_val] = node;
			}
		}
		xsg_free(hash_table->nodes);
		hash_table->nodes = new_nodes;
		hash_table->size = new_size;
	}
}

/******************************************************************************/

xsg_hash_table_t *xsg_hash_table_ref(xsg_hash_table_t *hash_table) {
	if (unlikely(hash_table == NULL))
		return NULL;
	if (unlikely(hash_table->ref_count <= 0))
		return hash_table;

	hash_table->ref_count++;
	return hash_table;
}

void xsg_hash_table_unref(xsg_hash_table_t *hash_table) {
	if (unlikely(hash_table == NULL))
		return;
	if (unlikely(hash_table->ref_count <= 0))
		return;

	hash_table->ref_count--;

	if (hash_table->ref_count == 0) {
		int i;

		for (i = 0; i < hash_table->size; i++)
			xsg_hash_nodes_destroy(hash_table->nodes[i], hash_table->key_destroy_func, hash_table->value_destroy_func);
		xsg_free(hash_table->nodes);
		xsg_free(hash_table);
	}
}

/******************************************************************************/

static xsg_hash_node_t **xsg_hash_table_lookup_node(xsg_hash_table_t *hash_table, const void *key) {
	xsg_hash_node_t **node;

	node = &hash_table->nodes[(*hash_table->hash_func)(key) % hash_table->size];

	if (hash_table->key_equal_func)
		while (*node && !(*hash_table->key_equal_func)((*node)->key, key))
			node = &(*node)->next;
	else
		while (*node && (*node)->key != key)
			node = &(*node)->next;
	return node;
}

void *xsg_hash_table_lookup(xsg_hash_table_t *hash_table, const void *key) {
	xsg_hash_node_t *node;

	if (unlikely(hash_table == NULL))
		return NULL;

	node = *xsg_hash_table_lookup_node(hash_table, key);

	return node ? node->value : NULL;
}

bool xsg_hash_table_lookup_extended(xsg_hash_table_t *hash_table, const void *lookup_key, void **orig_key, void **value) {
	xsg_hash_node_t *node;

	if (unlikely(hash_table == NULL))
		return FALSE;

	node = *xsg_hash_table_lookup_node(hash_table, lookup_key);

	if (node) {
		if (orig_key)
			*orig_key = node->key;
		if (value)
			*value = node->value;
		return TRUE;
	} else {
		return FALSE;
	}
}

/******************************************************************************/

void xsg_hash_table_insert(xsg_hash_table_t *hash_table, void *key, void *value) {
	xsg_hash_node_t **node;

	if (unlikely(hash_table == NULL))
		return;

	if (unlikely(hash_table->ref_count <= 0))
		return;

	node = xsg_hash_table_lookup_node(hash_table, key);

	if (*node) {
		if (hash_table->key_destroy_func)
			hash_table->key_destroy_func(key);
		if (hash_table->value_destroy_func)
			hash_table->value_destroy_func(key);
		(*node)->value = value;
	} else {
		*node = xsg_hash_node_new(key, value);
		hash_table->nnodes++;
		xsg_hash_table_resize(hash_table);
	}
}

/******************************************************************************/

bool xsg_hash_table_remove(xsg_hash_table_t *hash_table, const void *key) {
	xsg_hash_node_t **node, *dest;

	if (unlikely(hash_table == NULL))
		return FALSE;

	node = xsg_hash_table_lookup_node(hash_table, key);

	if (*node) {
		dest = *node;
		(*node) = dest->next;
		xsg_hash_node_destroy(dest, hash_table->key_destroy_func, hash_table->value_destroy_func);
		hash_table->nnodes--;
		xsg_hash_table_resize(hash_table);
		return TRUE;
	}
	return FALSE;
}

void xsg_hash_table_remove_all(xsg_hash_table_t *hash_table) {
	unsigned i;

	if (unlikely(hash_table == NULL))
		return;

	for (i = 0; i < hash_table->size; i++) {
		xsg_hash_nodes_destroy(hash_table->nodes[i], hash_table->key_destroy_func, hash_table->value_destroy_func);
		hash_table->nodes[i] = NULL;
	}
	hash_table->nnodes = 0;
	xsg_hash_table_resize(hash_table);
}

void xsg_hash_table_destroy(xsg_hash_table_t *hash_table) {
	if (unlikely(hash_table == NULL))
		return;
	if (unlikely(hash_table->ref_count <= 0))
		return;

	xsg_hash_table_remove_all(hash_table);
	xsg_hash_table_unref(hash_table);
}


