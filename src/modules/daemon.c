/* daemon.c
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
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <alloca.h>

/******************************************************************************/

#define LAST_ALIVE_TIMEOUT 16
#define MAX_BUF_LEN 262144
#define BUFFER_SIZE 4096

/******************************************************************************/

#ifndef STDIN_FILENO
# define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
# define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
# define STDERR_FILENO 2
#endif

/******************************************************************************/

typedef enum _type_t {
	END = 0,
	NUM = 1,
	STR = 2
} type_t;

typedef struct _daemon_t {
	char *command;

	pid_t pid;

	enum {
		RUNNING,	/* daemon is running, everything is fine */
		KILL,		/* daemon is running, but an error occurred,
				 * next tick: send SIGTERM */
		SEND_SIGTERM,	/* daemon is running, already send SIGTERM,
				 * next tick: send SIGKILL */
		SEND_SIGKILL,	/* daemon is running, already send SIGKILL,
				 * wait with waitpid() */
		NOTRUNNING	/* daemon is NOT running, waitpid was ok,
				 * next tick: fork and exec */
	} state;

	xsg_main_poll_t stdin_poll;
	xsg_main_poll_t stdout_poll;
	xsg_main_poll_t stderr_poll;

	uint64_t last_alive_tick;

	size_t init;

	uint32_t id_buffer; /* buffer for next daemon_var_id */
	ssize_t id_buffer_fill;

	uint8_t log_level_buffer; /* buffer for next log_level */
	bool log_level_buffer_filled;
	xsg_string_t *log_buffer; /* buffer for next log message */

	char *write_buffer; /* configuration send to daemon */
	size_t write_buffer_len;
	ssize_t write_buffer_done;
	ssize_t write_buffer_todo;

	bool send_alive;

	xsg_list_t *var_list;
} daemon_t;

typedef struct _daemon_var_t {
	daemon_t *daemon;
	xsg_var_t *var;

	type_t type;

	xsg_string_t *str;
	xsg_string_t *new_str;

	double num;
	double new_num;
	unsigned new_num_fill;
} daemon_var_t;

/******************************************************************************/

static const char *magic_init = "\nxsysguardd_init_version_1\n";

static xsg_list_t *daemon_list = NULL;

static uint32_t daemon_var_count = 0;
static xsg_list_t *daemon_var_list = NULL;
static daemon_var_t **daemon_var_array = NULL;

static uint64_t last_alive_timeout = LAST_ALIVE_TIMEOUT;
static size_t max_buf_len = MAX_BUF_LEN;

/******************************************************************************/

static daemon_var_t *
get_daemon_var(uint32_t daemon_var_id)
{
	if (unlikely(daemon_var_array == NULL)) {
		daemon_var_t *daemon_var;

		xsg_debug("daemon_var_array is NULL, using daemon_var_list...");
		daemon_var = xsg_list_nth_data(daemon_var_list, daemon_var_id);
		if (unlikely(daemon_var == NULL)) {
			xsg_warning("invalid daemon_var_id: %"PRIu32,
					daemon_var_id);
		}
		return daemon_var;
	}
	if (unlikely(daemon_var_id >= daemon_var_count)) {
		xsg_warning("invalid daemon_var_id: %"PRIu32, daemon_var_id);
		return NULL;
	}

	return daemon_var_array[daemon_var_id];
}

static void
build_daemon_var_array(void)
{
	xsg_list_t *l;
	uint32_t daemon_var_id = 0;

	daemon_var_array = xsg_new(daemon_var_t *, daemon_var_count);

	for (l = daemon_var_list; l; l = l->next) {
		daemon_var_array[daemon_var_id] = l->data;
		daemon_var_id++;
	}
}

/******************************************************************************/

static bool
am_big_endian(void)
{
	long one = 1;
	return !(*((char *)(&one)));
}

/******************************************************************************/

