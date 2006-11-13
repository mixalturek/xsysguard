/* stat.c
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
#include <statgrab.h>
#include <string.h>
#include <stdlib.h>

/******************************************************************************/

typedef struct {
	sg_host_info		*host_info;
	sg_cpu_stats		*cpu_stats;
	sg_cpu_stats		*cpu_stats_diff;
	sg_cpu_percents		*cpu_percents;
	sg_mem_stats		*mem_stats;
	sg_load_stats		*load_stats;
	sg_user_stats		*user_stats;
	sg_swap_stats		*swap_stats;

	int fs_entries;
	sg_fs_stats		*fs_stats_by_device_name;
	sg_fs_stats             *fs_stats_by_mnt_point;

	int disk_io_entries;
	sg_disk_io_stats	*disk_io_stats;

	int disk_io_diff_entries;
	sg_disk_io_stats	*disk_io_stats_diff;

	int network_io_entries;
	sg_network_io_stats	*network_io_stats;

	int network_io_diff_entries;
	sg_network_io_stats	*network_io_stats_diff;

	int network_iface_entries;
	sg_network_iface_stats  *network_iface_stats;

	sg_page_stats		*page_stats;
	sg_page_stats		*page_stats_diff;

	int process_entries;
	sg_process_stats	*process_stats;
	sg_process_stats	*process_stats_by_name;
	sg_process_stats	*process_stats_by_pid;
	sg_process_stats	*process_stats_by_uid;
	sg_process_stats	*process_stats_by_gid;
	sg_process_stats	*process_stats_by_size;
	sg_process_stats	*process_stats_by_res;
	sg_process_stats	*process_stats_by_cpu;
	sg_process_stats	*process_stats_by_time;

	sg_process_count	*process_count;
} stats_t;

static stats_t stats = { 0 };

/******************************************************************************
 *
 * handle libstatgrab errors
 *
 ******************************************************************************/

static void libstatgrab_error() {
	sg_error error;
	const char *error_str;
	const char *error_arg;
	int errnum;

	error = sg_get_error();

	if (error == SG_ERROR_NONE)
		return;

	error_str = sg_str_error(error);
	error_arg = sg_get_error_arg();
	errnum = sg_get_error_errno();

	sg_set_error(SG_ERROR_NONE, "");

	if (errnum > 0)
		g_warning("libstatgrab: %s: %s: %s", error_str, error_arg, strerror(errnum));
	else
		g_warning("libstatgrab: %s: %s", error_str, error_arg);
}

/******************************************************************************
 *
 * libstatgrab functions
 *
 ******************************************************************************/

