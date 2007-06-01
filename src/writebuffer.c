/* writebuffer.c
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
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "writebuffer.h"
#include "vard.h"

/******************************************************************************/

#define MAX_LOG_BUFFER_LEN 262144

/******************************************************************************/

typedef struct _buffer_t {
	char *buf;
	size_t len;
	size_t done;
	size_t todo;
	size_t allocated_len;
} buffer_t;

/******************************************************************************/

static buffer_t buffer_array[2] = {
	{ NULL, 0, 0, 0, 0 },
	{ NULL, 0, 0, 0, 0 }
};

static buffer_t *write_buffer = buffer_array;
static buffer_t *send_buffer = buffer_array + 1;

/******************************************************************************/

static void buffer_writer(void *arg, xsg_main_poll_events_t events);

static xsg_main_poll_t poll = {
	fd: STDOUT_FILENO,
	events: XSG_MAIN_POLL_WRITE,
	func: buffer_writer,
	arg: NULL
};

/******************************************************************************/

static void buffer_writer(void *arg, xsg_main_poll_events_t events) {
	char *buffer;
	ssize_t n;

	xsg_debug("buffer_writer: flushing %u bytes", write_buffer->todo);

	buffer = write_buffer->buf + write_buffer->done;

	n = write(STDOUT_FILENO, buffer, write_buffer->todo);

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("buffer_writer: write failed: %s", strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		fprintf(stderr, "buffer_writer: write failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	write_buffer->done += n;
	write_buffer->todo -= n;

	if (write_buffer->todo < 1) {
		xsg_main_remove_poll(&poll);
		xsg_var_queue_vars();
	}
}

/******************************************************************************/

bool xsg_writebuffer_ready(void) {
	return (write_buffer->todo == 0);
}

/******************************************************************************/

void xsg_writebuffer_flush(void) {
	buffer_t *tmp;

	tmp = write_buffer;
	write_buffer = send_buffer;
	send_buffer = tmp;

	write_buffer->todo = write_buffer->len;
	write_buffer->done = 0;

	send_buffer-> len = 0;

	xsg_main_add_poll(&poll);
}

/******************************************************************************/

static size_t nearest_power(size_t base, size_t num) {
	if (num > ((size_t)-1) / 2) {
		return ((size_t)-1);
	} else {
		size_t n = base;
		while (n < num)
			n <<= 1;
		return n;
	}
}

static void buffer_maybe_expand(buffer_t *buffer, size_t len) {
	if (buffer->len + len >= buffer->allocated_len) {
		buffer->allocated_len = nearest_power(1, buffer->len + len);
		buffer->buf = xsg_realloc(buffer->buf, buffer->allocated_len);
	}
}

/******************************************************************************/

void xsg_writebuffer_queue_num(uint32_t id, double num) {

	buffer_maybe_expand(send_buffer, sizeof(uint32_t) + sizeof(double));

	id = xsg_uint32_be(id);
	memcpy(send_buffer->buf + send_buffer->len, &id, sizeof(uint32_t));
	send_buffer->len += sizeof(uint32_t);

	num = xsg_double_be(num);
	memcpy(send_buffer->buf + send_buffer->len, &num, sizeof(double));
	send_buffer->len += sizeof(double);
}

void xsg_writebuffer_queue_str(uint32_t id, xsg_string_t *str) {
	size_t len = str->len;

	buffer_maybe_expand(send_buffer, sizeof(uint32_t) + len + 1);

	id = xsg_uint32_be(id);
	memcpy(send_buffer->buf + send_buffer->len, &id, sizeof(uint32_t));
	send_buffer->len += sizeof(uint32_t);

	memcpy(send_buffer->buf + send_buffer->len, str->str, len + 1);
	send_buffer->len += len + 1;
}

/******************************************************************************/

void xsg_writebuffer_queue_alive(void) {
	uint8_t alive[] = { 0xff, 0xff, 0xff, 0xff, 0x00 };

	buffer_maybe_expand(send_buffer, sizeof(uint32_t) + sizeof(uint8_t));

	memcpy(send_buffer->buf + send_buffer->len, alive, sizeof(uint32_t) + sizeof(uint8_t));
	send_buffer->len += sizeof(uint32_t) + sizeof(uint8_t);
}

void xsg_writebuffer_queue_log(uint8_t level, const char *message, size_t len) {
	static bool already_running = FALSE; // we call xsg_realloc which may call us
	uint32_t id = 0xffffffff;

	if (unlikely(already_running))
		return;

	if (unlikely(send_buffer->len > MAX_LOG_BUFFER_LEN))
		return;

	already_running = TRUE;

	buffer_maybe_expand(send_buffer, sizeof(uint32_t) + sizeof(uint8_t) + len + 1);

	memcpy(send_buffer->buf + send_buffer->len, &id, sizeof(uint32_t));
	send_buffer->len += sizeof(uint32_t);

	memcpy(send_buffer->buf + send_buffer->len, &level, sizeof(uint8_t));
	send_buffer->len += sizeof(uint8_t);

	memcpy(send_buffer->buf + send_buffer->len, message, len + 1);
	send_buffer->len += len + 1;

	already_running = FALSE;
}