static int
sclose(int fd)
{
	if (fd < 0) {
		xsg_warning("trying to close fd %d", fd);
		return 0;
	} else {
		int n;

		do {
			n = close(fd);
		} while (unlikely(n == -1) && (errno == EINTR));

		if (unlikely(n == -1)) {
			int err = errno;

			xsg_debug("close for fd %d failed: %s", fd,
					strerror(errno));
			errno = err;
		}
		return n;
	}
}

/******************************************************************************/

static void
stdin_daemon(void *arg, xsg_main_poll_events_t events);
static void
stdout_daemon(void *arg, xsg_main_poll_events_t events);
static void
stderr_daemon(void *arg, xsg_main_poll_events_t events);

/******************************************************************************/

static void
kill_daemon(daemon_t *daemon)
{
	int n;

	xsg_message("[%d]%s: killing daemon...", (int) daemon->pid,
			daemon->command);

	if (unlikely(daemon == NULL)) {
		return;
	}

	if (daemon->state == RUNNING) {
		daemon->state = KILL;
	} else {
		return;
	}

	n = sclose(daemon->stdin_poll.fd);
	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: close(stdin) failed: %s",
				(int) daemon->pid, daemon->command,
				strerror(errno));
	}

	n = sclose(daemon->stdout_poll.fd);
	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: close(stdout) failed: %s",
				(int) daemon->pid, daemon->command,
				strerror(errno));
	}

	n = sclose(daemon->stderr_poll.fd);
	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: close(stderr) failed: %s",
				(int) daemon->pid, daemon->command,
				strerror(errno));
	}

	xsg_main_remove_poll(&daemon->stdin_poll);
	xsg_main_remove_poll(&daemon->stdout_poll);
	xsg_main_remove_poll(&daemon->stderr_poll);

	daemon->stdin_poll.fd = -1;
	daemon->stdout_poll.fd = -1;
	daemon->stderr_poll.fd = -1;
}

/******************************************************************************/

static daemon_t *
find_daemon(char *command)
{
	xsg_list_t *l;
	daemon_t *daemon;

	for (l = daemon_list; l; l = l->next) {
		daemon = l->data;

		if (strcmp(daemon->command, command) == 0) {
			return daemon;
		}
	}

	daemon = xsg_new(daemon_t, 1);
	daemon_list = xsg_list_append(daemon_list, daemon);

	daemon->command = xsg_strdup(command);

	daemon->pid = 0;
	daemon->state = NOTRUNNING;

	daemon->stdin_poll.fd = -1;
	daemon->stdin_poll.events = XSG_MAIN_POLL_WRITE;
	daemon->stdin_poll.func = stdin_daemon;
	daemon->stdin_poll.arg = daemon;

	daemon->stdout_poll.fd = -1;
	daemon->stdout_poll.events = XSG_MAIN_POLL_READ;
	daemon->stdout_poll.func = stdout_daemon;
	daemon->stdout_poll.arg = daemon;

	daemon->stderr_poll.fd = -1;
	daemon->stderr_poll.events = XSG_MAIN_POLL_READ;
	daemon->stderr_poll.func = stderr_daemon;
	daemon->stderr_poll.arg = daemon;

	daemon->last_alive_tick = 0;

	daemon->log_buffer = xsg_string_new(NULL);

	daemon->write_buffer = NULL;
	daemon->write_buffer_len = 0;
	daemon->write_buffer_done = 0;
	daemon->write_buffer_todo = 0;

	daemon->send_alive = FALSE;

	daemon->var_list = NULL;

	return daemon;
}

/******************************************************************************/

