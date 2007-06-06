/* daemon.c
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
#include <errno.h>
#include <endian.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>

/******************************************************************************/

#define LAST_ALIVE_TIMEOUT 8
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

typedef struct _daemon_t {
	char *name;
	char *command;

	pid_t pid;

	enum {
		RUNNING,	// daemon is running, everything is fine
		KILL,		// daemon is running, but an error occurred, next tick: send SIGTERM
		SEND_SIGTERM,	// daemon is running, already send SIGTERM, next tick: send SIGKILL
		SEND_SIGKILL,	// daemon is running, already send SIGKILL, wait with waitpid()
		NOTRUNNING	// daemon is NOT running, waitpid was ok, next tick: fork and exec
	} state;

	xsg_main_poll_t stdin_poll;
	xsg_main_poll_t stdout_poll;
	xsg_main_poll_t stderr_poll;

	uint64_t last_alive_tick;

	uint32_t id_buffer; // buffer for next daemon_var_id
	ssize_t id_buffer_fill;

	uint8_t log_level_buffer; // buffer for next log_level
	bool log_level_buffer_filled;
	xsg_string_t *log_buffer; // buffer for next log message

	char *write_buffer; // configuration send to daemon
	size_t write_buffer_len;
	ssize_t write_buffer_done;
	ssize_t write_buffer_todo;

	xsg_list_t *var_list;
} daemon_t;

typedef struct _daemon_var_t {
	daemon_t *daemon;
	xsg_var_t *var;
	uint64_t update;
	char *config;

	enum {
		END = 0x00,
		NUM = 0x01,
		STR = 0x02
	} type;

	xsg_string_t *str;
	xsg_string_t *new_str;

	double num;
	double new_num;
	unsigned new_num_fill;
} daemon_var_t;

/******************************************************************************/

static xsg_list_t *daemon_list = NULL;

static uint32_t daemon_var_count = 0;
static xsg_list_t *daemon_var_list = NULL;
static daemon_var_t **daemon_var_array = NULL;

static uint64_t last_alive_timeout = LAST_ALIVE_TIMEOUT;
static size_t max_buf_len = MAX_BUF_LEN;

/******************************************************************************/

static daemon_var_t *get_daemon_var(uint32_t daemon_var_id) {
	if (unlikely(daemon_var_array == NULL)) {
		daemon_var_t *daemon_var;

		xsg_debug("daemon_var_array is NULL, using daemon_var_list...");
		daemon_var = xsg_list_nth_data(daemon_var_list, daemon_var_id);
		if (unlikely(daemon_var == NULL))
			xsg_warning("invalid daemon_var_id: %"PRIu32, daemon_var_id);
		return daemon_var;
	}
	if (unlikely(daemon_var_id >= daemon_var_count)) {
		xsg_warning("invalid daemon_var_id: %"PRIu32, daemon_var_id);
		return NULL;
	}

	return daemon_var_array[daemon_var_id];
}

static void build_daemon_var_array(void) {
	xsg_list_t *l;
	uint32_t daemon_var_id = 0;

	daemon_var_array = xsg_new(daemon_var_t *, daemon_var_count);

	for (l = daemon_var_list; l; l = l->next) {
		daemon_var_array[daemon_var_id] = l->data;
		daemon_var_id++;
	}
}

/******************************************************************************/

static int sclose(int fd) {
	if (fd < 0) {
		xsg_warning("Trying to close fd %d", fd);
		return 0;
	} else {
		int n;

		do {
			n = close(fd);
		} while (unlikely(n == -1) && (errno == EINTR));

		if (unlikely(n == -1)) {
			int err = errno;

			xsg_debug("close for fd %d failed: %s", fd, strerror(errno));
			errno = err;
		}
		return n;
	}
}

/******************************************************************************/

static void stdin_daemon(void *arg, xsg_main_poll_events_t events);
static void stdout_daemon(void *arg, xsg_main_poll_events_t events);
static void stderr_daemon(void *arg, xsg_main_poll_events_t events);

