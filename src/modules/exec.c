/* exec.c
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
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

/******************************************************************************/

#define BUFFER_SIZE 4096

/******************************************************************************/

typedef struct _exec_t {
	char *command;

	int updatec;
	uint64_t *updatev;

	pid_t pid;

	xsg_buffer_t *buffer;

	xsg_main_poll_t poll;
} exec_t;

/******************************************************************************/

static xsg_list_t *exec_list = NULL;

/******************************************************************************/

static void
kill_exec(exec_t *e)
{
	xsg_main_remove_poll(&e->poll);
	e->poll.fd = -1;

	if (!e->pid) {
		return;
	}

	xsg_message("[%d]%s: killing...", (int) e->pid,
			e->command);

	kill(e->pid, 15); /* SIGTERM */
}

/******************************************************************************/

static void
read_stdout_exec(void *arg, xsg_main_poll_events_t events)
{
	exec_t *e = (exec_t *) arg;
	char buffer[BUFFER_SIZE];
	ssize_t n;

	n = read(e->poll.fd, buffer, BUFFER_SIZE);

	if (!e->pid) {
		return;
	}

	if (n == -1 && errno == EINTR) {
		xsg_debug("[%d]%s: read(stdout) failed: %s", (int) e->pid,
				e->command, strerror(errno));
		return;
	}

	if (n == -1) {
		xsg_debug("[%d]%s: read(stdout) failed: %s", (int) e->pid,
				e->command, strerror(errno));
		kill_exec(e);
		return;
	}

	if (n == 0) {
		xsg_debug("[%d]%s: read(stdout) returned EOF", (int) e->pid,
				e->command);
		kill_exec(e);
		return;
	}

	xsg_buffer_add(e->buffer, buffer, n);
}

/******************************************************************************/

static xsg_buffer_t *
find_exec_buffer(const char *command, uint64_t update)
{
	xsg_list_t *l;
	exec_t *e;
	int i;

	for (l = exec_list; l; l = l->next) {
		e = l->data;

		if (!strcmp(e->command, command)) {
			for (i = 0; i < e->updatec; i++) {
				if ((update % e->updatev[i]) == 0) {
					return e->buffer;
				}
				if ((e->updatev[i] % update) == 0) {
					e->updatev[i] = update;
					return e->buffer;
				}
			}
			e->updatev = xsg_renew(uint64_t, e->updatev,
					e->updatec + 1);
			e->updatev[e->updatec] = update;
			e->updatec += 1;
			return e->buffer;
		}
	}

	e = xsg_new(exec_t, 1);

	exec_list = xsg_list_append(exec_list, e);

	e->command = xsg_strdup(command);

	e->updatev = xsg_new(uint64_t, 1);
	e->updatev[0] = update;
	e->updatec = 1;

	e->pid = 0;

	e->buffer = xsg_buffer_new();

	e->poll.fd = -1;
	e->poll.events = XSG_MAIN_POLL_READ;
	e->poll.func = read_stdout_exec;
	e->poll.arg = e;

	return e->buffer;
}

/******************************************************************************/

static void
catch_execs(void)
{
	xsg_list_t *l;
	exec_t *e;

	for (l = exec_list; l; l = l->next) {
		e = l->data;

		if (e->pid) {
			int status;
			int ret;

			xsg_debug("[%d]%s: calling waitpid...",
					(int) e->pid, e->command);

			ret = waitpid(e->pid, &status, WNOHANG);

			xsg_debug("[%d]%s: waitpid returned %d",
					(int) e->pid, e->command, ret);

			if (ret > 0) {
				if (WIFEXITED(status) || WIFSIGNALED(status)) {
					e->pid = 0;
				}
			} else if (ret < 0) {
				e->pid = 0;
			}
		}
	}
}

static void
run_exec(exec_t *e)
{
	int p[2];
	pid_t pid;

	if (e->pid) {
		return;
	}

	xsg_debug("executing %s", e->command);

	if (pipe(p) < 0) {
		xsg_warning("%s: cannot create pipe: %s", e->command,
				strerror(errno));
		return;
	}

	pid = fork();

	if (pid < 0) {
		xsg_warning("%s: cannot fork: %s", e->command,
				strerror(errno));
		close(p[0]);
		close(p[1]);
		return;
	} else if (pid == 0) {
		close(p[0]);
		if (p[1] != STDOUT_FILENO) {
			if (dup2(p[1], STDOUT_FILENO) != STDOUT_FILENO) {
				exit(EXIT_FAILURE);
			}
			close(p[1]);
		}
		execl("/bin/sh", "sh", "-c", e->command, NULL);
		exit(EXIT_FAILURE);
	}

	close(p[1]);

	e->pid = pid;

	xsg_debug("[%u]%s: running...", e->pid, e->command);

	e->poll.fd = p[0];

	xsg_main_add_poll(&e->poll);

	xsg_set_cloexec_flag(p[0], TRUE);
}

static void
update_execs(uint64_t tick)
{
	xsg_list_t *l;
	exec_t *e;
	int i;

	catch_execs();

	for (l = exec_list; l; l = l->next) {
		e = l->data;

		if (e->pid) {
			continue;
		}

		for (i = 0; i < e->updatec; i++) {
			if ((tick == 0) || (tick % e->updatev[i]) == 0) {
				run_exec(e);
			}
		}
	}
}

/******************************************************************************/

static void
shutdown_execs(void)
{
	xsg_list_t *l;

	for (l = exec_list; l; l = l->next) {
		exec_t *e = l->data;

		if (!e->pid) {
			continue;
		}

		kill_exec(e);
	}

	catch_execs();
}

/******************************************************************************/

static void
signal_handler_exec(int signum)
{
	catch_execs();
}

/******************************************************************************/

static void
parse_exec(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	char *command;
	xsg_buffer_t *buffer;

	command = xsg_conf_read_string();

	buffer = find_exec_buffer(command, update);

	xsg_free(command);

	xsg_buffer_parse(buffer, var, num, str, arg);

	xsg_main_add_update_func(update_execs);
	xsg_main_add_shutdown_func(shutdown_execs);
	xsg_main_add_signal_handler(signal_handler_exec);
}

static const char *
help_exec(void)
{
	static xsg_string_t *string = NULL;

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	xsg_buffer_help(string, XSG_MODULE_NAME, "<command>");

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_exec, help_exec, "execute processes and read from their stdout");