static void
daemon_write_buffer_add_var(
	daemon_t *daemon,
	uint64_t update,
	type_t t,
	char *config
)
{
	size_t header_len, config_len;
	uint32_t id_be;
	uint8_t type;
	uint32_t config_len_be;

	if (daemon->write_buffer_len == 0) {
		uint64_t interval;
		uint8_t log_level;
		uint64_t timeout;

		interval = xsg_uint64_be(xsg_main_get_interval());
		log_level = xsg_log_level;
		timeout = xsg_uint64_be(last_alive_timeout);

		size_t init_len = sizeof(magic_init) + sizeof(uint64_t)
				+ sizeof(uint8_t) + sizeof(uint64_t);

		daemon->write_buffer = xsg_realloc(daemon->write_buffer,
				init_len);

		/* init */
		memcpy(daemon->write_buffer + daemon->write_buffer_len,
				magic_init, sizeof(magic_init));
		daemon->write_buffer_len += sizeof(magic_init);

		/* interval */
		memcpy(daemon->write_buffer + daemon->write_buffer_len,
				&interval, sizeof(uint64_t));
		daemon->write_buffer_len += sizeof(uint64_t);

		/* log level */
		memcpy(daemon->write_buffer + daemon->write_buffer_len,
				&log_level, sizeof(uint8_t));
		daemon->write_buffer_len += sizeof(uint8_t);

		/* last alive timeout */
		memcpy(daemon->write_buffer + daemon->write_buffer_len,
				&timeout, sizeof(uint64_t));
		daemon->write_buffer_len += sizeof(uint64_t);
	}

	header_len = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)
			+ sizeof(uint64_t) + sizeof(uint32_t);
	config_len = strlen(config);

	daemon->write_buffer = xsg_realloc(daemon->write_buffer,
			daemon->write_buffer_len + header_len + config_len);

	/* type */
	type = t;
	memcpy(daemon->write_buffer + daemon->write_buffer_len, &type,
			sizeof(uint8_t));
	daemon->write_buffer_len += sizeof(uint8_t);

	/* id */
	id_be = xsg_uint32_be(daemon_var_count);
	memcpy(daemon->write_buffer + daemon->write_buffer_len, &id_be,
			sizeof(uint32_t));
	daemon->write_buffer_len += sizeof(uint32_t);

	/* update */
	update = xsg_uint64_be(update);
	memcpy(daemon->write_buffer + daemon->write_buffer_len, &update,
			sizeof(uint64_t));
	daemon->write_buffer_len += sizeof(uint64_t);

	/* config_len */
	config_len_be = xsg_uint32_be(config_len);
	memcpy(daemon->write_buffer + daemon->write_buffer_len, &config_len_be,
			sizeof(uint32_t));
	daemon->write_buffer_len += sizeof(uint32_t);

	/* config */
	memcpy(daemon->write_buffer + daemon->write_buffer_len, config,
			config_len);
	daemon->write_buffer_len += config_len;
}

/******************************************************************************/

static void
stdin_daemon(void *arg, xsg_main_poll_events_t events)
{
	daemon_t *daemon = (daemon_t *) arg;
	char *buffer;
	ssize_t n;

	if (daemon->write_buffer_todo < 1) {
		if (daemon->send_alive) {
			char end_buffer = END;

			n = write(daemon->stdin_poll.fd, &end_buffer, 1);

			daemon->send_alive = FALSE;
		}
		xsg_main_remove_poll(&daemon->stdin_poll);
		return;
	}

	buffer = daemon->write_buffer + daemon->write_buffer_done;

	n = write(daemon->stdin_poll.fd, buffer, daemon->write_buffer_todo);

	if (daemon->state != RUNNING) {
		xsg_warning("%s: write(stdin) failed: daemon is not running",
				daemon->command);
		return;
	}

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: write(stdin) failed: %s", (int) daemon->pid,
				daemon->command, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: write(stdin) failed: %s",
				(int) daemon->pid, daemon->command,
				strerror(errno));
		kill_daemon(daemon);
		return;
	}

	daemon->write_buffer_done += n;
	daemon->write_buffer_todo -= n;

	if (daemon->write_buffer_todo < 1) {
		xsg_main_remove_poll(&daemon->stdin_poll);
	}
}