/******************************************************************************/

static void kill_daemon(daemon_t *daemon) {
	int n;

	xsg_message("[%d]%s: Killing daemon \"%s\"", (int) daemon->pid, daemon->name, daemon->command);

	if (unlikely(daemon == NULL))
		return;

	if (daemon->state == RUNNING)
		daemon->state = KILL;
	else
		return;

	n = sclose(daemon->stdin_poll.fd);
	if (unlikely(n == -1))
		xsg_warning("[%d]%s: close(stdin) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));

	n = sclose(daemon->stdout_poll.fd);
	if (unlikely(n == -1))
		xsg_warning("[%d]%s: close(stdout) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));

	n = sclose(daemon->stderr_poll.fd);
	if (unlikely(n == -1))
		xsg_warning("[%d]%s: close(stderr) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));

	xsg_main_remove_poll(&daemon->stdin_poll);
	xsg_main_remove_poll(&daemon->stdout_poll);
	xsg_main_remove_poll(&daemon->stderr_poll);

	daemon->stdin_poll.fd = -1;
	daemon->stdout_poll.fd = -1;
	daemon->stderr_poll.fd = -1;
}

/******************************************************************************/

static daemon_t *find_daemon(char *name) {
	xsg_list_t *l;
	daemon_t *daemon;
	char *command;

	for (l = daemon_list; l; l = l->next) {
		daemon = l->data;

		if (strcmp(daemon->name, name) == 0)
			return daemon;
	}

	daemon = xsg_new(daemon_t, 1);
	daemon_list = xsg_list_append(daemon_list, daemon);

	daemon->name = xsg_strdup(name);

	command = getenv(name);
	if (command != NULL)
		daemon->command = xsg_strdup(command);
	else
		daemon->command = name;

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

	return daemon;;
}

/******************************************************************************/

static void build_daemon_write_buffers(void) {
	char *init = "\0\nxsysguard\n";
	uint64_t interval = xsg_uint64_be(xsg_main_get_interval());
	uint8_t log_level = xsg_log_level;
	xsg_list_t *l;
	uint32_t id = 0;

	for (l = daemon_var_list; l; l = l->next) {
		daemon_var_t *daemon_var = l->data;
		daemon_t *daemon = daemon_var->daemon;
		size_t header_len, config_len;
		uint32_t id_be;
		uint64_t update_be;
		uint8_t type;
		uint32_t config_len_be;

		if (daemon->write_buffer_len == 0) {
			size_t init_len = sizeof(init) + sizeof(uint64_t) + sizeof(uint8_t);

			daemon->write_buffer = xsg_realloc(daemon->write_buffer, init_len);

			// init
			memcpy(daemon->write_buffer + daemon->write_buffer_len, init, sizeof(init));
			daemon->write_buffer_len += sizeof(init);

			// interval
			memcpy(daemon->write_buffer + daemon->write_buffer_len, &interval, sizeof(uint64_t));
			daemon->write_buffer_len += sizeof(uint64_t);

			// log level
			memcpy(daemon->write_buffer + daemon->write_buffer_len, &log_level, sizeof(uint8_t));
			daemon->write_buffer_len += sizeof(uint8_t);
		}

		header_len = sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t);
		config_len = strlen(daemon_var->config);

		daemon->write_buffer = xsg_realloc(daemon->write_buffer, daemon->write_buffer_len + header_len + config_len);

		// type
		type = daemon_var->type;
		memcpy(daemon->write_buffer + daemon->write_buffer_len, &type, sizeof(uint8_t));
		daemon->write_buffer_len += sizeof(uint8_t);

		// id
		id_be = xsg_uint32_be(id);
		memcpy(daemon->write_buffer + daemon->write_buffer_len, &id_be, sizeof(uint32_t));
		daemon->write_buffer_len += sizeof(uint32_t);

		// update
		update_be = xsg_uint64_be(daemon_var->update);
		memcpy(daemon->write_buffer + daemon->write_buffer_len, &update_be, sizeof(uint64_t));
		daemon->write_buffer_len += sizeof(uint64_t);

		// config_len
		config_len_be = xsg_uint32_be(config_len);
		memcpy(daemon->write_buffer + daemon->write_buffer_len, &config_len_be, sizeof(uint32_t));
		daemon->write_buffer_len += sizeof(uint32_t);

		// config
		memcpy(daemon->write_buffer + daemon->write_buffer_len, daemon_var->config, config_len);
		daemon->write_buffer_len += config_len;

		id++;
	}

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;
		uint8_t end = END;

		daemon->write_buffer = xsg_realloc(daemon->write_buffer, daemon->write_buffer_len + sizeof(uint8_t));

		memcpy(daemon->write_buffer + daemon->write_buffer_len, &end, sizeof(uint8_t));
		daemon->write_buffer_len += sizeof(uint8_t);
	}
}

/******************************************************************************/

static void stdin_daemon(void *arg, xsg_main_poll_events_t events) {
	daemon_t *daemon = (daemon_t *) arg;
	char *buffer;
	ssize_t n;

	buffer = daemon->write_buffer + daemon->write_buffer_done;

	n = write(daemon->stdin_poll.fd, buffer, daemon->write_buffer_todo);

	if (daemon->state != RUNNING) {
		xsg_warning("%s: write(stdin) failed: daemon is not running", daemon->name);
		return;
	}

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: write(stdin) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: write(stdin) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		kill_daemon(daemon);
		return;
	}

	daemon->write_buffer_done += n;
	daemon->write_buffer_todo -= n;

	if (daemon->write_buffer_todo < 1)
		xsg_main_remove_poll(&daemon->stdin_poll);
}


