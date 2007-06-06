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

	if (likely(error == SG_ERROR_NONE))
		return;

	error_str = sg_str_error(error);
	error_arg = sg_get_error_arg();
	errnum = sg_get_error_errno();

	sg_set_error(SG_ERROR_NONE, "");

	if (errnum > 0)
		xsg_warning("libstatgrab: %s: %s: %s", error_str, error_arg, strerror(errnum));
	else
		xsg_warning("libstatgrab: %s: %s", error_str, error_arg);
}

/******************************************************************************
 *
 * libstatgrab functions
 *
 ******************************************************************************/

static void get_host_info() {
	xsg_debug("sg_get_host_info");
	if (!(stats.host_info = sg_get_host_info())) {
		xsg_warning("sg_get_host_info returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_stats() {
	xsg_debug("sg_get_cpu_stats");
	if (!(stats.cpu_stats = sg_get_cpu_stats())) {
		xsg_warning("sg_get_cpu_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_stats_diff() {
	xsg_debug("sg_get_cpu_stats_diff");
	if (!(stats.cpu_stats_diff = sg_get_cpu_stats_diff())) {
		xsg_warning("sg_get_cpu_stats_diff returned NULL");
		libstatgrab_error();
	}
}

static void get_cpu_percents() {
	xsg_debug("sg_get_cpu_percents");
	if (!(stats.cpu_percents = sg_get_cpu_percents())) {
		xsg_warning("sg_get_cpu_percents returned NULL");
		libstatgrab_error();
	}
}

static void get_mem_stats() {
	xsg_debug("sg_get_mem_stats");
	if (!(stats.mem_stats = sg_get_mem_stats())) {
		xsg_warning("sg_get_mem_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_load_stats() {
	xsg_debug("sg_get_load_stats");
	if (!(stats.load_stats = sg_get_load_stats())) {
		xsg_warning("sg_get_load_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_user_stats() {
	xsg_debug("sg_get_user_stats");
	if (!(stats.user_stats = sg_get_user_stats())) {
		xsg_warning("sg_get_user_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_swap_stats() {
	xsg_debug("sg_get_swap_stats");
	if (!(stats.swap_stats = sg_get_swap_stats())) {
		xsg_warning("sg_get_swap_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_fs_stats() {
	size_t size;

	xsg_debug("sg_get_fs_stats");
	if (!(stats.fs_stats_by_device_name = sg_get_fs_stats(&stats.fs_entries))) {
		xsg_warning("sg_get_fs_stats returned NULL");
		libstatgrab_error();
		return;
	}
	size = sizeof(sg_fs_stats) * stats.fs_entries;
	stats.fs_stats_by_mnt_point = (sg_fs_stats *) xsg_realloc((void *) stats.fs_stats_by_mnt_point, size);
	memcpy(stats.fs_stats_by_mnt_point, stats.fs_stats_by_device_name, size);

	qsort(stats.fs_stats_by_device_name, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_device_name);
	qsort(stats.fs_stats_by_mnt_point, stats.fs_entries, sizeof(sg_fs_stats), sg_fs_compare_mnt_point);
}

static void get_disk_io_stats() {
	xsg_debug("sg_get_disk_io_stats");
	if (!(stats.disk_io_stats = sg_get_disk_io_stats(&stats.disk_io_entries))) {
		xsg_warning("sg_get_disk_io_stats returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.disk_io_stats, stats.disk_io_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void get_disk_io_stats_diff() {
	xsg_debug("sg_get_disk_io_stats_diff");
	if (!(stats.disk_io_stats_diff = sg_get_disk_io_stats_diff(&stats.disk_io_diff_entries))) {
		xsg_warning("sg_get_disk_io_stats_diff returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.disk_io_stats_diff, stats.disk_io_diff_entries, sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

static void get_network_io_stats() {
	xsg_debug("sg_get_network_io_stats");
	if (!(stats.network_io_stats = sg_get_network_io_stats(&stats.network_io_entries))) {
		xsg_warning("sg_get_network_io_stats returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_io_stats, stats.network_io_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void get_network_io_stats_diff() {
	xsg_debug("sg_get_network_io_stats_diff");
	if (!(stats.network_io_stats_diff = sg_get_network_io_stats_diff(&stats.network_io_diff_entries))) {
		xsg_warning("sg_get_network_io_stats_diff returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_io_stats_diff, stats.network_io_diff_entries, sizeof(sg_network_io_stats), sg_network_io_compare_name);
}

static void get_network_iface_stats() {
	xsg_debug("sg_get_network_iface_stats");
	if (!(stats.network_iface_stats = sg_get_network_iface_stats(&stats.network_iface_entries))) {
		xsg_warning("sg_get_network_iface_stats returned NULL");
		libstatgrab_error();
		return;
	}
	qsort(stats.network_iface_stats, stats.network_iface_entries, sizeof(sg_network_iface_stats), sg_network_iface_compare_name);
}

static void get_page_stats() {
	xsg_debug("sg_get_page_stats");
	if (!(stats.page_stats = sg_get_page_stats())) {
		xsg_warning("sg_get_page_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_page_stats_diff() {
	xsg_debug("sg_get_page_stats_diff");
	if (!(stats.page_stats_diff = sg_get_page_stats_diff())) {
		xsg_warning("sg_get_page_stats_diff returned NULL");
		libstatgrab_error();
	}
}

static void get_process_stats() {
	xsg_debug("sg_get_process_stats");
	if (stats.process_stats_by_name) {
		xsg_free(stats.process_stats_by_name);
		stats.process_stats_by_name = 0;
	}
	if (stats.process_stats_by_pid) {
		xsg_free(stats.process_stats_by_pid);
		stats.process_stats_by_pid = 0;
	}
	if (stats.process_stats_by_uid) {
		xsg_free(stats.process_stats_by_uid);
		stats.process_stats_by_uid = 0;
	}
	if (stats.process_stats_by_gid) {
		xsg_free(stats.process_stats_by_gid);
		stats.process_stats_by_gid = 0;
	}
	if (stats.process_stats_by_size) {
		xsg_free(stats.process_stats_by_size);
		stats.process_stats_by_size = 0;
	}
	if (stats.process_stats_by_res) {
		xsg_free(stats.process_stats_by_res);
		stats.process_stats_by_res = 0;
	}
	if (stats.process_stats_by_cpu) {
		xsg_free(stats.process_stats_by_cpu);
		stats.process_stats_by_cpu = 0;
	}
	if (stats.process_stats_by_time) {
		xsg_free(stats.process_stats_by_time);
		stats.process_stats_by_time = 0;
	}
	if (!(stats.process_stats = sg_get_process_stats(&stats.process_entries))) {
		xsg_warning("sg_get_process_stats returned NULL");
		libstatgrab_error();
	}
}

static void get_process_count() {
	xsg_debug("sg_get_process_count");
	if (!(stats.process_count = sg_get_process_count())) {
		xsg_warning("sg_get_process_count returned NULL");
		libstatgrab_error();
	}
}

/******************************************************************************/

typedef struct {
	uint64_t update;
	void (*func)();
} stat_t;

xsg_list_t *stat_list = NULL;

static void add_stat(uint64_t update, void (*func)()) {
	stat_t *stat = NULL;
	xsg_list_t *l;

	/* find matching entry in stat_list */
	for (l = stat_list; l; l = l->next) {
		stat = l->data;
		if (stat->func == func) {
			/* is update a multiple of stat->update? */
			if ((update % stat->update) == 0)
				return;
			/* is stat->update a multiple of update? */
			if ((stat->update % update) == 0) {
				stat->update = update;
				return;
			}
		}
	}
	/* no matching entry found in stat_list -> add new entry */
	stat = xsg_new(stat_t, 1);
	stat->update = update;
	stat->func = func;
	stat_list = xsg_list_append(stat_list, stat);
}

/******************************************************************************/

void update_stats(uint64_t tick) {
	stat_t *stat;
	xsg_list_t *l;

	for (l = stat_list; l; l = l->next) {
		stat = l->data;
		if (tick % stat->update == 0)
			stat->func();
	}
}

/******************************************************************************
 *
 * host_info
 *
 ******************************************************************************/

static char *get_host_info_os_name(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_os_name: \"%s\"", stats.host_info->os_name);
		return stats.host_info->os_name;
	} else {
		xsg_debug("get_host_info_os_name: UNKNOWN");
		return NULL;
	}
}

static char *get_host_info_os_release(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_os_release: \"%s\"", stats.host_info->os_release);
		return stats.host_info->os_release;
	} else {
		xsg_debug("get_host_info_os_release: UNKNOWN");
		return NULL;
	}
}

static char *get_host_info_os_version(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_os_version: \"%s\"", stats.host_info->os_version);
		return stats.host_info->os_version;
	} else {
		xsg_debug("get_host_info_os_version: UNKNOWN");
		return NULL;
	}
}

static char *get_host_info_platform(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_platform: \"%s\"", stats.host_info->platform);
		return stats.host_info->platform;
	} else {
		xsg_debug("get_host_info_platform: UNKNOWN");
		return NULL;
	}
}

static char *get_host_info_hostname(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_hostname: \"%s\"", stats.host_info->hostname);
		return stats.host_info->hostname;
	} else {
		xsg_debug("get_host_info_hostname: UNKNOWN");
		return NULL;
	}
}

static double get_host_info_uptime(void *arg) {
	if (likely(stats.host_info != NULL)) {
		xsg_debug("get_host_info_uptime: %f", (double) stats.host_info->uptime);
		return (double) stats.host_info->uptime;
	} else {
		xsg_debug("get_host_info_uptime: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_host_info(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("os_name"))
		*s = get_host_info_os_name;
	else if (xsg_conf_find_command("os_release"))
		*s = get_host_info_os_release;
	else if (xsg_conf_find_command("os_version"))
		*s = get_host_info_os_version;
	else if (xsg_conf_find_command("platform"))
		*s = get_host_info_platform;
	else if (xsg_conf_find_command("hostname"))
		*s = get_host_info_hostname;
	else if (xsg_conf_find_command("uptime"))
		*n = get_host_info_uptime;
	else
		xsg_conf_error("os_name, os_release, os_version, platform, hostname or uptime expected");
}

/******************************************************************************
 *
 * cpu_stats
 *
 ******************************************************************************/

static double get_cpu_stats_user(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_user: %f", (double) stats.cpu_stats->user);
		return (double) stats.cpu_stats->user;
	} else {
		xsg_debug("get_cpu_stats_user: UNKNOWN");
		return DNAN;
	}

}

static double get_cpu_stats_kernel(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_kernel: %f", (double) stats.cpu_stats->kernel);
		return (double) stats.cpu_stats->kernel;
	} else {
		xsg_debug("get_cpu_stats_kernel: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_idle(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_idle: %f", (double) stats.cpu_stats->idle);
		return (double) stats.cpu_stats->idle;
	} else {
		xsg_debug("get_cpu_stats_idle: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_iowait(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_iowait: %f", (double) stats.cpu_stats->iowait);
		return (double) stats.cpu_stats->iowait;
	} else {
		xsg_debug("get_cpu_stats_iowait: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_swap(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_swap: %f", (double) stats.cpu_stats->swap);
		return (double) stats.cpu_stats->swap;
	} else {
		xsg_debug("get_cpu_stats_swap: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_nice(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_nice: %f", (double) stats.cpu_stats->nice);
		return (double) stats.cpu_stats->nice;
	} else {
		xsg_debug("get_cpu_stats_nice: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_total(void *arg) {
	if (likely(stats.cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_total: %f", (double) stats.cpu_stats->total);
		return (double) stats.cpu_stats->total;
	} else {
		xsg_debug("get_cpu_stats_total: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_cpu_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("user"))
		*n = get_cpu_stats_user;
	else if (xsg_conf_find_command("kernel"))
		*n = get_cpu_stats_kernel;
	else if (xsg_conf_find_command("idle"))
		*n = get_cpu_stats_idle;
	else if (xsg_conf_find_command("iowait"))
		*n = get_cpu_stats_iowait;
	else if (xsg_conf_find_command("swap"))
		*n = get_cpu_stats_swap;
	else if (xsg_conf_find_command("nice"))
		*n = get_cpu_stats_nice;
	else if (xsg_conf_find_command("total"))
		*n = get_cpu_stats_total;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or total expected");
}

/******************************************************************************
 *
 * cpu_stats_diff
 *
 ******************************************************************************/

static double get_cpu_stats_diff_user(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_user: %f", (double) stats.cpu_stats_diff->user);
		return (double) stats.cpu_stats_diff->user;
	} else {
		xsg_debug("get_cpu_stats_diff_user: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_kernel(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_kernel: %f", (double) stats.cpu_stats_diff->kernel);
		return (double) stats.cpu_stats_diff->kernel;
	} else {
		xsg_debug("get_cpu_stats_diff_kernel: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_idle(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_idle: %f", (double) stats.cpu_stats_diff->idle);
		return (double) stats.cpu_stats_diff->idle;
	} else {
		xsg_debug("get_cpu_stats_diff_idle: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_iowait(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_iowait: %f", (double) stats.cpu_stats_diff->iowait);
		return (double) stats.cpu_stats_diff->iowait;
	} else {
		xsg_debug("get_cpu_stats_diff_iowait: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_swap(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_swap: %f", (double) stats.cpu_stats_diff->swap);
		return (double) stats.cpu_stats_diff->swap;
	} else {
		xsg_debug("get_cpu_stats_diff_swap: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_nice(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_nice: %f", (double) stats.cpu_stats_diff->nice);
		return (double) stats.cpu_stats_diff->nice;
	} else {
		xsg_debug("get_cpu_stats_diff_nice: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_stats_diff_total(void *arg) {
	if (likely(stats.cpu_stats_diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_total: %f", (double) stats.cpu_stats_diff->total);
		return (double) stats.cpu_stats_diff->total;
	} else {
		xsg_debug("get_cpu_stats_diff_total: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_cpu_stats_diff(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("user"))
		*n = get_cpu_stats_diff_user;
	else if (xsg_conf_find_command("kernel"))
		*n = get_cpu_stats_diff_kernel;
	else if (xsg_conf_find_command("idle"))
		*n = get_cpu_stats_diff_idle;
	else if (xsg_conf_find_command("iowait"))
		*n = get_cpu_stats_diff_iowait;
	else if (xsg_conf_find_command("swap"))
		*n = get_cpu_stats_diff_swap;
	else if (xsg_conf_find_command("nice"))
		*n = get_cpu_stats_diff_nice;
	else if (xsg_conf_find_command("total"))
		*n = get_cpu_stats_diff_total;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or total expected");
}

/******************************************************************************
 *
 * cpu_percents
 *
 ******************************************************************************/

static double get_cpu_percents_user(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_user: %f", stats.cpu_percents->user);
		return stats.cpu_percents->user;
	} else {
		xsg_debug("get_cpu_percents_user: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_percents_kernel(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_kernel: %f", stats.cpu_percents->kernel);
		return stats.cpu_percents->kernel;
	} else {
		xsg_debug("get_cpu_percents_kernel: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_percents_idle(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_idle: %f", stats.cpu_percents->idle);
		return stats.cpu_percents->idle;
	} else {
		xsg_debug("get_cpu_percents_idle: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_percents_iowait(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_iowait: %f", stats.cpu_percents->iowait);
		return stats.cpu_percents->iowait;
	} else {
		xsg_debug("get_cpu_percents_iowait: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_percents_swap(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_swap: %f", stats.cpu_percents->swap);
		return stats.cpu_percents->swap;
	} else {
		xsg_debug("get_cpu_percents_swap: UNKNOWN");
		return DNAN;
	}
}

static double get_cpu_percents_nice(void *arg) {
	if (likely(stats.cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_nice: %f", stats.cpu_percents->nice);
		return stats.cpu_percents->nice;
	} else {
		xsg_debug("get_cpu_percents_nice: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_cpu_percents(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("user"))
		*n = get_cpu_percents_user;
	else if (xsg_conf_find_command("kernel"))
		*n = get_cpu_percents_kernel;
	else if (xsg_conf_find_command("idle"))
		*n = get_cpu_percents_idle;
	else if (xsg_conf_find_command("iowait"))
		*n = get_cpu_percents_iowait;
	else if (xsg_conf_find_command("swap"))
		*n = get_cpu_percents_swap;
	else if (xsg_conf_find_command("nice"))
		*n = get_cpu_percents_nice;
	else
		xsg_conf_error("user, kernel, idle, iowait, swap or nice expected");
}

/******************************************************************************
 *
 * mem_stats
 *
 ******************************************************************************/

static double get_mem_stats_total(void *arg) {
	if (likely(stats.mem_stats != NULL)) {
		xsg_debug("get_cpu_percents_nice: %f", (double) stats.mem_stats->total);
		return (double) stats.mem_stats->total;
	} else {
		xsg_debug("get_cpu_percents_nice: UNKNOWN");
		return DNAN;
	}
}

static double get_mem_stats_free(void *arg) {
	if (likely(stats.mem_stats != NULL)) {
		xsg_debug("get_mem_stats_free: %f", (double) stats.mem_stats->free);
		return (double) stats.mem_stats->free;
	} else {
		xsg_debug("get_mem_stats_free: UNKNOWN");
		return DNAN;
	}
}

static double get_mem_stats_used(void *arg) {
	if (likely(stats.mem_stats != NULL)) {
		xsg_debug("get_mem_stats_used: %f", (double) stats.mem_stats->used);
		return (double) stats.mem_stats->used;
	} else {
		xsg_debug("get_mem_stats_used: UNKNOWN");
		return DNAN;
	}
}

static double get_mem_stats_cache(void *arg) {
	if (likely(stats.mem_stats != NULL)) {
		xsg_debug("get_mem_stats_cache: %f", (double) stats.mem_stats->cache);
		return (double) stats.mem_stats->cache;
	} else {
		xsg_debug("get_mem_stats_cache: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_mem_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("total"))
		*n = get_mem_stats_total;
	else if (xsg_conf_find_command("free"))
		*n = get_mem_stats_free;
	else if (xsg_conf_find_command("used"))
		*n = get_mem_stats_used;
	else if (xsg_conf_find_command("cache"))
		*n = get_mem_stats_cache;
	else
		xsg_conf_error("total, free, used or cache expected");
}

/******************************************************************************
 *
 * load_stats
 *
 ******************************************************************************/

static double get_load_stats_min1(void *arg) {
	if (likely(stats.load_stats != NULL)) {
		xsg_debug("get_load_stats_min1: %f", stats.load_stats->min1);
		return stats.load_stats->min1;
	} else {
		xsg_debug("get_load_stats_min1: UNKNOWN");
		return DNAN;
	}
}

static double get_load_stats_min5(void *arg) {
	if (likely(stats.load_stats != NULL)) {
		xsg_debug("get_load_stats_min5: %f", stats.load_stats->min5);
		return stats.load_stats->min5;
	} else {
		xsg_debug("get_load_stats_min5: UNKNOWN");
		return DNAN;
	}
}

static double get_load_stats_min15(void *arg) {
	if (likely(stats.load_stats != NULL)) {
		xsg_debug("get_load_stats_min15: %f", stats.load_stats->min15);
		return stats.load_stats->min15;
	} else {
		xsg_debug("get_load_stats_min15: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_load_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("min1"))
		*n = get_load_stats_min1;
	else if (xsg_conf_find_command("min5"))
		*n = get_load_stats_min5;
	else if (xsg_conf_find_command("min15"))
		*n = get_load_stats_min15;
	else
		xsg_conf_error("min1, min5 or min15 expected");
}

/******************************************************************************
 *
 * user_stats
 *
 ******************************************************************************/

static char *get_user_stats_name_list(void *arg) {
	if (likely(stats.user_stats != NULL)) {
		xsg_debug("get_user_stats_name_list: \"%s\"", stats.user_stats->name_list);
		return stats.user_stats->name_list;
	} else {
		xsg_debug("get_user_stats_name_list: UNKNOWN");
		return NULL;
	}
}

static double get_user_stats_num_entries(void *arg) {
	if (likely(stats.user_stats != NULL)) {
		xsg_debug("get_user_stats_num_entries: %f", (double) stats.user_stats->num_entries);
		return (double) stats.user_stats->num_entries;
	} else {
		xsg_debug("get_user_stats_num_entries: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_user_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("num_entries"))
		*n = get_user_stats_num_entries;
	else if (xsg_conf_find_command("name_list"))
		*s = get_user_stats_name_list;
	else
		xsg_conf_error("num_entries or name_list expected");
}

/******************************************************************************
 *
 * swap_stats
 *
 ******************************************************************************/

static double get_swap_stats_total(void *arg) {
	if (likely(stats.swap_stats != NULL)) {
		xsg_debug("get_swap_stats_total: %f", (double) stats.swap_stats->total);
		return (double) stats.swap_stats->total;
	} else {
		xsg_debug("get_swap_stats_total: UNKNOWN");
		return DNAN;
	}
}

static double get_swap_stats_used(void *arg) {
	if (likely(stats.swap_stats != NULL)) {
		xsg_debug("get_swap_stats_used: %f", (double) stats.swap_stats->used);
		return (double) stats.swap_stats->used;
	} else {
		xsg_debug("get_swap_stats_used: UNKNOWN");
		return DNAN;
	}
}

static double get_swap_stats_free(void *arg) {
	if (likely(stats.swap_stats != NULL)) {
		xsg_debug("get_swap_stats_free: %f", (double) stats.swap_stats->free);
		return (double) stats.swap_stats->free;
	} else {
		xsg_debug("get_swap_stats_free: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_swap_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("total"))
		*n = get_swap_stats_total;
	else if (xsg_conf_find_command("used"))
		*n = get_swap_stats_used;
	else if (xsg_conf_find_command("free"))
		*n = get_swap_stats_free;
	else
		xsg_conf_error("total, used or free expected");
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

/******************************************************************************/

static char *get_fs_stats_device_name(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_device_name: \"%s\"", ret->device_name);
		return ret->device_name;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_device_name: \"%s\"", ret->device_name);
		return ret->device_name;
	} else {
		xsg_debug("get_fs_stats_device_name: fs_stats for \"%s\" not found", data);
		return NULL;
	}
}

static char *get_fs_stats_fs_type(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_fs_type: \"%s\"", ret->fs_type);
		return ret->fs_type;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_fs_type: \"%s\"", ret->fs_type);
		return ret->fs_type;
	} else {
		xsg_debug("get_fs_stats_fs_type: fs_stats for \"%s\" not found", data);
		return NULL;
	}
}

static char *get_fs_stats_mnt_point(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_mnt_point: \"%s\"", ret->mnt_point);
		return ret->mnt_point;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_mnt_point: \"%s\"", ret->mnt_point);
		return ret->mnt_point;
	} else {
		xsg_debug("get_fs_stats_mnt_point: fs_stats for \"%s\" not found", data);
		return NULL;
	}
}

static double get_fs_stats_size(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_size: %f", (double) ret->size);
		return (double) ret->size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_size: %f", (double) ret->size);
		return (double) ret->size;
	} else {
		xsg_debug("get_fs_stats_size: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_used(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_used: %f", (double) ret->used);
		return (double) ret->used;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_used: %f", (double) ret->used);
		return (double) ret->used;
	} else {
		xsg_debug("get_fs_stats_used: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_avail(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_avail: %f", (double) ret->avail);
		return (double) ret->avail;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_avail: %f", (double) ret->avail);
		return (double) ret->avail;
	} else {
		xsg_debug("get_fs_stats_avail: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_total_inodes(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_total_inodes: %f", (double) ret->total_inodes);
		return (double) ret->total_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_total_inodes: %f", (double) ret->total_inodes);
		return (double) ret->total_inodes;
	} else {
		xsg_debug("get_fs_stats_total_inodes: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_used_inodes(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_used_inodes: %f", (double) ret->used_inodes);
		return (double) ret->used_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_used_inodes: %f", (double) ret->used_inodes);
		return (double) ret->used_inodes;
	} else {
		xsg_debug("get_fs_stats_used_inodes: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_free_inodes(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_free_inodes: %f", (double) ret->free_inodes);
		return (double) ret->free_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_free_inodes: %f", (double) ret->free_inodes);
		return (double) ret->free_inodes;
	} else {
		xsg_debug("get_fs_stats_free_inodes: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_avail_inodes(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_avail_inodes: %f", (double) ret->avail_inodes);
		return (double) ret->avail_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_avail_inodes: %f", (double) ret->avail_inodes);
		return (double) ret->avail_inodes;
	} else {
		xsg_debug("get_fs_stats_avail_inodes: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_io_size(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_io_size: %f", (double) ret->io_size);
		return (double) ret->io_size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_io_size: %f", (double) ret->io_size);
		return (double) ret->io_size;
	} else {
		xsg_debug("get_fs_stats_io_size: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_block_size(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_block_size: %f", (double) ret->block_size);
		return (double) ret->block_size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_block_size: %f", (double) ret->block_size);
		return (double) ret->block_size;
	} else {
		xsg_debug("get_fs_stats_block_size: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_total_blocks(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_total_blocks: %f", (double) ret->total_blocks);
		return (double) ret->total_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_total_blocks: %f", (double) ret->total_blocks);
		return (double) ret->total_blocks;
	} else {
		xsg_debug("get_fs_stats_total_blocks: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_free_blocks(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_free_blocks: %f", (double) ret->free_blocks);
		return (double) ret->free_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_free_blocks: %f", (double) ret->free_blocks);
		return (double) ret->free_blocks;
	} else {
		xsg_debug("get_fs_stats_free_blocks: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_used_blocks(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_used_blocks: %f", (double) ret->used_blocks);
		return (double) ret->used_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_used_blocks: %f", (double) ret->used_blocks);
		return (double) ret->used_blocks;
	} else {
		xsg_debug("get_fs_stats_used_blocks: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

static double get_fs_stats_avail_blocks(void *arg) {
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_avail_blocks: %f", (double) ret->avail_blocks);
		return (double) ret->avail_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_avail_blocks: %f", (double) ret->avail_blocks);
		return (double) ret->avail_blocks;
	} else {
		xsg_debug("get_fs_stats_avail_blocks: fs_stats for \"%s\" not found", data);
		return DNAN;
	}
}

/******************************************************************************/

static void parse_fs_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = (void *) xsg_conf_read_string();
	if (xsg_conf_find_command("device_name"))
		*s = get_fs_stats_device_name;
	else if (xsg_conf_find_command("fs_type"))
		*s = get_fs_stats_fs_type;
	else if (xsg_conf_find_command("mnt_point"))
		*s = get_fs_stats_mnt_point;
	else if (xsg_conf_find_command("size"))
		*n = get_fs_stats_size;
	else if (xsg_conf_find_command("used"))
		*n = get_fs_stats_used;
	else if (xsg_conf_find_command("avail"))
		*n = get_fs_stats_avail;
	else if (xsg_conf_find_command("total_inodes"))
		*n = get_fs_stats_total_inodes;
	else if (xsg_conf_find_command("used_inodes"))
		*n = get_fs_stats_used_inodes;
	else if (xsg_conf_find_command("free_inodes"))
		*n = get_fs_stats_free_inodes;
	else if (xsg_conf_find_command("avail_inodes"))
		*n = get_fs_stats_avail_inodes;
	else if (xsg_conf_find_command("io_size"))
		*n = get_fs_stats_io_size;
	else if (xsg_conf_find_command("block_size"))
		*n = get_fs_stats_block_size;
	else if (xsg_conf_find_command("total_blocks"))
		*n = get_fs_stats_total_blocks;
	else if (xsg_conf_find_command("free_blocks"))
		*n = get_fs_stats_free_blocks;
	else if (xsg_conf_find_command("used_blocks"))
		*n = get_fs_stats_used_blocks;
	else if (xsg_conf_find_command("avail_blocks"))
		*n = get_fs_stats_avail_blocks;
	else
		xsg_conf_error("device_name, fs_type, mnt_point, size, used, avail, "
				"total_inodes, used_inodes, free_inodes, avail_inodes, "
				"io_size, block_size, total_blocks, free_blocks, "
				"used_blocks or avail_blocks expected");
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

/******************************************************************************/

static double get_disk_io_stats_read_bytes(void *arg) {
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_read_bytes: %f", (double) ret->read_bytes);
		return (double) ret->read_bytes;
	} else {
		xsg_debug("get_disk_io_stats_read_bytes: disk_io_stats for \"%s\" not found", disk_name);
		return DNAN;
	}
}

static double get_disk_io_stats_write_bytes(void *arg) {
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_write_bytes: %f", (double) ret->write_bytes);
		return (double) ret->write_bytes;
	} else {
		xsg_debug("get_disk_io_stats_write_bytes: disk_io_stats fpr \"%s\" not found", disk_name);
		return DNAN;
	}
}

/******************************************************************************/

static void parse_disk_io_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = (void *) xsg_conf_read_string();
	if (xsg_conf_find_command("read_bytes"))
		*n = get_disk_io_stats_read_bytes;
	else if (xsg_conf_find_command("write_bytes"))
		*n = get_disk_io_stats_write_bytes;
	else
		xsg_conf_error("read_bytes or write_bytes expected");
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

/******************************************************************************/

static double get_disk_io_stats_diff_read_bytes(void *arg) {
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_diff_read_bytes: %f", (double) ret->read_bytes);
		return (double) ret->read_bytes;
	} else {
		xsg_debug("get_disk_io_stats_diff_read_bytes: disk_io_stats_diff for \"%s\" not found", disk_name);
		return DNAN;
	}
}

static double get_disk_io_stats_diff_write_bytes(void *arg) {
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_diff_write_bytes: %f", (double) ret->write_bytes);
		return (double) ret->write_bytes;
	} else {
		xsg_debug("get_disk_io_stats_diff_write_bytes: disk_io_stats_diff for \"%s\" not found", disk_name);
		return DNAN;
	}
}

/******************************************************************************/

static void parse_disk_io_stats_diff(double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = (void *) xsg_conf_read_string();
	if (xsg_conf_find_command("read_bytes"))
		*n = get_disk_io_stats_diff_read_bytes;
	else if (xsg_conf_find_command("write_bytes"))
		*n = get_disk_io_stats_diff_write_bytes;
	else
		xsg_conf_error("read_bytes or write_bytes expected");
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

/******************************************************************************/

static double get_network_io_stats_tx(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_tx: %f", (double) ret->tx);
		return (double) ret->tx;
	} else {
		xsg_debug("get_network_io_stats_tx: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_rx(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_rx: %f", (double) ret->rx);
		return (double) ret->rx;
	} else {
		xsg_debug("get_network_io_stats_rx: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_ipackets(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_ipackets: %f", (double) ret->ipackets);
		return (double) ret->ipackets;
	} else {
		xsg_debug("get_network_io_stats_ipackets: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_opackets(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_opackets: %f", (double) ret->opackets);
		return (double) ret->opackets;
	} else {
		xsg_debug("get_network_io_stats_opackets: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_ierrors(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_ierrors: %f", (double) ret->ierrors);
		return (double) ret->ierrors;
	} else {
		xsg_debug("get_network_io_stats_ierrors: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_oerrors(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_oerrors: %f", (double) ret->oerrors);
		return (double) ret->oerrors;
	} else {
		xsg_debug("get_network_io_stats_oerrors: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_collisions(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_collisions: %f", (double) ret->collisions);
		return (double) ret->collisions;
	} else {
		xsg_debug("get_network_io_stats_collisions: network_io_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

/******************************************************************************/

static void parse_network_io_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = (void *) xsg_conf_read_string();
	if (xsg_conf_find_command("tx"))
		*n = get_network_io_stats_tx;
	else if (xsg_conf_find_command("rx"))
		*n = get_network_io_stats_rx;
	else if (xsg_conf_find_command("ipackets"))
		*n = get_network_io_stats_ipackets;
	else if (xsg_conf_find_command("opackets"))
		*n = get_network_io_stats_opackets;
	else if (xsg_conf_find_command("ierrors"))
		*n = get_network_io_stats_ierrors;
	else if (xsg_conf_find_command("oerrors"))
		*n = get_network_io_stats_oerrors;
	else if (xsg_conf_find_command("collisions"))
		*n = get_network_io_stats_collisions;
	else
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors or collisions expected");
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

/******************************************************************************/

static double get_network_io_stats_diff_tx(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_tx: %f", (double) ret->tx);
		return (double) ret->tx;
	} else {
		xsg_debug("get_network_io_stats_diff_tx: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_rx(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_rx: %f", (double) ret->rx);
		return (double) ret->rx;
	} else {
		xsg_debug("get_network_io_stats_diff_rx: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_ipackets(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_ipackets: %f", (double) ret->ipackets);
		return (double) ret->ipackets;
	} else {
		xsg_debug("get_network_io_stats_diff_ipackets: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_opackets(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_opackets: %f", (double) ret->opackets);
		return (double) ret->opackets;
	} else {
		xsg_debug("get_network_io_stats_diff_opackets: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_ierrors(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_ierrors: %f", (double) ret->ierrors);
		return (double) ret->ierrors;
	} else {
		xsg_debug("get_network_io_stats_diff_ierrors: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_oerrors(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_oerrors: %f", (double) ret->oerrors);
		return (double) ret->oerrors;
	} else {
		xsg_debug("get_network_io_stats_diff_oerrors: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_io_stats_diff_collisions(void *arg) {
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_diff_collisions: %f", (double) ret->collisions);
		return (double) ret->collisions;
	} else {
		xsg_debug("get_network_io_stats_diff_collisions: network_io_stats_diff for \"%s\" not found", interface_name);
		return DNAN;
	}
}

/******************************************************************************/

static void parse_network_io_stats_diff(double (**n)(void *), char *(**s)(void *), void **arg) {
	*arg = (void *) xsg_conf_read_string();
	if (xsg_conf_find_command("tx"))
		*n = get_network_io_stats_diff_tx;
	else if (xsg_conf_find_command("rx"))
		*n = get_network_io_stats_diff_rx;
	else if (xsg_conf_find_command("ipackets"))
		*n = get_network_io_stats_diff_ipackets;
	else if (xsg_conf_find_command("opackets"))
		*n = get_network_io_stats_diff_opackets;
	else if (xsg_conf_find_command("ierrors"))
		*n = get_network_io_stats_diff_ierrors;
	else if (xsg_conf_find_command("oerrors"))
		*n = get_network_io_stats_diff_oerrors;
	else if (xsg_conf_find_command("collisions"))
		*n = get_network_io_stats_diff_collisions;
	else
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors or collisions expected");
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

/******************************************************************************/

static double get_network_iface_stats_speed(void *arg) {
	sg_network_iface_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_speed: %f", (double) ret->speed);
		return (double) ret->speed;
	} else {
		xsg_debug("get_network_iface_stats_speed: network_iface_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double get_network_iface_stats_duplex_number(void *arg) {
	sg_network_iface_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_duplex_number: %f", (double) ret->duplex);
		return (double) ret->duplex;
	} else {
		xsg_debug("get_network_iface_stats_duplex_number: network_iface_stats for \"%s\" not found", interface_name);
		return DNAN;
	}
}

typedef struct {
	char *interface_name;
	char *full;
	char *half;
	char *unknown;
} network_iface_stats_duplex_data_t;

static char *get_network_iface_stats_duplex_string(void *arg) {
	sg_network_iface_stats *ret;
	network_iface_stats_duplex_data_t *data;
	char *interface_name;

	data = (network_iface_stats_duplex_data_t *) arg;
	interface_name = data->interface_name;

	if (!(ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_duplex_string: network_iface_stats for \"%s\" not found", interface_name);
		return NULL;
	}
	if (ret->duplex == SG_IFACE_DUPLEX_FULL) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"", data->full);
		return data->full;
	} else if (ret->duplex == SG_IFACE_DUPLEX_HALF) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"", data->half);
		return data->half;
	} else if (ret->duplex == SG_IFACE_DUPLEX_UNKNOWN) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"", data->unknown);
		return data->unknown;
	} else {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"", data->unknown);
		return data->unknown;
	}
}

/******************************************************************************/

static void parse_network_iface_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	char *interface_name;

	interface_name = xsg_conf_read_string();

	if (xsg_conf_find_command("speed")) {
		*n = get_network_iface_stats_speed;
		*arg = (void *) interface_name;
	} else if (xsg_conf_find_command("duplex")) {
		if (xsg_conf_find_command("n")) {
			*n = get_network_iface_stats_duplex_number;
			*arg = (void *) interface_name;
		} else if (xsg_conf_find_command("s")) {
			network_iface_stats_duplex_data_t *data;

			data = xsg_new(network_iface_stats_duplex_data_t, 1);
			data->interface_name = interface_name;
			data->full = xsg_conf_read_string();
			data->half = xsg_conf_read_string();
			data->unknown = xsg_conf_read_string();

			*s = get_network_iface_stats_duplex_string;
			*arg = (void *) data;
		} else {
			xsg_conf_error("n or s expected");
		}
	} else {
		xsg_conf_error("speed or duplex expected");
	}
}

/******************************************************************************
 *
 * page_stats
 *
 ******************************************************************************/

static double get_page_stats_pages_pagein(void *arg) {
	if (likely(stats.page_stats != NULL)) {
		xsg_debug("get_page_stats_pages_pagein: %f", (double) stats.page_stats->pages_pagein);
		return (double) stats.page_stats->pages_pagein;
	} else {
		xsg_debug("get_page_stats_pages_pagein: UNKNOWN");
		return DNAN;
	}
}

static double get_page_stats_pages_pageout(void *arg) {
	if (likely(stats.page_stats != NULL)) {
		xsg_debug("get_page_stats_pages_pageout: %f", (double) stats.page_stats->pages_pageout);
		return (double) stats.page_stats->pages_pageout;
	} else {
		xsg_debug("get_page_stats_pages_pageout: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_page_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("pages_pagein"))
		*n = get_page_stats_pages_pagein;
	else if (xsg_conf_find_command("pages_pageout"))
		*n = get_page_stats_pages_pageout;
	else
		xsg_conf_error("pages_pagein or pages_pageout expected");
}

/******************************************************************************
 *
 * page_stats_diff
 *
 ******************************************************************************/

static double get_page_stats_diff_pages_pagein(void *arg) {
	if (likely(stats.page_stats != NULL)) {
		xsg_debug("get_page_stats_diff_pages_pagein: %f", (double) stats.page_stats->pages_pagein);
		return (double) stats.page_stats->pages_pagein;
	} else {
		xsg_debug("get_page_stats_diff_pages_pagein: UNKNOWN");
		return DNAN;
	}
}

static double get_page_stats_diff_pages_pageout(void *arg) {
	if (likely(stats.page_stats != NULL)) {
		xsg_debug("get_page_stats_diff_pages_pageout: %f", (double) stats.page_stats->pages_pageout);
		return (double) stats.page_stats->pages_pageout;
	} else {
		xsg_debug("get_page_stats_diff_pages_pageout: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_page_stats_diff(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("pages_pagein"))
		*n = get_page_stats_diff_pages_pagein;
	else if (xsg_conf_find_command("pages_pageout"))
		*n = get_page_stats_diff_pages_pageout;
	else
		xsg_conf_error("pages_pagein or pages_pageout expected");
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
	stats.process_stats_by_name = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_name, stats.process_stats, size);
	qsort(stats.process_stats_by_name, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_name);
	return stats.process_stats_by_name;
}

sg_process_stats *get_process_stats_orderedby_pid() {
	size_t size;

	if (stats.process_stats_by_pid)
		return stats.process_stats_by_pid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_pid = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_pid, stats.process_stats, size);
	qsort(stats.process_stats_by_pid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_pid);
	return stats.process_stats_by_pid;
}

sg_process_stats *get_process_stats_orderedby_uid() {
	size_t size;

	if (stats.process_stats_by_uid)
		return stats.process_stats_by_uid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_uid = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_uid, stats.process_stats, size);
	qsort(stats.process_stats_by_uid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_uid);
	return stats.process_stats_by_uid;
}

sg_process_stats *get_process_stats_orderedby_gid() {
	size_t size;

	if (stats.process_stats_by_gid)
		return stats.process_stats_by_gid;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_gid = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_gid, stats.process_stats, size);
	qsort(stats.process_stats_by_gid, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_gid);
	return stats.process_stats_by_gid;
}

sg_process_stats *get_process_stats_orderedby_size() {
	size_t size;

	if (stats.process_stats_by_size)
		return stats.process_stats_by_size;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_size = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_size, stats.process_stats, size);
	qsort(stats.process_stats_by_size, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_size);
	return stats.process_stats_by_size;
}

sg_process_stats *get_process_stats_orderedby_res() {
	size_t size;

	if (stats.process_stats_by_res)
		return stats.process_stats_by_res;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_res = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_res, stats.process_stats, size);
	qsort(stats.process_stats_by_res, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_res);
	return stats.process_stats_by_res;
}

sg_process_stats *get_process_stats_orderedby_cpu() {
	size_t size;

	if (stats.process_stats_by_cpu)
		return stats.process_stats_by_cpu;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_cpu = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_cpu, stats.process_stats, size);
	qsort(stats.process_stats_by_cpu, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_cpu);
	return stats.process_stats_by_cpu;
}

sg_process_stats *get_process_stats_orderedby_time() {
	size_t size;

	if (stats.process_stats_by_time)
		return stats.process_stats_by_time;

	size = sizeof(sg_process_stats) * stats.process_entries;
	stats.process_stats_by_time = (sg_process_stats *) xsg_malloc(size);
	memcpy(stats.process_stats_by_time, stats.process_stats, size);
	qsort(stats.process_stats_by_time, stats.process_entries, sizeof(sg_process_stats), sg_process_compare_time);
	return stats.process_stats_by_time;
}

/******************************************************************************/

typedef struct {
	sg_process_stats *(*list_func)();
	bool ascending;
	uint32_t number;
} process_stats_data_t;

typedef struct {
	sg_process_stats *(*list_func)();
	bool ascending;
	uint32_t number;
	char *running;
	char *sleeping;
	char *stopped;
	char *zombie;
	char *unknown;
} process_stats_state_data_t;

static char *get_process_stats_process_name(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_process_name: process_stats for number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_process_name: \"%s\"", sg->process_name);

	return sg->process_name;
}

static char *get_process_stats_proctitle(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_proctitle: process_stats for number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proctitle: \"%s\"", sg->proctitle);

	return sg->proctitle;
}

static double get_process_stats_pid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_pid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_pid: %f", (double) sg->pid);

	return (double) sg->pid;
}

static double get_process_stats_parent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_parent: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_parent: %f", (double) sg->parent);

	return (double) sg->parent;
}

static double get_process_stats_pgid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_pgid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_pgid: %f", (double) sg->pgid);

	return (double) sg->pgid;
}

static double get_process_stats_uid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_uid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_uid: %f", (double) sg->uid);

	return (double) sg->uid;
}

static double get_process_stats_euid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_euid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_euid: %f", (double) sg->euid);

	return (double) sg->euid;
}

static double get_process_stats_gid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_gid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_gid: %f", (double) sg->gid);

	return (double) sg->gid;
}

static double get_process_stats_egid(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_egid: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_egid: %f", (double) sg->egid);

	return (double) sg->egid;
}

static double get_process_stats_proc_size(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_proc_size: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proc_size: %f", (double) sg->proc_size);

	return (double) sg->proc_size;
}

static double get_process_stats_proc_resident(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_proc_resident: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proc_resident: %f", (double) sg->proc_resident);

	return (double) sg->proc_resident;
}

static double get_process_stats_time_spent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_time_spent: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_time_spent: %f", (double) sg->time_spent);

	return (double) sg->time_spent;
}

static double get_process_stats_cpu_percent(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_cpu_percent: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_cpu_percent: %f", sg->cpu_percent);

	return sg->cpu_percent;
}

static double get_process_stats_nice(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_nice: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_nice: %f", (double) sg->nice);

	return (double) sg->nice;
}

static double get_process_stats_state_number(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_state_number: process_stats for number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_state_number: %f", (double) sg->state);

	return (double) sg->state;
}

static char *get_process_stats_state_string(void *arg) {
	sg_process_stats *sg_list, *sg;
	process_stats_state_data_t *data;
	uint32_t index;

	data = (process_stats_state_data_t *) arg;

	if (data->number >= stats.process_entries) {
		xsg_debug("get_process_stats_state_string: process_stats for number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending)
		index = data->number;
	else
		index = stats.process_entries - data->number;

	sg_list = (data->list_func)();
	sg = sg_list + index;

	switch (sg->state) {
		case SG_PROCESS_STATE_RUNNING:
			xsg_debug("get_process_stats_state_string: \"%s\"", data->running);
			return data->running;
		case SG_PROCESS_STATE_SLEEPING:
			xsg_debug("get_process_stats_state_string: \"%s\"", data->sleeping);
			return data->sleeping;
		case SG_PROCESS_STATE_STOPPED:
			xsg_debug("get_process_stats_state_string: \"%s\"", data->stopped);
			return data->stopped;
		case SG_PROCESS_STATE_ZOMBIE:
			xsg_debug("get_process_stats_state_string: \"%s\"", data->zombie);
			return data->zombie;
		case SG_PROCESS_STATE_UNKNOWN:
		default:
			xsg_debug("get_process_stats_state_string: \"%s\"", data->unknown);
			return data->unknown;
	}
}

/******************************************************************************/

static void parse_process_stats(double (**n)(void *), char *(**s)(void *), void **arg) {
	sg_process_stats *(*list_func)() = NULL;
	bool ascending = TRUE;
	uint32_t number = 0;
	bool state_command = FALSE;

	if (xsg_conf_find_command("process_name"))
		*s = get_process_stats_process_name;
	else if (xsg_conf_find_command("proctitle"))
		*s = get_process_stats_proctitle;
	else if (xsg_conf_find_command("pid"))
		*n = get_process_stats_pid;
	else if (xsg_conf_find_command("parent"))
		*n = get_process_stats_parent;
	else if (xsg_conf_find_command("pgid"))
		*n = get_process_stats_pgid;
	else if (xsg_conf_find_command("uid"))
		*n = get_process_stats_uid;
	else if (xsg_conf_find_command("euid"))
		*n = get_process_stats_euid;
	else if (xsg_conf_find_command("gid"))
		*n = get_process_stats_gid;
	else if (xsg_conf_find_command("egid"))
		*n = get_process_stats_egid;
	else if (xsg_conf_find_command("proc_size"))
		*n = get_process_stats_proc_size;
	else if (xsg_conf_find_command("proc_resident"))
		*n = get_process_stats_proc_resident;
	else if (xsg_conf_find_command("time_spent"))
		*n = get_process_stats_time_spent;
	else if (xsg_conf_find_command("cpu_percent"))
		*n = get_process_stats_cpu_percent;
	else if (xsg_conf_find_command("nice"))
		*n = get_process_stats_nice;
	else if (xsg_conf_find_command("state"))
		state_command = TRUE;
	else
		xsg_conf_error("process_name, proctitle, pid, parent, pgid, uid, euid, gid, "
				"egid, proc_size, proc_resident, time_spent, cpu_percent, "
				"nice or state expected");

	if (xsg_conf_find_command("name"))
		list_func = get_process_stats_orderedby_name;
	else if (xsg_conf_find_command("pid"))
		list_func = get_process_stats_orderedby_pid;
	else if (xsg_conf_find_command("uid"))
		list_func = get_process_stats_orderedby_uid;
	else if (xsg_conf_find_command("gid"))
		list_func = get_process_stats_orderedby_gid;
	else if (xsg_conf_find_command("size"))
		list_func = get_process_stats_orderedby_size;
	else if (xsg_conf_find_command("res"))
		list_func = get_process_stats_orderedby_res;
	else if (xsg_conf_find_command("cpu"))
		list_func = get_process_stats_orderedby_cpu;
	else if (xsg_conf_find_command("time"))
		list_func = get_process_stats_orderedby_time;
	else
		xsg_conf_error("name, pid, uid, gid, size, res, cpu or time expected");

	if (xsg_conf_find_command("ascending"))
		ascending = TRUE;
	else if (xsg_conf_find_command("descending"))
		ascending = FALSE;
	else
		xsg_conf_error("ascending or descending expected");

	number = xsg_conf_read_uint();

	if (state_command) {
		if (xsg_conf_find_command("n"))
			*n = get_process_stats_state_number;
		else if (xsg_conf_find_command("s"))
			*s = get_process_stats_state_string;
		else
			xsg_conf_error("n or s expected");
	}

	if (*s == get_process_stats_state_string) {
		process_stats_state_data_t *data;
		data = xsg_new(process_stats_state_data_t, 1);
		data->list_func = list_func;
		data->ascending = ascending;
		data->number = number;
		data->running = xsg_conf_read_string();
		data->sleeping = xsg_conf_read_string();
		data->stopped = xsg_conf_read_string();
		data->zombie = xsg_conf_read_string();
		data->unknown = xsg_conf_read_string();
	} else {
		process_stats_data_t *data;
		data = xsg_new(process_stats_data_t, 1);
		data->list_func = list_func;
		data->ascending = ascending;
		data->number = number;
	}
}

/******************************************************************************
 *
 * process_count
 *
 ******************************************************************************/

static double get_process_count_total(void *arg) {
	if (likely(stats.process_count != NULL)) {
		xsg_debug("get_process_count_total: %f", (double) stats.process_count->total);
		return (double) stats.process_count->total;
	} else {
		xsg_debug("get_process_count_total: UNKNOWN");
		return DNAN;
	}
}

static double get_process_count_running(void *arg) {
	if (likely(stats.process_count != NULL)) {
		xsg_debug("get_process_count_running: %f", (double) stats.process_count->running);
		return (double) stats.process_count->running;
	} else {
		xsg_debug("get_process_count_running: UNKNOWN");
		return DNAN;
	}
}

static double get_process_count_sleeping(void *arg) {
	if (likely(stats.process_count != NULL)) {
		xsg_debug("get_process_count_sleeping: %f", (double) stats.process_count->sleeping);
		return (double) stats.process_count->sleeping;
	} else {
		xsg_debug("get_process_count_sleeping: UNKNOWN");
		return DNAN;
	}
}

static double get_process_count_stopped(void *arg) {
	if (likely(stats.process_count != NULL)) {
		xsg_debug("get_process_count_stopped: %f", (double) stats.process_count->stopped);
		return (double) stats.process_count->stopped;
	} else {
		xsg_debug("get_process_count_stopped: UNKNOWN");
		return DNAN;
	}
}

static double get_process_count_zombie(void *arg) {
	if (likely(stats.process_count != NULL)) {
		xsg_debug("get_process_count_zombie: %f", (double) stats.process_count->zombie);
		return (double) stats.process_count->zombie;
	} else {
		xsg_debug("get_process_count_zombie: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void parse_process_count(double (**n)(void *), char *(**s)(void *), void **arg) {
	if (xsg_conf_find_command("total"))
		*n = get_process_count_total;
	else if (xsg_conf_find_command("running"))
		*n = get_process_count_running;
	else if (xsg_conf_find_command("sleeping"))
		*n = get_process_count_sleeping;
	else if (xsg_conf_find_command("stopped"))
		*n = get_process_count_stopped;
	else if (xsg_conf_find_command("zombie"))
		*n = get_process_count_zombie;
	else
		xsg_conf_error("total, running, sleeping, stopped or zombie expected");
}

/******************************************************************************
 *
 * Module functions
 *
 ******************************************************************************/

static void init_stats(void) {
	xsg_message("Running sg_init()");
	sg_init();
	xsg_message("Running sg_snapshot()");
	sg_snapshot();
	update_stats(0);
}

static void shutdown_stats(void) {
	xsg_message("Running sg_shutdown()");
	sg_shutdown();
}

/******************************************************************************/

void parse(uint64_t update, xsg_var_t *var, double (**n)(void *), char *(**s)(void *), void **arg) {
	xsg_main_add_init_func(init_stats);
	xsg_main_add_update_func(update_stats);
	xsg_main_add_shutdown_func(shutdown_stats);

	if (xsg_conf_find_command("host_info")){
		parse_host_info(n ,s, arg);
		add_stat(update, get_host_info);
	} else if (xsg_conf_find_command("cpu_stats")) {
		parse_cpu_stats(n, s, arg);
		add_stat(update, get_cpu_stats);
	} else if (xsg_conf_find_command("cpu_stats_diff")) {
		parse_cpu_stats_diff(n, s, arg);
		add_stat(update, get_cpu_stats_diff);
	} else if (xsg_conf_find_command("cpu_percents")) {
		parse_cpu_percents(n, s, arg);
		add_stat(update, get_cpu_percents);
	} else if (xsg_conf_find_command("mem_stats")) {
		parse_mem_stats(n, s, arg);
		add_stat(update, get_mem_stats);
	} else if (xsg_conf_find_command("load_stats")) {
		parse_load_stats(n, s, arg);
		add_stat(update, get_load_stats);
	} else if (xsg_conf_find_command("user_stats")) {
		parse_user_stats(n, s, arg);
		add_stat(update, get_user_stats);
	} else if (xsg_conf_find_command("swap_stats")) {
		parse_swap_stats(n, s, arg);
		add_stat(update, get_swap_stats);
	} else if (xsg_conf_find_command("fs_stats")) {
		parse_fs_stats(n, s, arg);
		add_stat(update, get_fs_stats);
	} else if (xsg_conf_find_command("disk_io_stats")) {
		parse_disk_io_stats(n, s, arg);
		add_stat(update, get_disk_io_stats);
	} else if (xsg_conf_find_command("disk_io_stats_diff")) {
		parse_disk_io_stats_diff(n, s, arg);
		add_stat(update, get_disk_io_stats_diff);
	} else if (xsg_conf_find_command("network_io_stats")) {
		parse_network_io_stats(n, s, arg);
		add_stat(update, get_network_io_stats);
	} else if (xsg_conf_find_command("network_io_stats_diff")) {
		parse_network_io_stats_diff(n, s, arg);
		add_stat(update, get_network_io_stats_diff);
	} else if (xsg_conf_find_command("network_iface_stats")) {
		parse_network_iface_stats(n, s, arg);
		add_stat(update, get_network_iface_stats);
	} else if (xsg_conf_find_command("page_stats")) {
		parse_page_stats(n, s, arg);
		add_stat(update, get_page_stats);
	} else if (xsg_conf_find_command("page_stats_diff")) {
		parse_page_stats_diff(n, s, arg);
		add_stat(update, get_page_stats_diff);
	} else if (xsg_conf_find_command("process_stats")) {
		parse_process_stats(n, s, arg);
		add_stat(update, get_process_stats);
	} else if (xsg_conf_find_command("process_count")) {
		parse_process_count(n, s, arg);
		add_stat(update, get_process_count);
	} else {
		xsg_conf_error("host_info, cpu_stats, cpu_stats_diff, cpu_percents, "
				"mem_stats, load_stats, user_stats, swap_stats, fs_stats, "
				"disk_io_stats, disk_io_stats_diff, network_io_stats, "
				"network_io_stats_difff, network_iface_stats, page_stats, "
				"page_stats_diff, process_stats or process_count expected");
	}
}

char *info() {
	return "interface for libstatgrab";
}