static void
stdout_daemon(void *arg, xsg_main_poll_events_t events)
{
	daemon_t *daemon = (daemon_t *) arg;
	char buffer_array[BUFFER_SIZE];
	char *buffer = buffer_array;
	ssize_t n;

	n = read(daemon->stdout_poll.fd, buffer, BUFFER_SIZE - 1);

	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_DEBUG)) {
		char *hex = alloca(n * 3 + 1);
		ssize_t i;

		for (i = 0; i < n; i++) {
			sprintf(hex + i * 3, "%02x ", buffer[i] & 0xff);
		}

		xsg_debug("[%d]%s: received: %s", (int) daemon->pid,
				daemon->command, hex);
	}

	if (daemon->state != RUNNING) {
		return;
	}

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: read(stdout) failed: %s", (int) daemon->pid,
				daemon->command, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: read(stdout) failed: %s",
				(int) daemon->pid, daemon->command,
				strerror(errno));
		kill_daemon(daemon);
		return;
	}

	if (unlikely(n == 0)) {
		xsg_warning("[%d]%s: read(stdout) returned EOF",
				(int) daemon->pid, daemon->command);
		kill_daemon(daemon);
		return;
	}

	daemon->last_alive_tick = xsg_main_get_tick();

	while (n != 0) {
		while (daemon->init < sizeof(magic_init)) {
			if (magic_init[daemon->init] == *buffer) {
				daemon->init++;
			} else {
				daemon->init = 1; /* skip the first '\n' */
			}
			buffer++;
			n--;
			if (unlikely(n == 0)) {
				return;
			}
		}

		while (daemon->id_buffer_fill < sizeof(uint32_t)) {
			if (am_big_endian()) {
				unsigned int i = daemon->id_buffer_fill;
				((char *)(&daemon->id_buffer))[i] = buffer[0];
			} else {
				unsigned int i = sizeof(uint32_t) - 1
						- daemon->id_buffer_fill;
				((char *)(&daemon->id_buffer))[i] = buffer[0];
			}

			daemon->id_buffer_fill++;
			buffer++;
			n--;

			if (unlikely(n == 0)) {
				return;
			}
		}

		if (daemon->id_buffer == 0xffffffff) { /* LOG MESSAGE */
			if (unlikely(!daemon->log_level_buffer_filled)) {
				((char *)(&daemon->log_level_buffer))[0]
						= buffer[0];
				daemon->log_level_buffer_filled = TRUE;
				buffer++;
				n--;
				if (unlikely(n == 0)) {
					return;
				}
			}

			if (daemon->log_level_buffer > 0) {
				while (buffer[0] != '\0') {
					if (daemon->log_buffer->len
					  < max_buf_len) {
						xsg_string_append_c(
							daemon->log_buffer,
							buffer[0]);
					}
					buffer++;
					n--;
					if (unlikely(n == 0)) {
						return;
					}
				}

				/* the '\0' */
				buffer++;
				n--;

				if (xsg_log_level >= daemon->log_level_buffer) {
					xsg_log(XSG_LOG_DOMAIN,
						MAX(daemon->log_level_buffer,
						2), "[%d]%s: Received log "
						"message: %s",
						(int) daemon->pid, daemon->command,
						daemon->log_buffer->str);
				}

				xsg_string_truncate(daemon->log_buffer, 0);
			}
			daemon->log_level_buffer_filled = FALSE;

			daemon->id_buffer_fill = 0;
		} else {
			daemon_var_t *daemon_var;

			daemon_var = get_daemon_var(daemon->id_buffer);

			if (unlikely(daemon_var == NULL)
			 || unlikely(daemon_var->daemon != daemon)) {
				xsg_warning("[%d]%s: Received invalid "
						"id: %"PRIu32,
						(int) daemon->pid, daemon->command,
						daemon->id_buffer);
				kill_daemon(daemon);
				return;
			}

			if (daemon_var->type == NUM) {
				while (daemon_var->new_num_fill
				     < sizeof(double)) {
					if (am_big_endian()) {
						unsigned int i;
						i = daemon_var->new_num_fill;
						((char *)(&daemon_var->new_num))[i]
							= buffer[0];
					} else {
						unsigned int i;
						i = sizeof(double) - 1
							- daemon_var->new_num_fill;
						((char *)(&daemon_var->new_num))[i]
							= buffer[0];
					}

					daemon_var->new_num_fill++;
					buffer++;
					n--;
					if (unlikely(n == 0)) {
						break;
					}
				}

				if (daemon_var->new_num_fill == sizeof(double)) {
					daemon_var->num = daemon_var->new_num;

					xsg_debug("[%d]%s: Received number for "
							"id %"PRIu32": %f",
							(int) daemon->pid,
							daemon->command,
							daemon->id_buffer,
							daemon_var->num);

					daemon_var->new_num_fill = 0;

					xsg_var_dirty(daemon_var->var);

					daemon->id_buffer_fill = 0;
				}
			} else if (daemon_var->type == STR) {
				xsg_string_t *tmp;

				while (buffer[0] != '\0') {
					if (daemon_var->new_str->len
					  < max_buf_len) {
						xsg_string_append_c(
							daemon_var->new_str,
							buffer[0]);
					}
					buffer++;
					n--;
					if (unlikely(n == 0)) {
						return;
					}
				}

				/* the '\0' */
				buffer++;
				n--;

				tmp = daemon_var->new_str;
				daemon_var->new_str = daemon_var->str;
				daemon_var->str = tmp;

				xsg_debug("[%d]%s: received string for "
						"id %"PRIu32": \"%s\"",
						(int) daemon->pid, daemon->command,
						daemon->id_buffer,
						daemon_var->str->str);

				xsg_string_truncate(daemon_var->new_str, 0);

				xsg_var_dirty(daemon_var->var);

				daemon->id_buffer_fill = 0;
			} else {
				xsg_warning("invalid daemon var type");
			}
		}
	}
}