static void stdout_daemon(void *arg, xsg_main_poll_events_t events) {
	daemon_t *daemon = (daemon_t *) arg;
	char buffer_array[BUFFER_SIZE];
	char *buffer = buffer_array;
	ssize_t n;

	n = read(daemon->stdout_poll.fd, buffer, BUFFER_SIZE - 1);
#if 0
	if (unlikely(xsg_log_level >= XSG_LOG_LEVEL_DEBUG)) {
		char *hex = alloca(n * 3 + 1);
		ssize_t i;

		for (i = 0; i < n; i++)
			sprintf(hex + i * 3, "%02x ", buffer[i] & 0xff);

		xsg_debug("%s: received: %s", daemon->name, hex);
	}
#endif
	if (daemon->state != RUNNING) {
		return;
	}

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: read(stdout) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: read(stdout) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		kill_daemon(daemon);
		return;
	}

	if (unlikely(n == 0)) {
		xsg_warning("[%d]%s: read(stdout) returned EOF", (int) daemon->pid, daemon->name);
		kill_daemon(daemon);
		return;
	}

	daemon->last_alive_tick = xsg_main_get_tick();

	while (n != 0) {

		while (daemon->id_buffer_fill < sizeof(uint32_t)) {
#			if __BYTE_ORDER == __LITTLE_ENDIAN
				((char *)(&daemon->id_buffer))[sizeof(uint32_t) - 1 - daemon->id_buffer_fill] = buffer[0];
#			elif __BYTE_ORDER == __BIG_ENDIAN
				((char *)(&daemon->id_buffer))[daemon->id_buffer_fill] = buffer[0];
#			else
#				error UNKNOWN BYTEORDER
#			endif
			daemon->id_buffer_fill++;
			buffer++;
			n--;
			if (unlikely(n == 0))
				return;
		}

		if (daemon->id_buffer == 0xffffffff) { // LOG MESSAGE
			if (unlikely(!daemon->log_level_buffer_filled)) {
				((char *)(&daemon->log_level_buffer))[0] = buffer[0];
				daemon->log_level_buffer_filled = TRUE;
				buffer++;
				n--;
				if (unlikely(n == 0))
					return;
			}

			if (daemon->log_level_buffer > 0) {
				while (buffer[0] != '\0') {
					if (daemon->log_buffer->len < max_buf_len)
						daemon->log_buffer = xsg_string_append_c(daemon->log_buffer, buffer[0]);
					buffer++;
					n--;
					if (unlikely(n == 0))
						return;
				}

				// the '\0'
				buffer++;
				n--;

				if (xsg_log_level >= daemon->log_level_buffer)
					xsg_log(XSG_LOG_DOMAIN, daemon->log_level_buffer, "[%d]%s: Received log message: %s",
							(int) daemon->pid, daemon->name, daemon->log_buffer->str);

				daemon->log_buffer = xsg_string_truncate(daemon->log_buffer, 0);
			}
			daemon->log_level_buffer_filled = FALSE;

			daemon->id_buffer_fill = 0;
		} else {
			daemon_var_t *daemon_var;

			daemon_var = get_daemon_var(daemon->id_buffer);

			if (unlikely(daemon_var == NULL) || unlikely(daemon_var->daemon != daemon)) {
				xsg_warning("[%d]%s: Received invalid id: %"PRIu32, (int) daemon->pid, daemon->name, daemon->id_buffer);
				kill_daemon(daemon);
				return;
			}

			if (daemon_var->type == NUM) {
				while (daemon_var->new_num_fill < sizeof(double)) {
#					if __BYTE_ORDER == __LITTLE_ENDIAN
						((char *)(&daemon_var->new_num))[sizeof(double) - 1 - daemon_var->new_num_fill] = buffer[0];
#					elif __BYTE_ORDER == __BIG_ENDIAN
						((char *)(&daemon_var->new_num))[daemon_var->new_num_fill] = buffer[0];
#					else
#						error UNKNOWN BYTEORDER
#					endif
					daemon_var->new_num_fill++;
					buffer++;
					n--;
					if (unlikely(n == 0))
						break;
				}

				if (daemon_var->new_num_fill == sizeof(double)) {
					daemon_var->num = daemon_var->new_num;

					xsg_debug("[%d]%s: Received number for id %"PRIu32": %f", (int) daemon->pid, daemon->name,
							daemon->id_buffer, daemon_var->num);

					daemon_var->new_num_fill = 0;

					xsg_var_dirty(daemon_var->var);

					daemon->id_buffer_fill = 0;
				}
			} else if (daemon_var->type == STR) {
				xsg_string_t *tmp;

				while (buffer[0] != '\0') {
					if (daemon_var->new_str->len < max_buf_len)
						daemon_var->new_str = xsg_string_append_c(daemon_var->new_str, buffer[0]);
					buffer++;
					n--;
					if (unlikely(n == 0))
						return;
				}

				// the '\0'
				buffer++;
				n--;

				tmp = daemon_var->new_str;
				daemon_var->new_str = daemon_var->str;
				daemon_var->str = tmp;

				xsg_debug("[%d]%s: received string for id %"PRIu32": \"%s\"", (int) daemon->pid, daemon->name,
						daemon->id_buffer, daemon_var->str->str);

				daemon_var->new_str = xsg_string_truncate(daemon_var->new_str, 0);

				xsg_var_dirty(daemon_var->var);

				daemon->id_buffer_fill = 0;
			} else {
				xsg_warning("invalid daemon var type");
			}
		}
	}
}