static void get_host_info() {
	g_message("Get (host_info)");
	if (!(stats.host_info = sg_get_host_info())) {
		g_warning("sg_get_host_info() returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_stats() {
	g_message("Get (cpu_stats)");
	if (!(stats.cpu_stats = sg_get_cpu_stats())) {
		g_warning("sg_get_cpu_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_stats_diff() {
	g_message("Get (cpu_stats_diff)");
	if (!(stats.cpu_stats_diff = sg_get_cpu_stats_diff())) {
		g_warning("sg_get_cpu_stats_diff() returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_percents() {
	g_message("Get (cpu_percents)");
	if (!(stats.cpu_percents = sg_get_cpu_percents())) {
		g_warning("sg_get_cpu_percents() returned NULL");
		libstatgrab_error();
	}
}

static void get_mem_stats() {
	g_message("Get (mem_stats)");
	if (!(stats.mem_stats = sg_get_mem_stats())) {
		g_warning("sg_get_mem_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_load_stats() {
	g_message("Get (load_stats)");
	if (!(stats.load_stats = sg_get_load_stats())) {
		g_warning("sg_get_load_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_user_stats() {
	g_message("Get (user_stats)");
	if (!(stats.user_stats = sg_get_user_stats())) {
		g_warning("sg_get_user_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_swap_stats() {
	g_message("Get (swap_stats)");
	if (!(stats.swap_stats = sg_get_swap_stats())) {
		g_warning("sg_get_swap_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_fs_stats() {
	size_t size;

	g_message("Get (fs_stats)");
	if (!(stats.fs_stats_by_device_name = sg_get_fs_stats(&stats.fs_entries))) {
		g_warning("sg_get_fs_stats() returned NULL");
		libstatgrab_error();
		return;
	}
	size = sizeof(sg_fs_stats) * stats.fs_entries;
	stats.fs_stats_by_mnt_point = (sg_fs_stats *) g_realloc((void *) stats.fs_stats_by_mnt_point, size);
	memcpy(stats.fs_stats_by_mnt_point, stats.fs_stats_by_device_name, size);

	qsort(stats.fs_stats_by_device_name, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_device_name);
	qsort(stats.fs_stats_by_mnt_point, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_mnt_point);
}

static void get_disk_io_stats() {
	g_message("Get (disk_io_stats)");
	if (!(stats.disk_io_stats = sg_get_disk_io_stats(&stats.disk_io_entries))) {
		g_warning("sg_get_disk_io_stats() returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.disk_io_stats, stats.disk_io_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void get_disk_io_stats_diff() {
	g_message("Get (disk_io_stats_diff)");
	if (!(stats.disk_io_stats_diff = sg_get_disk_io_stats_diff(&stats.disk_io_diff_entries))) {
		g_warning("sg_get_disk_io_stats_diff() returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.disk_io_stats_diff, stats.disk_io_diff_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void get_network_io_stats() {
	g_message("Get (network_io_stats)");
	if (!(stats.network_io_stats = sg_get_network_io_stats(&stats.network_io_entries))) {
		g_warning("sg_get_network_io_stats() returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_io_stats, stats.network_io_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void get_network_io_stats_diff() {
	g_message("Get (network_io_stats_diff)");
	if (!(stats.network_io_stats_diff = sg_get_network_io_stats_diff(&stats.network_io_diff_entries))) {
		g_warning("sg_get_network_io_stats_diff() returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_io_stats_diff, stats.network_io_diff_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void get_network_iface_stats() {
	g_message("Get (network_iface_stats)");
	if (!(stats.network_iface_stats = sg_get_network_iface_stats(&stats.network_iface_entries))) {
		g_warning("sg_get_network_iface_stats() returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_iface_stats, stats.network_iface_entries, sizeof(sg_network_iface_stats), sg_network_iface_compare_name);
}

static void get_page_stats() {
	g_message("Get (page_stats)");
	if (!(stats.page_stats = sg_get_page_stats())) {
		g_warning("sg_get_page_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_page_stats_diff() {
	g_message("Get (page_stats_diff)");
	if (!(stats.page_stats_diff = sg_get_page_stats_diff())) {
		g_warning("sg_get_page_stats_diff() returned NULL");
		libstatgrab_error();
	}
}

static void get_process_stats() {
	g_message("Get (process_stats)");
	if (stats.process_stats_by_name) {
		free(stats.process_stats_by_name);
		stats.process_stats_by_name = 0;
	}
	if (stats.process_stats_by_pid) {
		free(stats.process_stats_by_pid);
		stats.process_stats_by_pid = 0;
	}
	if (stats.process_stats_by_uid) {
		free(stats.process_stats_by_uid);
		stats.process_stats_by_uid = 0;
	}
	if (stats.process_stats_by_gid) {
		free(stats.process_stats_by_gid);
		stats.process_stats_by_gid = 0;
	}
	if (stats.process_stats_by_size) {
		free(stats.process_stats_by_size);
		stats.process_stats_by_size = 0;
	}
	if (stats.process_stats_by_res) {
		free(stats.process_stats_by_res);
		stats.process_stats_by_res = 0;
	}
	if (stats.process_stats_by_cpu) {
		free(stats.process_stats_by_cpu);
		stats.process_stats_by_cpu = 0;
	}
	if (stats.process_stats_by_time) {
		free(stats.process_stats_by_time);
		stats.process_stats_by_time = 0;
	}
	if (!(stats.process_stats = sg_get_process_stats(&stats.process_entries))) {
		g_warning("sg_get_process_stats() returned NULL");
		libstatgrab_error();
	}
}

static void get_process_count() {
	g_message("Get (process_count)");
	if (!(stats.process_count = sg_get_process_count())) {
		g_warning("sg_get_process_count() returned NULL");
		libstatgrab_error();
	}
}

/******************************************************************************/

typedef struct {
	uint64_t update;
	void (*(func))();
} stat_t;

xsg_list_t *stat_list = NULL;

static void add_stat(uint64_t update, void (*(func))()) {
	stat_t *stat = 0;
	xsg_list_t *l;

	/* find matching entry in stat_list */
	for (l = stat_list; l; l = l->next) {
		stat =l->data;
		if (stat->func == func) {
			/* is update a multiple of stat->update? */
			if (update % stat->update == 0)
				return;
			/* is stat->update a multiple of update? */
			if (stat->update % update == 0) {
				stat->update = update;
				return;
			}
		}
	}
	/* no matching entry found in stat_list -> add new entry */
	stat = g_new(stat_t, 1);
	stat->update = update;
	stat->func = func;
	stat_list = xsg_list_append(stat_list, stat);
}

/******************************************************************************/

void update_stats(uint64_t count) {
	stat_t *stat;
	xsg_list_t *l;

	for (l = stat_list; l; l = l->next) {
		stat = l->data;
		if (count % stat->update == 0)
			(stat->func)();
	}
}

/******************************************************************************
 *
 * host_info
 *
 ******************************************************************************/

static void *get_host_info_os_name(void *arg) {
	char *value;

	if (stats.host_info)
		value = stats.host_info->os_name;
	else
		value = "";

	g_message("Get (host_info:os_name) \"%s\"", value);

	return (void *) value;
}

static void *get_host_info_os_release(void *arg) {
	char *value;

	if (stats.host_info)
		value =  stats.host_info->os_release;
	else
		value = "";

	g_message("Get (host_info:os_release) \"%s\"", value);

	return (void *) value;
}

static void *get_host_info_os_version(void *arg) {
	char *value;

	if (stats.host_info)
		value =  stats.host_info->os_version;
	else
		value = "";

	g_message("Get (host_info:os_version) \"%s\"", value);

	return (void *) value;
}

static void *get_host_info_platform(void *arg) {
	char *value;

	if (stats.host_info)
		value = stats.host_info->platform;
	else
		value = "";

	g_message("Get (host_info:platform) \"%s\"", value);

	return (void *) value;
}

static void *get_host_info_hostname(void *arg) {
	char *value;

	if (stats.host_info)
		value = stats.host_info->hostname;
	else
		value = "";

	g_message("Get (host_info:hostname) \"%s\"", value);

	return (void *) value;
}

typedef struct {
	uint64_t mod;
	uint64_t div;
} host_info_uptime_t;

static void *get_host_info_uptime(void *arg) {
	host_info_uptime_t *data;
	static int64_t value;

	if (!stats.host_info) {
		value = 0;
		return (void *) &value;
	} else {
		data = (host_info_uptime_t *) arg;
		value = stats.host_info->uptime;

		if (data->mod)
			value %= data->mod;
		if (data->div)
			value /= data->div;
	}
	g_message("Get (host_info:uptime:mod=%" PRIu64 ":div=%" PRIu64 ") %" PRId64,
			data->mod, data->div, value);

	return (void *) &value;
}

static void parse_host_info(xsg_var_t *var) {

	if (xsg_conf_find_command("os_name")) {
		var->type = XSG_STRING;
		var->func = get_host_info_os_name;
	} else if (xsg_conf_find_command("os_release")) {
		var->type = XSG_STRING;
		var->func = get_host_info_os_release;
	} else if (xsg_conf_find_command("os_version")) {
		var->type = XSG_STRING;
		var->func = get_host_info_os_version;
	} else if (xsg_conf_find_command("platform")) {
		var->type = XSG_STRING;
		var->func = get_host_info_platform;
	} else if (xsg_conf_find_command("hostname")) {
		var->type = XSG_STRING;
		var->func = get_host_info_hostname;
	} else if (xsg_conf_find_command("uptime")) {
		host_info_uptime_t *data = g_new0(host_info_uptime_t, 1);
		data->mod = xsg_conf_read_uint();
		data->div = xsg_conf_read_uint();
		var->type = XSG_INT;
		var->func = get_host_info_uptime;
		var->args = (void *) data;
	} else {
		xsg_conf_error("os_name, os_release, os_version, platform, hostname or uptime");
	}
}

/******************************************************************************
 *
 * cpu_stats
 *
 ******************************************************************************/

static void *get_cpu_stats_user(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->user;
	else
		value = 0;

	g_message("Get (cpu_stats:user) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_kernel(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->kernel;
	else
		value = 0;

	g_message("Get (cpu_stats:kernel) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_idle(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->idle;
	else
		value = 0;

	g_message("Get (cpu_stats:idle) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_iowait(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->iowait;
	else
		value = 0;

	g_message("Get (cpu_stats:iowait) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_swap(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->swap;
	else
		value = 0;

	g_message("Get (cpu_stats:swap) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_nice(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->nice;
	else
		value = 0;

	g_message("Get (cpu_stats:nice) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_total(void *arg) {
	static int64_t value;

	if (stats.cpu_stats)
		value = stats.cpu_stats->total;
	else
		value = 0;

	g_message("Get (cpu_stats:total) %" PRId64, value);

	return (void *) &value;
}

static void parse_cpu_stats(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("user"))
		var->func = get_cpu_stats_user;
	else if (xsg_conf_find_command("kernel"))
		var->func = get_cpu_stats_kernel;
	else if (xsg_conf_find_command("idle"))
		var->func = get_cpu_stats_idle;
	else if (xsg_conf_find_command("iowait"))
		var->func = get_cpu_stats_iowait;
	else if (xsg_conf_find_command("swap"))
		var->func = get_cpu_stats_swap;
	else if (xsg_conf_find_command("nice"))
		var->func = get_cpu_stats_nice;
	else if (xsg_conf_find_command("total"))
		var->func = get_cpu_stats_total;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or total");
}

/******************************************************************************
 *
 * cpu_stats_diff
 *
 ******************************************************************************/

static void *get_cpu_stats_diff_user(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->user;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:user) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_kernel(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->kernel;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:kernel) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_idle(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->idle;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:idle) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_iowait(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->iowait;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:iowait) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_swap(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->swap;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:swap) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_nice(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->nice;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:nice) %" PRId64, value);

	return (void *) &value;
}

static void *get_cpu_stats_diff_total(void *arg) {
	static int64_t value;

	if (stats.cpu_stats_diff)
		value = stats.cpu_stats_diff->total;
	else
		value = 0;

	g_message("Get (cpu_stats_diff:total) %" PRId64, value);

	return (void *) &value;
}

static void parse_cpu_stats_diff(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("user"))
		var->func = get_cpu_stats_diff_user;
	else if (xsg_conf_find_command("kernel"))
		var->func = get_cpu_stats_diff_kernel;
	else if (xsg_conf_find_command("idle"))
		var->func = get_cpu_stats_diff_idle;
	else if (xsg_conf_find_command("iowait"))
		var->func = get_cpu_stats_diff_iowait;
	else if (xsg_conf_find_command("swap"))
		var->func = get_cpu_stats_diff_swap;
	else if (xsg_conf_find_command("nice"))
		var->func = get_cpu_stats_diff_nice;
	else if (xsg_conf_find_command("total"))
		var->func = get_cpu_stats_diff_total;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or total");
}

/******************************************************************************
 *
 * cpu_percents
 *
 ******************************************************************************/

static void *get_cpu_percents_user(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->user;
	else
		value = 0.0;

	g_message("Get (cpu_percents:user) %f", value);

	return (void *) &value;
}

static void *get_cpu_percents_kernel(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->kernel;
	else
		value = 0.0;

	g_message("Get (cpu_percents:kernel) %f", value);

	return (void *) &value;
}

static void *get_cpu_percents_idle(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->idle;
	else
		value = 0.0;

	g_message("Get (cpu_percents:idle) %f", value);

	return (void *) &value;
}

static void *get_cpu_percents_iowait(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->iowait;
	else
		value = 0.0;

	g_message("Get (cpu_percents:iowait) %f", value);

	return (void *) &value;
}

static void *get_cpu_percents_swap(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->swap;
	else
		value = 0.0;

	g_message("Get (cpu_percents:swap) %f", value);

	return (void *) &value;
}

static void *get_cpu_percents_nice(void *arg) {
	static double value;

	if (stats.cpu_percents)
		value = stats.cpu_percents->nice;
	else
		value = 0.0;

	g_message("Get (cpu_percents:nice) %f", value);

	return (void *) &value;
}

static void parse_cpu_percents(xsg_var_t *var) {

	var->type = XSG_DOUBLE;

	if (xsg_conf_find_command("user"))
		var->func = get_cpu_percents_user;
	else if (xsg_conf_find_command("kernel"))
		var->func = get_cpu_percents_kernel;
	else if (xsg_conf_find_command("idle"))
		var->func = get_cpu_percents_idle;
	else if (xsg_conf_find_command("iowait"))
		var->func = get_cpu_percents_iowait;
	else if (xsg_conf_find_command("swap"))
		var->func = get_cpu_percents_swap;
	else if (xsg_conf_find_command("nice"))
		var->func = get_cpu_percents_nice;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap or nice");
}

/******************************************************************************
 *
 * mem_stats
 *
 ******************************************************************************/

static void *get_mem_stats_total(void *arg) {
	static int64_t value;

	if (stats.mem_stats)
		value = stats.mem_stats->total;
	else
		value = 0;

	g_message("Get (mem_stats:total) %" PRId64, value);

	return (void *) &value;
}

static void *get_mem_stats_free(void *arg) {
	static int64_t value;

	if (stats.mem_stats)
		value = stats.mem_stats->free;
	else
		value = 0;

	g_message("Get (mem_stats:free) %" PRId64, value);

	return (void *) &value;
}

static void *get_mem_stats_used(void *arg) {
	static int64_t value;

	if (stats.mem_stats)
		value = stats.mem_stats->used;
	else
		value = 0;

	g_message("Get (mem_stats:used) %" PRId64, value);

	return (void *) &value;
}

static void *get_mem_stats_cache(void *arg) {
	static int64_t value;

	if (stats.mem_stats)
		value = stats.mem_stats->cache;
	else
		value = 0;

	g_message("Get (mem_stats:cache) %" PRId64, value);

	return (void *) &value;
}

static void parse_mem_stats(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("total"))
		var->func = get_mem_stats_total;
	else if (xsg_conf_find_command("free"))
		var->func = get_mem_stats_free;
	else if (xsg_conf_find_command("used"))
		var->func = get_mem_stats_used;
	else if (xsg_conf_find_command("cache"))
		var->func = get_mem_stats_cache;
	else
		xsg_conf_error("total, free, used or cache");
}

/******************************************************************************
 *
 * load_stats
 *
 ******************************************************************************/

static void *get_load_stats_min1(void *arg) {
	static double value;

	if (stats.load_stats)
		value = stats.load_stats->min1;
	else
		value = 0.0;

	g_message("Get (load_stats:min1) %f", value);

	return (void *) &value;
}

static void *get_load_stats_min5(void *arg) {
	static double value;

	if (stats.load_stats)
		value = stats.load_stats->min5;
	else
		value = 0.0;

	g_message("Get (load_stats:min5) %f", value);

	return (void *) &value;
}

static void *get_load_stats_min15(void *arg) {
	static double value;

	if (stats.load_stats)
		value = stats.load_stats->min15;
	else
		value = 0.0;

	g_message("Get (load_stats:min15) %f", value);

	return (void *) &value;
}

static void parse_load_stats(xsg_var_t *var) {

	var->type = XSG_DOUBLE;

	if (xsg_conf_find_command("min1"))
		var->func = get_load_stats_min1;
	else if (xsg_conf_find_command("min5"))
		var->func = get_load_stats_min5;
	else if (xsg_conf_find_command("min15"))
		var->func = get_load_stats_min15;
	else
		xsg_conf_error("min1, min5 or min15");
}

/******************************************************************************
 *
 * user_stats
 *
 ******************************************************************************/

static void *get_user_stats_name_list(void *arg) {
	char *value;

	if (stats.user_stats)
		value = stats.user_stats->name_list;
	else
		value = "";

	g_message("Get (user_stats:name_list) \"%s\"", value);

	return (void *) value;
}

static void *get_user_stats_num_entries(void *arg) {
	static int64_t value;

	if (stats.user_stats)
		value = stats.user_stats->num_entries;
	else
		value = 0;

	g_message("Get (user_stats:num_entries) %" PRId64, value);

	return (void *) &value;
}

static void parse_user_stats(xsg_var_t *var) {

	if (xsg_conf_find_command("name_list")) {
		var->type = XSG_STRING;
		var->func = get_user_stats_name_list;
	} else if (xsg_conf_find_command("num_entries")) {
		var->type = XSG_INT;
		var->func = get_user_stats_num_entries;
	} else {
		xsg_conf_error("name_list or num_entries");
	}
}

/******************************************************************************
 *
 * swap_stats
 *
 ******************************************************************************/

static void *get_swap_stats_total(void *arg) {
	static int64_t value;

	if (stats.swap_stats)
		value = stats.swap_stats->total;
	else
		value = 0;

	g_message("Get (swap_stats:total) %" PRId64, value);

	return (void *) &value;
}

static void *get_swap_stats_used(void *arg) {
	static int64_t value;

	if (stats.swap_stats)
		value = stats.swap_stats->used;
	else
		value = 0;

	g_message("Get (swap_stats:used) %" PRId64, value);

	return (void *) &value;
}

static void *get_swap_stats_free(void *arg) {
	static int64_t value;

	if (stats.swap_stats)
		value = stats.swap_stats->free;
	else
		value = 0;

	g_message("Get (swap_stats:free) %" PRId64, value);

	return (void *) &value;
}

static void parse_swap_stats(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("total"))
		var->func = get_swap_stats_total;
	else if (xsg_conf_find_command("used"))
		var->func = get_swap_stats_used;
	else if (xsg_conf_find_command("free"))
		var->func = get_swap_stats_free;
	else
		xsg_conf_error("total, used or free");
}

/******************************************************************************
 *
 * fs_stats
 *
 ******************************************************************************/

static sg_fs_stats *get_fs_stats_for_device_name(char *device_name) {
	sg_fs_stats key;

	key.device_name = device_name;
	return bsearch(&key, stats.fs_stats_by_device_name, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_device_name);
}

static sg_fs_stats *get_fs_stats_for_mnt_point(char *mnt_point) {
	sg_fs_stats key;

	key.mnt_point = mnt_point;
	return bsearch(&key, stats.fs_stats_by_mnt_point, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_mnt_point);
}

static void *get_fs_stats_device_name(void *arg) {
	sg_fs_stats *ret;
	char *data;
	char *value;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->device_name;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->device_name;
	} else {
		g_message("Get (fs_stats:\"%s\":device_name) NOT FOUND", data);
		return (void *) "";
	}
	g_message("Get (fs_stats:\"%s\":device_name) \"%s\"", data, value);

	return (void *) value;
}

static void *get_fs_stats_fs_type(void *arg) {
	sg_fs_stats *ret;
	char *data;
	char *value;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->fs_type;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->fs_type;
	} else {
		g_message("Get (fs_stats:\"%s\":fs_type) NOT FOUND", data);
		return (void *) "";
	}
	g_message("Get (fs_stats:\"%s\":fs_type) \"%s\"", data, value);

	return (void *) value;
}

static void *get_fs_stats_mnt_point(void *arg) {
	sg_fs_stats *ret;
	char *data;
	char *value;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->mnt_point;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->mnt_point;
	} else {
		g_message("Get (fs_stats:\"%s\":mnt_point) NOT FOUND", data);
		return (void *) "";
	}
	g_message("Get (fs_stats:\"%s\":mnt_point) \"%s\"", data, value);

	return (void *) value;
}

static void *get_fs_stats_size(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->size;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":size) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":size) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_used(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->used;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->used;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":used) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":used) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_avail(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->avail;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->avail;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":avail) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":avail) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_total_inodes(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->total_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->total_inodes;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":total_inodes) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":total_inodes) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_used_inodes(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->used_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->used_inodes;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":used_inodes) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":used_inodes) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_free_inodes(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->free_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->free_inodes;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":free_inodes) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":free_inodes) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_avail_inodes(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->avail_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->avail_inodes;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":avail_inodes) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":avail_inodes) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_io_size(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->io_size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->io_size;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":io_size) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":io_size) %" PRId64, data, value);
	return (void *) &value;
}

static void *get_fs_stats_block_size(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->block_size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->block_size;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":block_size) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":block_size) %" PRId64, data, value);
	return (void *) &value;
}

static void *get_fs_stats_total_blocks(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->total_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->total_blocks;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":total_blocks) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":total_blocks) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_free_blocks(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->free_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->free_blocks;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":free_blocks) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":free_blocks) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_used_blocks(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->used_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->used_blocks;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":used_blocks) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":used_blocks) %" PRId64, data, value);

	return (void *) &value;
}

static void *get_fs_stats_avail_blocks(void *arg) {
	static int64_t value;
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		value = ret->avail_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		value = ret->avail_blocks;
	} else {
		value = 0;
		g_message("Get (fs_stats:\"%s\":avail_blocks) NOT FOUND", data);
		return (void *) &value;
	}
	g_message("Get (fs_stats:\"%s\":avail_blocks) %" PRId64, data, value);

	return (void *) &value;
}

static void parse_fs_stats(xsg_var_t *var) {
	char *data;

	data = xsg_conf_read_string();
	var->args = (void *) data;

	if (xsg_conf_find_command("device_name")) {
		var->type = XSG_STRING;
		var->func = get_fs_stats_device_name;
	} else if (xsg_conf_find_command("fs_type")) {
		var->type = XSG_STRING;
		var->func = get_fs_stats_fs_type;
	} else if (xsg_conf_find_command("mnt_point")) {
		var->type = XSG_STRING;
		var->func = get_fs_stats_mnt_point;
	} else if (xsg_conf_find_command("size")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_size;
	} else if (xsg_conf_find_command("used")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_used;
	} else if (xsg_conf_find_command("avail")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_avail;
	} else if (xsg_conf_find_command("total_inodes")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_total_inodes;
	} else if (xsg_conf_find_command("used_inodes")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_used_inodes;
	} else if (xsg_conf_find_command("free_inodes")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_free_inodes;
	} else if (xsg_conf_find_command("avail_inodes")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_avail_inodes;
	} else if (xsg_conf_find_command("io_size")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_io_size;
	} else if (xsg_conf_find_command("block_size")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_block_size;
	} else if (xsg_conf_find_command("total_blocks")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_total_blocks;
	} else if (xsg_conf_find_command("free_blocks")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_free_blocks;
	} else if (xsg_conf_find_command("used_blocks")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_used_blocks;
	} else if (xsg_conf_find_command("avail_blocks")) {
		var->type = XSG_INT;
		var->func = get_fs_stats_avail_blocks;
	} else {
		xsg_conf_error("device_name, fs_type, mnt_point, size, used, avail, "
				"total_inodes, used_inodes, free_inodes, avail_inodes, "
				"io_size, block_size, total_blocks, free_blocks, "
				"used_blocks or avail_blocks");
	}
}

/******************************************************************************
 *
 * disk_io_stats
 *
 ******************************************************************************/

static sg_disk_io_stats *get_disk_io_stats_for_disk_name(char *disk_name) {
	sg_disk_io_stats key;

	key.disk_name = disk_name;
	return bsearch(&key, stats.disk_io_stats, stats.disk_io_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void *get_disk_io_stats_read_bytes(void *arg) {
	static int64_t value;
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		value = ret->read_bytes;
		g_message("Get (disk_io_stats:\"%s\":read_bytes) %" PRId64, disk_name, value);
	} else {
		value = 0;
		g_message("Get (disk_io_stats:\"%s\":read_bytes) NOT FOUND", disk_name);
	}
	return (void *) &value;
}

static void *get_disk_io_stats_write_bytes(void *arg) {
	static int64_t value;
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		value = ret->write_bytes;
		g_message("Get (disk_io_stats:\"%s\":write_bytes) %" PRId64, disk_name, value);
	} else {
		value = 0;
		g_message("Get (disk_io_stats:\"%s\":write_bytes) NOT FOUND", disk_name);
	}
	return (void *) &value;
}

static void parse_disk_io_stats(xsg_var_t *var) {
	char *disk_name;

	disk_name = xsg_conf_read_string();
	var->type = XSG_INT;
	var->args = (void *) disk_name;

	if (xsg_conf_find_command("read_bytes"))
		var->func = get_disk_io_stats_read_bytes;
	else if (xsg_conf_find_command("write_bytes"))
		var->func = get_disk_io_stats_write_bytes;
	else
		xsg_conf_error("read_bytes or write_bytes");
}

/******************************************************************************
 *
 * disk_io_stats_diff
 *
 ******************************************************************************/

static sg_disk_io_stats *get_disk_io_stats_diff_for_disk_name(char *disk_name) {
	sg_disk_io_stats key;

	key.disk_name = disk_name;
	return bsearch(&key, stats.disk_io_stats_diff, stats.disk_io_diff_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void *get_disk_io_stats_diff_read_bytes(void *arg) {
	static int64_t value;
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(disk_name))) {
		value = ret->read_bytes;
		g_message("Get (disk_io_stats_diff:\"%s\":read_bytes) %" PRId64, disk_name, value);
	} else {
		value = 0;
		g_message("Get (disk_io_stats_diff:\"%s\":read_bytes) NOT FOUND", disk_name);
	}
	return (void *) &value;
}

static void *get_disk_io_stats_diff_write_bytes(void *arg) {
	static int64_t value;
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(disk_name))) {
		value = ret->write_bytes;
		g_message("Get (disk_io_stats_diff:\"%s\":write_bytes) %" PRId64, disk_name, value);
	} else {
		value = 0;
		g_message("Get (disk_io_stats_diff:\"%s\":write_bytes) NOT FOUND", disk_name);
	}
	return (void *) &value;
}

static void parse_disk_io_stats_diff(xsg_var_t *var) {
	char *disk_name;

	disk_name = xsg_conf_read_string();
	var->type = XSG_INT;
	var->args = (void *) disk_name;

	if (xsg_conf_find_command("read_bytes"))
		var->func = get_disk_io_stats_diff_read_bytes;
	else if (xsg_conf_find_command("write_bytes"))
		var->func = get_disk_io_stats_diff_write_bytes;
	else
		xsg_conf_error("read_bytes or write_bytes");
}

/******************************************************************************
 *
 * network_io_stats
 *
 ******************************************************************************/

static sg_network_io_stats *get_network_io_stats_for_interface_name(char *interface_name) {
	sg_network_io_stats key;

	key.interface_name = interface_name;
	return bsearch(&key, stats.network_io_stats, stats.network_io_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void *get_network_io_stats_tx(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->tx;
		g_message("Get (network_io_stats:\"%s\":tx) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":tx) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_rx(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->rx;
		g_message("Get (network_io_stats:\"%s\":rx) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":rx) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_ipackets(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->ipackets;
		g_message("Get (network_io_stats:\"%s\":ipackets) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":ipackets) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_opackets(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->opackets;
		g_message("Get (network_io_stats:\"%s\":opackets) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":opackets) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_ierrors(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->ierrors;
		g_message("Get (network_io_stats:\"%s\":ierrors) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":ierrors) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_oerrors(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->oerrors;
		g_message("Get (network_io_stats:\"%s\":oerrors) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":oerrors) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_collisions(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		value = ret->collisions;
		g_message("Get (network_io_stats:\"%s\":collisions) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats:\"%s\":collisions) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void parse_network_io_stats(xsg_var_t *var) {
	char *interface_name;

	interface_name = xsg_conf_read_string();
	var->type = XSG_INT;
	var->args = (void *) interface_name;

	if (xsg_conf_find_command("tx"))
		var->func = get_network_io_stats_tx;
	else if (xsg_conf_find_command("rx"))
		var->func = get_network_io_stats_rx;
	else if (xsg_conf_find_command("ipackets"))
		var->func = get_network_io_stats_ipackets;
	else if (xsg_conf_find_command("opackets"))
		var->func = get_network_io_stats_opackets;
	else if (xsg_conf_find_command("ierrors"))
		var->func = get_network_io_stats_ierrors;
	else if (xsg_conf_find_command("oerrors"))
		var->func = get_network_io_stats_oerrors;
	else if (xsg_conf_find_command("collisions"))
		var->func = get_network_io_stats_collisions;
	else
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors or collisions");
}

/******************************************************************************
 *
 * network_io_stats_diff
 *
 ******************************************************************************/

static sg_network_io_stats *get_network_io_stats_diff_for_interface_name(char *interface_name) {
	sg_network_io_stats key;

	key.interface_name = interface_name;
	return bsearch(&key, stats.network_io_stats_diff, stats.network_io_diff_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void *get_network_io_stats_diff_tx(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->tx;
		g_message("Get (network_io_stats_diff:\"%s\":tx) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":tx) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_rx(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->rx;
		g_message("Get (network_io_stats_diff:\"%s\":rx) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":rx) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_ipackets(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->ipackets;
		g_message("Get (network_io_stats_diff:\"%s\":ipackets) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":ipackets) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_opackets(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->opackets;
		g_message("Get (network_io_stats_diff:\"%s\":opackets) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":opackets) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_ierrors(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->ierrors;
		g_message("Get (network_io_stats_diff:\"%s\":ierrors) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":ierrors) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_oerrors(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->oerrors;
		g_message("Get (network_io_stats_diff:\"%s\":oerrors) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":oerrors) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void *get_network_io_stats_diff_collisions(void *arg) {
	static int64_t value;
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		value = ret->collisions;
		g_message("Get (network_io_stats_diff:\"%s\":collisions) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_io_stats_diff:\"%s\":collisions) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

static void parse_network_io_stats_diff(xsg_var_t *var) {
	char *interface_name;

	interface_name = xsg_conf_read_string();
	var->type = XSG_INT;
	var->args = (void *) interface_name;

	if (xsg_conf_find_command("tx"))
		var->func = get_network_io_stats_diff_tx;
	else if (xsg_conf_find_command("rx"))
		var->func = get_network_io_stats_diff_rx;
	else if (xsg_conf_find_command("ipackets"))
		var->func = get_network_io_stats_diff_ipackets;
	else if (xsg_conf_find_command("opackets"))
		var->func = get_network_io_stats_diff_opackets;
	else if (xsg_conf_find_command("ierrors"))
		var->func = get_network_io_stats_diff_ierrors;
	else if (xsg_conf_find_command("oerrors"))
		var->func = get_network_io_stats_diff_oerrors;
	else if (xsg_conf_find_command("collisions"))
		var->func = get_network_io_stats_diff_collisions;
	else
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors or collisions");
}

/******************************************************************************
 *
 * network_iface_stats
 *
 ******************************************************************************/

static sg_network_iface_stats *get_network_iface_stats_for_interface_name(char *interface_name) {
	sg_network_iface_stats key;

	key.interface_name = interface_name;
	return bsearch(&key, stats.network_iface_stats, stats.network_iface_entries, sizeof(sg_network_iface_stats), sg_network_iface_compare_name);
}


static void *get_network_iface_stats_speed(void *arg) {
	static int64_t value;
	sg_network_iface_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_iface_stats_for_interface_name(interface_name))) {
		value = ret->speed;
		g_message("Get (network_iface_stats:\"%s\":speed) %" PRId64, interface_name, value);
	} else {
		value = 0;
		g_message("Get (network_iface_stats:\"%s\":speed) NOT FOUND", interface_name);
	}
	return (void *) &value;
}

typedef struct {
	char *interface_name;
	char *full;
	char *half;
	char *unknown;
} network_iface_stats_duplex_data_t;

static void *get_network_iface_stats_duplex(void *arg) {
	sg_network_iface_stats *ret;
	network_iface_stats_duplex_data_t *data;
	char *interface_name;
	char *value;

	data = (network_iface_stats_duplex_data_t *) arg;
	interface_name = data->interface_name;

	if (!(ret = get_network_iface_stats_for_interface_name(interface_name))) {
		g_message("Get (network_iface_stats:\"%s\":duplex:\"%s\":\"%s\":\"%s\") NOT FOUND",
				interface_name, data->full, data->half, data->unknown);
		return (void *) "";
	}
	if (ret->duplex == SG_IFACE_DUPLEX_FULL)
		value = data->full;
	else if (ret->duplex == SG_IFACE_DUPLEX_HALF)
		value = data->half;
	else if (ret->duplex == SG_IFACE_DUPLEX_UNKNOWN)
		value = data->unknown;
	else
		value = data->unknown;
	g_message("Get (network_iface_stats:\"%s\":duplex:\"%s\":\"%s\":\"%s\") \"%s\"",
			interface_name, data->full, data->half, data->unknown, value);
	return (void *) value;;
}

static void parse_network_iface_stats(xsg_var_t *var) {
	char *interface_name;

	interface_name = xsg_conf_read_string();

	if (xsg_conf_find_command("speed")) {
		var->type = XSG_INT;
		var->func = get_network_iface_stats_speed;
	} else if (xsg_conf_find_command("duplex")) {
		network_iface_stats_duplex_data_t *data;

		data = g_new0(network_iface_stats_duplex_data_t, 1);
		data->interface_name = interface_name;
		data->full = xsg_conf_read_string();
		data->half = xsg_conf_read_string();
		data->unknown = xsg_conf_read_string();

		var->type = XSG_STRING;
		var->func = get_network_iface_stats_duplex;
		var->args = (void *) data;
	} else {
		xsg_conf_error("speed or duplex");
	}
}

/******************************************************************************
 *
 * page_stats
 *
 ******************************************************************************/

static void *get_page_stats_pages_pagein(void *arg) {
	static int64_t value;

	if (stats.page_stats)
		value = stats.page_stats->pages_pagein;
	else
		value = 0;

	g_message("Get (page_stats:pages_pagein) %" PRId64, value);

	return (void *) &value;
}

static void *get_page_stats_pages_pageout(void *arg) {
	static int64_t value;

	if (stats.page_stats)
		value = stats.page_stats->pages_pageout;
	else
		value = 0;

	g_message("Get (page_stats:pages_pageout) %" PRId64, value);

	return (void *) &value;
}

static void parse_page_stats(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("pages_pagein"))
		var->func = get_page_stats_pages_pagein;
	else if (xsg_conf_find_command("pages_pageout"))
		var->func = get_page_stats_pages_pageout;
	else
		xsg_conf_error("pages_pagein or pages_pageout");
}

/******************************************************************************
 *
 * page_stats_diff
 *
 ******************************************************************************/

static void *get_page_stats_diff_pages_pagein(void *arg) {
	static int64_t value;

	if (stats.page_stats)
		value = stats.page_stats->pages_pagein;
	else
		value = 0;

	g_message("Get (page_stats_diff:pages_pagein) %" PRId64, value);

	return (void *) &value;
}

static void *get_page_stats_diff_pages_pageout(void *arg) {
	static int64_t value;

	if (stats.page_stats)
		value = stats.page_stats->pages_pageout;
	else
		value = 0;

	g_message("Get (page_stats_diff:pages_pageout) %" PRId64, value);

	return (void *) &value;
}

static void parse_page_stats_diff(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("pages_pagein"))
		var->func = get_page_stats_diff_pages_pagein;
	else if (xsg_conf_find_command("pages_pageout"))
		var->func = get_page_stats_diff_pages_pageout;
	else
		xsg_conf_error("pages_pagein or pages_pageout");
}

/******************************************************************************
 *
 * process_stats
 *
 ******************************************************************************/

sg_process_stats *get_process_stats_orderedby_name() {
	size_t size;

	if (stats.process_stats_by_name)
		return stats.process_stats_by_name;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_name = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_name, stats.process_stats, size);
	qsort(stats.process_stats_by_name, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_name);
	return stats.process_stats_by_name;
}

sg_process_stats *get_process_stats_orderedby_pid() {
	size_t size;

	if (stats.process_stats_by_pid)
		return stats.process_stats_by_pid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_pid = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_pid, stats.process_stats, size);
	qsort(stats.process_stats_by_pid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_pid);
	return stats.process_stats_by_pid;
}

sg_process_stats *get_process_stats_orderedby_uid() {
	size_t size;

	if (stats.process_stats_by_uid)
		return stats.process_stats_by_uid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_uid = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_uid, stats.process_stats, size);
	qsort(stats.process_stats_by_uid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_uid);
	return stats.process_stats_by_uid;
}

sg_process_stats *get_process_stats_orderedby_gid() {
	size_t size;

	if (stats.process_stats_by_gid)
		return stats.process_stats_by_gid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_gid = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_gid, stats.process_stats, size);
	qsort(stats.process_stats_by_gid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_gid);
	return stats.process_stats_by_gid;
}

sg_process_stats *get_process_stats_orderedby_size() {
	size_t size;

	if (stats.process_stats_by_size)
		return stats.process_stats_by_size;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_size = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_size, stats.process_stats, size);
	qsort(stats.process_stats_by_size, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_size);
	return stats.process_stats_by_size;
}

sg_process_stats *get_process_stats_orderedby_res() {
	size_t size;

	if (stats.process_stats_by_res)
		return stats.process_stats_by_res;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_res = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_res, stats.process_stats, size);
	qsort(stats.process_stats_by_res, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_res);
	return stats.process_stats_by_res;
}

sg_process_stats *get_process_stats_orderedby_cpu() {
	size_t size;

	if (stats.process_stats_by_cpu)
		return stats.process_stats_by_cpu;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_cpu = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_cpu, stats.process_stats, size);
	qsort(stats.process_stats_by_cpu, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_cpu);
	return stats.process_stats_by_cpu;
}

sg_process_stats *get_process_stats_orderedby_time() {
	size_t size;

	if (stats.process_stats_by_time)
		return stats.process_stats_by_time;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_time = (sg_process_stats *) g_malloc(size);
	memcpy(stats.process_stats_by_time, stats.process_stats, size);
	qsort(stats.process_stats_by_time, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_time);
	return stats.process_stats_by_time;
}

typedef struct {
	sg_process_stats *(*list_func)();
	char *list_order;
	bool ascending;
	uint32_t number;
} process_stats_data_t;

typedef struct {
	sg_process_stats *(*(list_func))();
	char *list_order;
	bool ascending;
	uint32_t number;
	uint64_t mod;
	uint64_t div;
} process_stats_time_spent_data_t;

typedef struct {
	sg_process_stats *(*(list_func))();
	char *list_order;
	bool ascending;
	uint32_t number;
	char *running;
	char *sleeping;
	char *stopped;
	char *zombie;
	char *unknown;
} process_stats_state_data_t;

static void *get_process_stats_process_name(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	char *value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		g_message("Get (process_stats:process_name:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) "";
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->process_name;
	g_message("Get (process_stats:process_name:%s:%s:%" PRIu32 ") %s", data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) value;
}

static void *get_process_stats_proctitle(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	char *value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		g_message("Get (process_stats:proctitle:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) "";
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->proctitle;
	g_message("Get (process_stats:proctitle:%s:%s:%" PRIu32 ") %s", data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) value;
}

static void *get_process_stats_pid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:pid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->pid;
	g_message("Get (process_stats:pid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_parent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:parent:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->parent;
	g_message("Get (process_stats:parent:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_pgid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:pgid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->pgid;
	g_message("Get (process_stats:pgid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_uid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:uid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->uid;
	g_message("Get (process_stats:uid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_euid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:euid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->euid;
	g_message("Get (process_stats:euid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_gid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:gid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->gid;
	g_message("Get (process_stats:gid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_egid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:egid:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->egid;
	g_message("Get (process_stats:egid:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_proc_size(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:proc_size:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->proc_size;
	g_message("Get (process_stats:proc_size:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_proc_resident(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:proc_resident:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->proc_resident;
	g_message("Get (process_stats:proc_resident:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_time_spent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_time_spent_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_time_spent_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:time_spent:%s:%s:%" PRIu32 ":%llu:%llu) NOT FOUND",
				data->list_order,
				(data->ascending ? "ascending" : "descending"),
				data->number, data->mod, data->div);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->time_spent;

	if (data->mod)
		value %= data->mod;
	if (data->div)
		value /= data->div;

	g_message("Get (process_stats:time_spent:%s:%s:%" PRIu32 ":%llu:%llu) %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number,
			data->mod, data->div, value);

	return (void *) &value;
}

static void *get_process_stats_cpu_percent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static double value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:cpu_percent:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->cpu_percent;
	g_message("Get (process_stats:cpu_percent:%s:%s:%" PRIu32 ") %f", data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_nice(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;
	static int64_t value;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		value = 0;
		g_message("Get (process_stats:nice:%s:%s:%" PRIu32 ") NOT FOUND", data->list_order,
				(data->ascending ? "ascending" : "descending"), data->number);
		return (void *) &value;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	value = sg->nice;
	g_message("Get (process_stats:nice:%s:%s:%" PRIu32 ") %" PRId64, data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number, value);

	return (void *) &value;
}

static void *get_process_stats_state(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_state_data_t *data;
	uint32_t index;
	char *value;

	data = (process_stats_state_data_t *) arg;

	if (data->number >= stats.process_entries) {
		g_message("Get (process_stats:state:%s:%s:%" PRIu32 ":\"%s\":\"%s\":\"%s\":\"%s\":\"%s\") NOT FOUND",
				data->list_order,
				(data->ascending ? "ascending" : "descending"),
				data->number, data->running, data->sleeping, data->stopped,
				data->zombie, data->unknown);
		return (void *) "";
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	switch (sg->state) {
		case SG_PROCESS_STATE_RUNNING:
			value = data->running;
			break;
		case SG_PROCESS_STATE_SLEEPING:
			value = data->sleeping;
			break;
		case SG_PROCESS_STATE_STOPPED:
			value = data->stopped;
			break;
		case SG_PROCESS_STATE_ZOMBIE:
			value = data->zombie;
			break;
		case SG_PROCESS_STATE_UNKNOWN:
		default:
			value = data->unknown;
			break;
	}

	g_message("Get (process_stats:state:%s:%s:%" PRIu32 ":\"%s\":\"%s\":\"%s\":\"%s\":\"%s\") \"%s\"", data->list_order,
			(data->ascending ? "ascending" : "descending"), data->number,
			data->running, data->sleeping, data->stopped, data->zombie,
			data->unknown, value);

	return (void *) value;
}

static void parse_process_stats(xsg_var_t *var) {
	sg_process_stats *(*list_func)() = NULL;
	char *list_order = NULL;
	bool ascending = TRUE;
	uint32_t number = 0;

	if (xsg_conf_find_command("process_name")) {
		var->type = XSG_STRING;
		var->func = get_process_stats_process_name;
	} else if (xsg_conf_find_command("proctitle")) {
		var->type = XSG_STRING;
		var->func = get_process_stats_proctitle;
	} else if (xsg_conf_find_command("pid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_pid;
	} else if (xsg_conf_find_command("parent")) {
		var->type = XSG_INT;
		var->func = get_process_stats_parent;
	} else if (xsg_conf_find_command("pgid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_pgid;
	} else if (xsg_conf_find_command("uid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_uid;
	} else if (xsg_conf_find_command("euid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_euid;
	} else if (xsg_conf_find_command("gid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_gid;
	} else if (xsg_conf_find_command("egid")) {
		var->type = XSG_INT;
		var->func = get_process_stats_egid;
	} else if (xsg_conf_find_command("proc_size")) {
		var->type = XSG_INT;
		var->func = get_process_stats_proc_size;
	} else if (xsg_conf_find_command("proc_resident")) {
		var->type = XSG_INT;
		var->func = get_process_stats_proc_resident;
	} else if (xsg_conf_find_command("time_spent")) {
		var->type = XSG_INT;
		var->func = get_process_stats_time_spent;
	} else if (xsg_conf_find_command("cpu_percent")) {
		var->type = XSG_DOUBLE;
		var->func = get_process_stats_cpu_percent;
	} else if (xsg_conf_find_command("nice")) {
		var->type = XSG_INT;
		var->func = get_process_stats_nice;
	} else if (xsg_conf_find_command("state")) {
		var->type = XSG_STRING;
		var->func = get_process_stats_state;
	} else {
		xsg_conf_error("process_name, proctitle, pid, parent, pgid, uid, euid, gid, "
				"egid, proc_size, proc_resident, time_spent, cpu_percent, "
				"nice or state");
	}

	if (xsg_conf_find_command("name")) {
		list_func = get_process_stats_orderedby_name;
		list_order = "name";
	} else if (xsg_conf_find_command("pid")) {
		list_func = get_process_stats_orderedby_pid;
		list_order = "pid";
	} else if (xsg_conf_find_command("uid")) {
		list_func = get_process_stats_orderedby_uid;
		list_order = "uid";
	} else if (xsg_conf_find_command("gid")) {
		list_func = get_process_stats_orderedby_gid;
		list_order = "gid";
	} else if (xsg_conf_find_command("size")) {
		list_func = get_process_stats_orderedby_size;
		list_order = "size";
	} else if (xsg_conf_find_command("res")) {
		list_func = get_process_stats_orderedby_res;
		list_order = "res";
	} else if (xsg_conf_find_command("cpu")) {
		list_func = get_process_stats_orderedby_cpu;
		list_order = "cpu";
	} else if (xsg_conf_find_command("time")) {
		list_func = get_process_stats_orderedby_time;
		list_order = "time";
	} else {
		xsg_conf_error("name, pid, uid, gid, size, res, cpu or time");
	}

	if (xsg_conf_find_command("ascending"))
		ascending = TRUE;
	else if (xsg_conf_find_command("descending"))
		ascending = FALSE;
	else
		xsg_conf_error("ascending or descending");

	number = xsg_conf_read_uint();

	if (var->func == get_process_stats_time_spent) {
		process_stats_time_spent_data_t *data;
		data = g_new0(process_stats_time_spent_data_t, 1);
		data->list_func = list_func;
		data->list_order = list_order;
		data->ascending = ascending;
		data->number = number;
		data->mod = xsg_conf_read_uint();;
		data->div = xsg_conf_read_uint();;
	} else if (var->func == get_process_stats_state) {
		process_stats_state_data_t *data;
		data = g_new0(process_stats_state_data_t, 1);
		data->list_func = list_func;
		data->list_order = list_order;
		data->ascending = ascending;
		data->number = number;
		data->running = xsg_conf_read_string();
		data->sleeping = xsg_conf_read_string();
		data->stopped = xsg_conf_read_string();
		data->zombie = xsg_conf_read_string();
		data->unknown = xsg_conf_read_string();
	} else {
		process_stats_data_t *data;
		data = g_new0(process_stats_data_t, 1);
		data->list_func = list_func;
		data->list_order = list_order;
		data->ascending = ascending;
		data->number = number;
	}
}

/******************************************************************************
 *
 * process_count
 *
 ******************************************************************************/

static void *get_process_count_total(void *arg) {
	static int64_t value;

	if (stats.process_count)
		value = stats.process_count->total;
	else
		value = 0;

	g_message("Get (process_count:total) %" PRId64, value);

	return (void *) &value;
}

static void *get_process_count_running(void *arg) {
	static int64_t value;

	if (stats.process_count)
		value = stats.process_count->running;
	else
		value = 0;

	g_message("Get (process_count:running) %" PRId64, value);

	return (void *) &value;
}

static void *get_process_count_sleeping(void *arg) {
	static int64_t value;

	if (stats.process_count)
		value = stats.process_count->sleeping;
	else
		value = 0;

	g_message("Get (process_count:sleeping) %" PRId64, value);

	return (void *) &value;
}

static void *get_process_count_stopped(void *arg) {
	static int64_t value;

	if (stats.process_count)
		value = stats.process_count->stopped;
	else
		value = 0;

	g_message("Get (process_count:stopped) %" PRId64, value);

	return (void *) &value;
}

static void *get_process_count_zombie(void *arg) {
	static int64_t value;

	if (stats.process_count)
		value = stats.process_count->zombie;
	else
		value = 0;

	g_message("Get (process_count:zombie) %" PRId64, value);

	return (void *) &value;
}

static void parse_process_count(xsg_var_t *var) {

	var->type = XSG_INT;

	if (xsg_conf_find_command("total"))
		var->func = get_process_count_total;
	else if (xsg_conf_find_command("running"))
		var->func = get_process_count_running;
	else if (xsg_conf_find_command("sleeping"))
		var->func = get_process_count_sleeping;
	else if (xsg_conf_find_command("stopped"))
		var->func = get_process_count_stopped;
	else if (xsg_conf_find_command("zombie"))
		var->func = get_process_count_zombie;
	else
		xsg_conf_error("total, running, sleeping, stopped or zombie");
}

/******************************************************************************
 *
 * Module functions
 *
 ******************************************************************************/

void init_stats() {
	sg_init();
	sg_snapshot();
}

void shutdown_stats() {
	sg_shutdown();
}

/******************************************************************************/

void parse(xsg_var_t *var, uint16_t id, uint64_t update) {
	static bool first_time = TRUE;

	if (first_time) {
		xsg_main_add_init_func(init_stats);
		xsg_main_add_update_func(update_stats);
		xsg_main_add_shutdown_func(shutdown_stats);
		first_time = FALSE;
	}

	if (xsg_conf_find_command("host_info")){
		parse_host_info(var);
		add_stat(update, get_host_info);
	} else if (xsg_conf_find_command("cpu_stats")) {
		parse_cpu_stats(var);
		add_stat(update, get_cpu_stats);
	} else if (xsg_conf_find_command("cpu_stats_diff")) {
		parse_cpu_stats_diff(var);
		add_stat(update, get_cpu_stats_diff);
	} else if (xsg_conf_find_command("cpu_percents")) {
		parse_cpu_percents(var);
		add_stat(update, get_cpu_percents);
	} else if (xsg_conf_find_command("mem_stats")) {
		parse_mem_stats(var);
		add_stat(update, get_mem_stats);
	} else if (xsg_conf_find_command("load_stats")) {
		parse_load_stats(var);
		add_stat(update, get_load_stats);
	} else if (xsg_conf_find_command("user_stats")) {
		parse_user_stats(var);
		add_stat(update, get_user_stats);
	} else if (xsg_conf_find_command("swap_stats")) {
		parse_swap_stats(var);
		add_stat(update, get_swap_stats);
	} else if (xsg_conf_find_command("fs_stats")) {
		parse_fs_stats(var);
		add_stat(update, get_fs_stats);
	} else if (xsg_conf_find_command("disk_io_stats")) {
		parse_disk_io_stats(var);
		add_stat(update, get_disk_io_stats);
	} else if (xsg_conf_find_command("disk_io_stats_diff")) {
		parse_disk_io_stats_diff(var);
		add_stat(update, get_disk_io_stats_diff);
	} else if (xsg_conf_find_command("network_io_stats")) {
		parse_network_io_stats(var);
		add_stat(update, get_network_io_stats);
	} else if (xsg_conf_find_command("network_io_stats_diff")) {
		parse_network_io_stats_diff(var);
		add_stat(update, get_network_io_stats_diff);
	} else if (xsg_conf_find_command("network_iface_stats")) {
		parse_network_iface_stats(var);
		add_stat(update, get_network_iface_stats);
	} else if (xsg_conf_find_command("page_stats")) {
		parse_page_stats(var);
		add_stat(update, get_page_stats);
	} else if (xsg_conf_find_command("page_stats_diff")) {
		parse_page_stats_diff(var);
		add_stat(update, get_page_stats_diff);
	} else if (xsg_conf_find_command("process_stats")) {
		parse_process_stats(var);
		add_stat(update, get_process_stats);
	} else if (xsg_conf_find_command("process_count")) {
		parse_process_count(var);
		add_stat(update, get_process_count);
	} else {
		xsg_conf_error("host_info, cpu_stats, cpu_stats_diff, cpu_percents, "
				"mem_stats, load_stats, user_stats, swap_stats, fs_stats, "
				"disk_io_stats, disk_io_stats_diff, network_io_stats, "
				"network_io_stats_difff, network_iface_stats, page_stats, "
				"page_stats_diff, process_stats or process_count");
	}
}

char *info() {
	return "interface for libstatgrab";
}