static void
stderr_daemon(void *arg, xsg_main_poll_events_t events)
{
	daemon_t *daemon = (daemon_t *) arg;
	char buffer[BUFFER_SIZE];
	ssize_t n;

	n = read(daemon->stderr_poll.fd, buffer, BUFFER_SIZE - 1);

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: read(stderr) failed: %s", (int) daemon->pid,
				daemon->command, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_debug("[%d]%s: read(stderr) failed: %s", (int) daemon->pid,
				daemon->command, strerror(errno));
		return;
	}

	if (unlikely(n == 0)) {
		xsg_debug("[%d]%s: read(stderr) returned EOF",
				(int) daemon->pid, daemon->command);
		return;
	}

	buffer[n] = '\0';

	xsg_warning("[%d]%s: received message on stderr: \"%s\"",
			(int) daemon->pid, daemon->command, buffer);
}

/******************************************************************************/

static void
exec_daemon(daemon_t *daemon)
{
	int pipe1[2]; /* daemon's stdin */
	int pipe2[2]; /* daemon's stdout */
	int pipe3[2]; /* daemon's stderr */
	pid_t pid;
	xsg_list_t *l;

	xsg_message("%s: executing...", daemon->command);

	if (pipe(pipe1) < 0) {
		xsg_warning("%s: cannot create pipe: %s", daemon->command,
				strerror(errno));
		return;
	}

	if (pipe(pipe2) < 0) {
		xsg_warning("%s: cannot create pipe: %s", daemon->command,
				strerror(errno));
		sclose(pipe1[0]);
		sclose(pipe1[1]);
		return;
	}

	if (pipe(pipe3) < 0) {
		xsg_warning("%s: cannot create pipe: %s", daemon->command,
				strerror(errno));
		sclose(pipe1[0]);
		sclose(pipe1[1]);
		sclose(pipe2[0]);
		sclose(pipe2[1]);
		return;
	}

	pid = fork();

	if (pid < 0) {
		xsg_warning("%s: cannot fork: %s", daemon->command,
				strerror(errno));
		sclose(pipe1[0]);
		sclose(pipe1[1]);
		sclose(pipe2[0]);
		sclose(pipe2[1]);
		sclose(pipe3[0]);
		sclose(pipe3[1]);
		return;
	} else if (pid == 0) {
		sclose(pipe1[1]);
		sclose(pipe2[0]);
		sclose(pipe3[0]);
		if (pipe1[0] != STDIN_FILENO) {
			if (dup2(pipe1[0], STDIN_FILENO) != STDIN_FILENO) {
				exit(EXIT_FAILURE);
			}
			sclose(pipe1[0]);
		}
		if (pipe2[1] != STDOUT_FILENO) {
			if (dup2(pipe2[1], STDOUT_FILENO) != STDOUT_FILENO) {
				exit(EXIT_FAILURE);
			}
			sclose(pipe2[1]);
		}
		if (pipe3[1] != STDERR_FILENO) {
			if (dup2(pipe3[1], STDERR_FILENO) != STDERR_FILENO) {
				exit(EXIT_FAILURE);
			}
			sclose(pipe3[1]);
		}
		execl("/bin/sh", "sh", "-c", daemon->command, NULL);
		exit(EXIT_FAILURE);
	}

	sclose(pipe1[0]);
	sclose(pipe2[1]);
	sclose(pipe3[1]);

	daemon->state = RUNNING;
	daemon->pid = pid;

	xsg_message("[%u]%s: running...", daemon->pid, daemon->command);

	daemon->stdin_poll.fd = pipe1[1];
	daemon->stdout_poll.fd = pipe2[0];
	daemon->stderr_poll.fd = pipe3[0];

	xsg_main_add_poll(&daemon->stdin_poll);
	xsg_main_add_poll(&daemon->stdout_poll);
	xsg_main_add_poll(&daemon->stderr_poll);

	xsg_set_cloexec_flag(pipe1[1], TRUE);
	xsg_set_cloexec_flag(pipe2[0], TRUE);
	xsg_set_cloexec_flag(pipe3[0], TRUE);

	daemon->init = 1; /* skip the first '\n' */
	daemon->id_buffer = 0;
	daemon->id_buffer_fill = 0;
	daemon->log_level_buffer = 0;
	daemon->log_level_buffer_filled = FALSE;
	xsg_string_truncate(daemon->log_buffer, 0);
	daemon->write_buffer_done = 0;
	daemon->write_buffer_todo = daemon->write_buffer_len;

	for (l = daemon->var_list; l; l = l->next) {
		daemon_var_t *daemon_var = l->data;

		if (daemon_var->type == NUM) {
			daemon_var->num = DNAN;
			daemon_var->new_num = DNAN;
			daemon_var->new_num_fill = 0;
		} else {
			xsg_string_truncate(daemon_var->str, 0);
			xsg_string_truncate(daemon_var->new_str, 0);
		}
	}
}