static void stderr_daemon(void *arg, xsg_main_poll_events_t events) {
	daemon_t *daemon = (daemon_t *) arg;
	char buffer[BUFFER_SIZE];
	ssize_t n;

	n = read(daemon->stderr_poll.fd, buffer, BUFFER_SIZE - 1);

	if (unlikely(n == -1) && errno == EINTR) {
		xsg_debug("[%d]%s: read(stderr) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		return;
	}

	if (unlikely(n == -1)) {
		xsg_warning("[%d]%s: read(stderr) failed: %s", (int) daemon->pid, daemon->name, strerror(errno));
		kill_daemon(daemon);
		return;
	}

	if (unlikely(n == 0)) {
		xsg_warning("[%d]%s: read(stderr) returned EOF", (int) daemon->pid, daemon->name);
		kill_daemon(daemon);
		return;
	}

	buffer[n] = '\0';

	xsg_warning("[%d]%s: Received message on stderr: \"%s\"", (int) daemon->pid, daemon->name, buffer);
}

/******************************************************************************/

static void exec_daemon(daemon_t *daemon) {
	int pipe1[2]; // daemon's stdin
	int pipe2[2]; // daemon's stdout
	int pipe3[2]; // daemon's stderr
	pid_t pid;
	xsg_list_t *l;

	xsg_message("%s: Executing \"%s\"", daemon->name, daemon->command);

	if (pipe(pipe1) < 0) {
		xsg_warning("%s: Cannot create pipe: %s", daemon->name, strerror(errno));
		return;
	}

	if (pipe(pipe2) < 0) {
		xsg_warning("%s: Cannot create pipe: %s", daemon->name, strerror(errno));
		sclose(pipe1[0]);
		sclose(pipe1[1]);
		return;
	}

	if (pipe(pipe3) < 0) {
		xsg_warning("%s: Cannot create pipe: %s", daemon->name, strerror(errno));
		sclose(pipe1[0]);
		sclose(pipe1[1]);
		sclose(pipe2[0]);
		sclose(pipe2[1]);
		return;
	}

	pid = fork();

	if (pid < 0) {
		xsg_warning("%s: Cannot fork: %s", daemon->name, strerror(errno));
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
			if (dup2(pipe1[0], STDIN_FILENO) != STDIN_FILENO)
				exit(EXIT_FAILURE);
			sclose(pipe1[0]);
		}
		if (pipe2[1] != STDOUT_FILENO) {
			if (dup2(pipe2[1], STDOUT_FILENO) != STDOUT_FILENO)
				exit(EXIT_FAILURE);
			sclose(pipe2[1]);
		}
		if (pipe3[1] != STDERR_FILENO) {
			if (dup2(pipe3[1], STDERR_FILENO) != STDERR_FILENO)
				exit(EXIT_FAILURE);
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

	xsg_message("[%u]%s: Running...", daemon->pid, daemon->name);

	daemon->stdin_poll.fd = pipe1[1];
	daemon->stdout_poll.fd = pipe2[0];
	daemon->stderr_poll.fd = pipe3[0];

	xsg_main_add_poll(&daemon->stdin_poll);
	xsg_main_add_poll(&daemon->stdout_poll);
	xsg_main_add_poll(&daemon->stderr_poll);

	daemon->id_buffer = 0;
	daemon->id_buffer_fill = 0;
	daemon->log_level_buffer = 0;
	daemon->log_level_buffer_filled = FALSE;
	daemon->log_buffer = xsg_string_truncate(daemon->log_buffer, 0);
	daemon->write_buffer_done = 0;
	daemon->write_buffer_todo = daemon->write_buffer_len;

	for (l = daemon->var_list; l; l = l->next) {
		daemon_var_t *daemon_var = l->data;

		daemon_var->str = xsg_string_truncate(daemon_var->str, 0);
		daemon_var->num = DNAN;

		daemon_var->new_str = xsg_string_truncate(daemon_var->new_str, 0);
		daemon_var->new_num = DNAN;
		daemon_var->new_num_fill = 0;
	}
}

/******************************************************************************/

static double get_num(void *arg) {
	daemon_var_t *daemon_var = (daemon_var_t *) arg;
	double num;

	num = daemon_var->num;

	xsg_debug("%s: get_number for \"%s\": %f", daemon_var->daemon->name, daemon_var->config, num);

	return num;
}

static char *get_str(void *arg) {
	daemon_var_t *daemon_var = (daemon_var_t *) arg;
	char *str;

	str = daemon_var->str->str;

	xsg_debug("%s: get_strint for \"%s\": \"%s\"", daemon_var->daemon->name, daemon_var->config, str);

	return str;
}

/******************************************************************************/

static void init_daemons(void) {
	build_daemon_var_array();
	build_daemon_write_buffers();
}

static void shutdown_daemons(void) {
	xsg_list_t *l;

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;

		kill_daemon(daemon);

		if (daemon->pid > 0) {
			kill(daemon->pid, 15); // SIGTERM
			waitpid(daemon->pid, NULL, 0);
		}
	}
}

static void update_daemons(uint64_t tick) {
	xsg_list_t *l;

	for (l = daemon_list; l; l = l->next) {
		daemon_t *daemon = l->data;

		if ((tick > daemon->last_alive_tick) && (tick - daemon->last_alive_tick) > last_alive_timeout)
			kill_daemon(daemon); // sets state = KILL

		if (daemon->state != RUNNING && daemon->pid > 0) {
			int status;

			xsg_debug("[%d]%s: Calling waitpid...", (int) daemon->pid, daemon->name);

			if (waitpid(daemon->pid, &status, WNOHANG) > 0) {
				if (WIFEXITED(status) || WIFSIGNALED(status)) {
					daemon->state = NOTRUNNING;
					daemon->pid = 0;
				}
			}
		}

		switch (daemon->state) {
			case KILL:
				kill(daemon->pid, 15); // SIGTERM
				daemon->state = SEND_SIGTERM;
				break;
			case SEND_SIGTERM:
				kill(daemon->pid, 9); // SIGKILL
				daemon->state = SEND_SIGKILL;
				break;
			case SEND_SIGKILL:
				xsg_warning("[%d]%s: Send SIGKILL to process, but waitpid failed", (int) daemon->pid, daemon->name);
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

void parse(uint64_t update, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
	static bool first_time = TRUE;
	daemon_t *daemon;
	daemon_var_t *daemon_var;
	char *name;

	if (unlikely(first_time)) {
		char *timeout, *len;

		timeout = getenv("XSYSGUARD_DAEMON_TIMEOUT");

		if (timeout != NULL)
			last_alive_timeout = atoll(timeout);

		len = getenv("XSYSGUARD_DAEMON_MAXBUFLEN");

		if (len != NULL)
			max_buf_len = atoll(len);

		xsg_main_add_init_func(init_daemons);
		xsg_main_add_shutdown_func(shutdown_daemons);
		xsg_main_add_update_func(update_daemons);

		first_time = FALSE;
	}

	daemon_var = xsg_new(daemon_var_t, 1);

	name = xsg_conf_read_string();
	daemon = find_daemon(name);
	xsg_free(name);

	daemon_var->daemon = daemon;


	if (xsg_conf_find_command("n")) {
		daemon_var->type = NUM;
		daemon_var->num = DNAN;
		daemon_var->new_num = DNAN;
		*n = get_num;
	} else if (xsg_conf_find_command("s")) {
		daemon_var->type = STR;
		daemon_var->str = xsg_string_new(NULL);
		daemon_var->new_str = xsg_string_new(NULL);
		*s = get_str;
	} else {
		xsg_conf_error("n or s expected");
	}

	daemon_var->var = var;
	daemon_var->update = update;
	daemon_var->config = xsg_conf_read_string();

	*arg = (void *) daemon_var;

	daemon->var_list = xsg_list_append(daemon->var_list, daemon_var);
	daemon_var_list = xsg_list_append(daemon_var_list, daemon_var);
	daemon_var_count++;
}

char *info() {
	return "executes daemon processes";
}