/******************************************************************************/

static double
get_num(void *arg)
{
	daemon_var_t *daemon_var = (daemon_var_t *) arg;
	double num;

	num = daemon_var->num;

	xsg_debug("[%d]%s: get_number: %f", (int) daemon_var->daemon->pid,
			daemon_var->daemon->command, num);

	return num;
}

static const char *
get_str(void *arg)
{
	daemon_var_t *daemon_var = (daemon_var_t *) arg;
	char *str;

	str = daemon_var->str->str;

	xsg_debug("[%d]%s: get_string: \"%s\"", (int) daemon_var->daemon->pid,
			daemon_var->daemon->command, str);

	return str;
}

/******************************************************************************/

static void
init_daemons(void)
{
	char *timeout, *len;
	xsg_list_t *l;

	timeout = xsg_getenv("XSYSGUARD_DAEMON_TIMEOUT");

	if (timeout != NULL) {
		last_alive_timeout = atoll(timeout);
	}

	len = xsg_getenv("XSYSGUARD_DAEMON_MAXBUFLEN");

	if (len != NULL) {
		max_buf_len = atoll(len);
	}

	build_daemon_var_array();

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;
		uint8_t end = END;

		daemon->write_buffer = xsg_realloc(daemon->write_buffer,
				daemon->write_buffer_len + sizeof(uint8_t));

		memcpy(daemon->write_buffer + daemon->write_buffer_len, &end,
				sizeof(uint8_t));
		daemon->write_buffer_len += sizeof(uint8_t);
	}
}

static void
shutdown_daemons(void)
{
	xsg_list_t *l;

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;

		kill_daemon(daemon);

		if (daemon->pid > 0) {
			kill(daemon->pid, 15); /* SIGTERM */
			waitpid(daemon->pid, NULL, 0);
		}
	}
}

static void
update_daemons(uint64_t tick)
{
	xsg_list_t *l;

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;

		daemon->send_alive = TRUE;
		xsg_main_add_poll(&daemon->stdin_poll);

		if ((tick > daemon->last_alive_tick)
		 && (tick - daemon->last_alive_tick) > last_alive_timeout) {
			kill_daemon(daemon); /* sets state = KILL */
		}

		if (daemon->state != RUNNING && daemon->pid > 0) {
			int status;

			xsg_debug("[%d]%s: calling waitpid...",
					(int) daemon->pid, daemon->command);

			if (waitpid(daemon->pid, &status, WNOHANG) > 0) {
				if (WIFEXITED(status) || WIFSIGNALED(status)) {
					daemon->state = NOTRUNNING;
					daemon->pid = 0;
				}
			}
		}

		switch (daemon->state) {
		case KILL:
			kill(daemon->pid, 15); /* SIGTERM */
			daemon->state = SEND_SIGTERM;
			break;
		case SEND_SIGTERM:
			kill(daemon->pid, 9); /* SIGKILL */
			daemon->state = SEND_SIGKILL;
			break;
		case SEND_SIGKILL:
			xsg_warning("[%d]%s: send SIGKILL to process, but "
					"waitpid failed", (int) daemon->pid,
					daemon->command);
			break;
		case NOTRUNNING:
			exec_daemon(daemon);
			break;
		default:
			break;
		}
	}
}

/******************************************************************************/

static void
parse_daemon(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	daemon_t *daemon;
	daemon_var_t *daemon_var;
	char *command;
	type_t type = 0;

	xsg_main_add_init_func(init_daemons);
	xsg_main_add_shutdown_func(shutdown_daemons);
	xsg_main_add_update_func(update_daemons);

	daemon_var = xsg_new(daemon_var_t, 1);

	command = xsg_conf_read_string();
	daemon = find_daemon(command);
	xsg_free(command);

	if (xsg_conf_find_command("n")) {
		type = NUM;
	} else if (xsg_conf_find_command("num")) {
		type = NUM;
	} else if (xsg_conf_find_command("number")) {
		type = NUM;
	} else if (xsg_conf_find_command("s")) {
		type = STR;
	} else if (xsg_conf_find_command("str")) {
		type = STR;
	} else if (xsg_conf_find_command("string")) {
		type = STR;
	} else {
		xsg_conf_error("n, num, number, s, str or string expected");
	}

	daemon_var->daemon = daemon;
	daemon_var->var = var;
	daemon_var->type = type;

	if (type == NUM) {
		daemon_var->num = DNAN;
		daemon_var->new_num = DNAN;
		*num = get_num;
	} else {
		daemon_var->str = xsg_string_new(NULL);
		daemon_var->new_str = xsg_string_new(NULL);
		*str = get_str;
	}

	*arg = daemon_var;

	daemon_write_buffer_add_var(daemon, update, type,
			xsg_conf_read_string());

	daemon->var_list = xsg_list_append(daemon->var_list, daemon_var);
	daemon_var_list = xsg_list_append(daemon_var_list, daemon_var);
	daemon_var_count++;
}

static const char *
help_daemon(void)
{
	static char *string = NULL;

	if (string == NULL) {
		xsg_asprintf(&string, "N %s:<command>:num:<variable>\n"
					"S %s:<command>:str:<variable>\n",
					XSG_MODULE_NAME, XSG_MODULE_NAME);
	}

	return string;
}

/******************************************************************************/

XSG_MODULE(parse_daemon, help_daemon, "execute xsysguardd processes");

