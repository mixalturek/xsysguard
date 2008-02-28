/* statgrab.c
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
#include <statgrab.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/******************************************************************************/

typedef struct _cpu_stats_diff_t {
	uint64_t update;
	sg_cpu_stats *diff;
} cpu_stats_diff_t;

typedef struct _disk_io_stats_diff_t {
	uint64_t update;
	int entries;
	sg_disk_io_stats *diff;
} disk_io_stats_diff_t;

typedef struct _network_io_stats_diff_t {
	uint64_t update;
	int entries;
	sg_network_io_stats *diff;
} network_io_stats_diff_t;

typedef struct _page_stats_diff_t {
	uint64_t update;
	sg_page_stats *diff;
} page_stats_diff_t;

/******************************************************************************/

static sg_host_info *host_info = NULL;
static sg_cpu_stats *cpu_stats = NULL;
static xsg_list_t *cpu_stats_diff_list = NULL;
static sg_cpu_percents *cpu_percents = NULL;
static sg_mem_stats *mem_stats = NULL;
static sg_load_stats *load_stats = NULL;
static sg_user_stats *user_stats = NULL;
static sg_swap_stats *swap_stats = NULL;
static int fs_entries = 0;
static sg_fs_stats *fs_stats_by_device_name = NULL;
static sg_fs_stats *fs_stats_by_mnt_point = NULL;
static int disk_io_entries = 0;
static sg_disk_io_stats * disk_io_stats = NULL;
static xsg_list_t *disk_io_stats_diff_list = NULL;
static int network_io_entries = 0;
static sg_network_io_stats *network_io_stats = NULL;
static xsg_list_t *network_io_stats_diff_list = NULL;
static int network_iface_entries = 0;
static sg_network_iface_stats *network_iface_stats = NULL;
static sg_page_stats *page_stats = NULL;
static xsg_list_t *page_stats_diff_list = NULL;
static int process_entries = 0;
static sg_process_stats *process_stats = NULL;
static sg_process_stats *process_stats_by_name = NULL;
static sg_process_stats *process_stats_by_pid = NULL;
static sg_process_stats *process_stats_by_uid = NULL;
static sg_process_stats *process_stats_by_gid = NULL;
static sg_process_stats *process_stats_by_size = NULL;
static sg_process_stats *process_stats_by_res = NULL;
static sg_process_stats *process_stats_by_cpu = NULL;
static sg_process_stats *process_stats_by_time = NULL;
static sg_process_count *process_count = NULL;

/******************************************************************************
 *
 * handle libstatgrab errors
 *
 ******************************************************************************/

static void
libstatgrab_error(void)
{
	sg_error error;
	const char *error_str;
	const char *error_arg;
	int errnum;

	error = sg_get_error();

	if (likely(error == SG_ERROR_NONE)) {
		return;
	}

	error_str = sg_str_error(error);
	error_arg = sg_get_error_arg();
	errnum = sg_get_error_errno();

	sg_set_error(SG_ERROR_NONE, "");

	if (errnum > 0) {
		xsg_warning("libstatgrab: %s: %s: %s", error_str, error_arg,
				strerror(errnum));
	} else {
		xsg_warning("libstatgrab: %s: %s", error_str, error_arg);
	}
}

/******************************************************************************
 *
 * libstatgrab functions
 *
 ******************************************************************************/

static void
get_host_info(uint64_t tick)
{
	xsg_debug("sg_get_host_info");
	if (!(host_info = sg_get_host_info())) {
		xsg_warning("%s returned NULL", "sg_get_host_info");
		libstatgrab_error();
	}
}

static void
get_cpu_stats(uint64_t tick)
{
	xsg_debug("sg_get_cpu_stats");
	if (!(cpu_stats = sg_get_cpu_stats())) {
		xsg_warning("%s returned NULL", "sg_get_cpu_stats");
		libstatgrab_error();
	}
}

static void
get_cpu_stats_diff(uint64_t tick)
{
	sg_cpu_stats *diff;
	xsg_list_t *l;

	xsg_debug("sg_get_cpu_stats_diff");
	if (!(diff = sg_get_cpu_stats_diff())) {
		xsg_warning("%s returned NULL", "sg_get_cpu_stats_diff");
		libstatgrab_error();
	}

	for (l = cpu_stats_diff_list; l; l = l->next) {
		cpu_stats_diff_t *d = l->data;

		if (tick % d->update == 0) {
			d->diff->user = 0;
			d->diff->kernel = 0;
			d->diff->idle = 0;
			d->diff->iowait = 0;
			d->diff->swap = 0;
			d->diff->nice = 0;
			d->diff->total = 0;
			d->diff->systime = 0;
		}

		d->diff->user += diff->user;
		d->diff->kernel += diff->kernel;
		d->diff->idle += diff->idle;
		d->diff->iowait += diff->iowait;
		d->diff->swap += diff->swap;
		d->diff->nice += diff->nice;
		d->diff->total += diff->total;
		d->diff->systime += diff->systime;
	}
}

static void
get_cpu_percents(uint64_t tick)
{
	xsg_debug("sg_get_cpu_percents");
	if (!(cpu_percents = sg_get_cpu_percents())) {
		xsg_warning("%s returned NULL", "sg_get_cpu_percents");
		libstatgrab_error();
	}
}

static void
get_mem_stats(uint64_t tick)
{
	xsg_debug("sg_get_mem_stats");
	if (!(mem_stats = sg_get_mem_stats())) {
		xsg_warning("%s returned NULL", "sg_get_mem_stats");
		libstatgrab_error();
	}
}

static void
get_load_stats(uint64_t tick)
{
	xsg_debug("sg_get_load_stats");
	if (!(load_stats = sg_get_load_stats())) {
		xsg_warning("%s returned NULL", "sg_get_load_stats");
		libstatgrab_error();
	}
}

static void
get_user_stats(uint64_t tick)
{
	xsg_debug("sg_get_user_stats");
	if (!(user_stats = sg_get_user_stats())) {
		xsg_warning("%s returned NULL", "sg_get_user_stats");
		libstatgrab_error();
	}
}

static void
get_swap_stats(uint64_t tick)
{
	xsg_debug("sg_get_swap_stats");
	if (!(swap_stats = sg_get_swap_stats())) {
		xsg_warning("%s returned NULL", "sg_get_swap_stats");
		libstatgrab_error();
	}
}

static void
get_fs_stats(uint64_t tick)
{
	size_t size;

	xsg_debug("sg_get_fs_stats");
	if (!(fs_stats_by_device_name = sg_get_fs_stats(&fs_entries))) {
		xsg_warning("%s returned NULL", "sg_get_fs_stats");
		libstatgrab_error();
		return;
	}
	size = sizeof(sg_fs_stats) * fs_entries;
	fs_stats_by_mnt_point = (sg_fs_stats *) xsg_realloc(
			(void *) fs_stats_by_mnt_point, size);
	memcpy(fs_stats_by_mnt_point, fs_stats_by_device_name, size);

	qsort(fs_stats_by_device_name, fs_entries, sizeof(sg_fs_stats),
			sg_fs_compare_device_name);
	qsort(fs_stats_by_mnt_point, fs_entries, sizeof(sg_fs_stats),
			sg_fs_compare_mnt_point);
}

static void
get_disk_io_stats(uint64_t tick)
{
	xsg_debug("sg_get_disk_io_stats");
	if (!(disk_io_stats = sg_get_disk_io_stats(&disk_io_entries))) {
		xsg_warning("%s returned NULL", "sg_get_disk_io_stats");
		libstatgrab_error();
		return;
	}
	qsort(disk_io_stats, disk_io_entries, sizeof(sg_disk_io_stats),
			sg_disk_io_compare_name);
}

static void
get_disk_io_stats_diff(uint64_t tick)
{
	sg_disk_io_stats *diff;
	int entries;
	xsg_list_t *l;

	xsg_debug("sg_get_disk_io_stats_diff");
	if (!(diff = sg_get_disk_io_stats_diff(&entries))) {
		xsg_warning("%s returned NULL", "sg_get_disk_io_stats_diff");
		libstatgrab_error();
		return;
	}

	for (l = disk_io_stats_diff_list; l; l = l->next) {
		disk_io_stats_diff_t *d = l->data;
		sg_disk_io_stats *old_diff = d->diff;
		int old_entries = d->entries;
		int i, j;

		d->diff = xsg_new(sg_disk_io_stats, entries);

		for (i = 0; i < entries; i++) {
			d->diff[i].disk_name = xsg_strdup(diff[i].disk_name);
			d->diff[i].read_bytes = diff[i].read_bytes;
			d->diff[i].write_bytes = diff[i].write_bytes;
			d->diff[i].systime = diff[i].systime;
		}

		if (tick % d->update != 0) {
			for (i = 0; i < entries; i++) {
				sg_disk_io_stats *now, *old = NULL;

				now = &d->diff[i];

				for (j = 0; j < old_entries; j++) {
					old = &old_diff[(i + j) % old_entries];

					if (!strcmp(now->disk_name,
							old->disk_name)) {
						break;
					}
				}

				if (old == NULL) {
					continue;
				}

				now->read_bytes += old->read_bytes;
				now->write_bytes += old->write_bytes;
				now->systime += old->systime;
			}
		}

		for (i = 0; i < old_entries; i++) {
			xsg_free(old_diff[i].disk_name);
		}

		xsg_free(old_diff);

		qsort(d->diff, entries, sizeof(sg_disk_io_stats),
				sg_disk_io_compare_name);
	}
}

static void
get_network_io_stats(uint64_t tick)
{
	xsg_debug("sg_get_network_io_stats");
	if (!(network_io_stats = sg_get_network_io_stats(&network_io_entries))) {
		xsg_warning("%s returned NULL", "sg_get_network_io_stats");
		libstatgrab_error();
		return;
	}
	qsort(network_io_stats, network_io_entries, sizeof(sg_network_io_stats),
			sg_network_io_compare_name);
}

static void
get_network_io_stats_diff(uint64_t tick)
{
	sg_network_io_stats *diff;
	int entries;
	xsg_list_t *l;

	xsg_debug("sg_get_network_io_stats_diff");
	if (!(diff = sg_get_network_io_stats_diff(&entries))) {
		xsg_warning("%s returned NULL", "sg_get_network_io_stats_diff");
		libstatgrab_error();
		return;
	}

	for (l = network_io_stats_diff_list; l; l = l->next) {
		network_io_stats_diff_t *d = l->data;
		sg_network_io_stats *old_diff = d->diff;
		int old_entries = d->entries;
		int i, j;

		d->diff = xsg_new(sg_network_io_stats, entries);

		for (i = 0; i < entries; i++) {
			d->diff[i].interface_name
				= xsg_strdup(diff[i].interface_name);
			d->diff[i].rx = diff[i].rx;
			d->diff[i].tx = diff[i].tx;
			d->diff[i].ipackets = diff[i].ipackets;
			d->diff[i].opackets = diff[i].opackets;
			d->diff[i].ierrors = diff[i].ierrors;
			d->diff[i].oerrors = diff[i].oerrors;
			d->diff[i].collisions = diff[i].collisions;
			d->diff[i].systime = diff[i].systime;
		}

		if (tick % d->update != 0) {
			for (i = 0; i < entries; i++) {
				sg_network_io_stats *now, *old = NULL;

				now = &d->diff[i];

				for (j = 0; j < old_entries; j++) {
					old = &old_diff[(i + j) % old_entries];

					if (!strcmp(now->interface_name,
							old->interface_name)) {
						break;
					}
				}

				if (old == NULL) {
					continue;
				}

				now->rx += old->rx;
				now->tx += old->tx;
				now->ipackets += old->ipackets;
				now->opackets += old->opackets;
				now->ierrors += old->ierrors;
				now->oerrors += old->oerrors;
				now->collisions += old->collisions;
				now->systime += old->systime;
			}
		}

		for (i = 0; i < old_entries; i++) {
			xsg_free(old_diff[i].interface_name);
		}

		xsg_free(old_diff);

		qsort(d->diff, entries, sizeof(sg_network_io_stats),
				sg_network_io_compare_name);
	}
}

static void
get_network_iface_stats(uint64_t tick)
{
	xsg_debug("sg_get_network_iface_stats");
	if (!(network_iface_stats = sg_get_network_iface_stats(&network_iface_entries))) {
		xsg_warning("%s returned NULL", "sg_get_network_iface_stats");
		libstatgrab_error();
		return;
	}
	qsort(network_iface_stats, network_iface_entries,
			sizeof(sg_network_iface_stats),
			sg_network_iface_compare_name);
}

static void
get_page_stats(uint64_t tick)
{
	xsg_debug("sg_get_page_stats");
	if (!(page_stats = sg_get_page_stats())) {
		xsg_warning("%s returned NULL", "sg_get_page_stats");
		libstatgrab_error();
	}
}

static void
get_page_stats_diff(uint64_t tick)
{
	sg_page_stats *diff;
	xsg_list_t *l;

	xsg_debug("sg_get_page_stats_diff");
	if (!(diff = sg_get_page_stats_diff())) {
		xsg_warning("%s returned NULL", "sg_get_page_stats_diff");
		libstatgrab_error();
	}

	for (l = page_stats_diff_list; l; l = l->next) {
		page_stats_diff_t *d = l->data;

		if (tick % d->update == 0) {
			d->diff->pages_pagein = 0;
			d->diff->pages_pageout = 0;
			d->diff->systime = 0;
		}

		d->diff->pages_pagein += diff->pages_pagein;
		d->diff->pages_pageout += diff->pages_pageout;
		d->diff->systime += diff->systime;
	}
}

static void
get_process_stats(uint64_t tick)
{
	xsg_debug("sg_get_process_stats");
	if (process_stats_by_name) {
		xsg_free(process_stats_by_name);
		process_stats_by_name = NULL;
	}
	if (process_stats_by_pid) {
		xsg_free(process_stats_by_pid);
		process_stats_by_pid = NULL;
	}
	if (process_stats_by_uid) {
		xsg_free(process_stats_by_uid);
		process_stats_by_uid = NULL;
	}
	if (process_stats_by_gid) {
		xsg_free(process_stats_by_gid);
		process_stats_by_gid = NULL;
	}
	if (process_stats_by_size) {
		xsg_free(process_stats_by_size);
		process_stats_by_size = NULL;
	}
	if (process_stats_by_res) {
		xsg_free(process_stats_by_res);
		process_stats_by_res = NULL;
	}
	if (process_stats_by_cpu) {
		xsg_free(process_stats_by_cpu);
		process_stats_by_cpu = NULL;
	}
	if (process_stats_by_time) {
		xsg_free(process_stats_by_time);
		process_stats_by_time = NULL;
	}
	if (!(process_stats = sg_get_process_stats(&process_entries))) {
		xsg_warning("%s returned NULL", "sg_get_process_stats");
		libstatgrab_error();
	}
}

static void
get_process_count(uint64_t tick)
{
	xsg_debug("sg_get_process_count");
	if (!(process_count = sg_get_process_count())) {
		xsg_warning("%s returned NULL", "sg_get_process_count");
		libstatgrab_error();
	}
}

/******************************************************************************/

typedef struct {
	uint64_t update;
	void (*func)(uint64_t);
} stat_t;

xsg_list_t *stat_list = NULL;

static void
add_stat(uint64_t update, void (*func)(uint64_t))
{
	stat_t *stat = NULL;
	xsg_list_t *l;

	/* find matching entry in stat_list */
	for (l = stat_list; l; l = l->next) {
		stat = l->data;
		if (stat->func == func) {
			/* is update a multiple of stat->update? */
			if ((update % stat->update) == 0) {
				return;
			}
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

static void
update_stats(uint64_t tick)
{
	stat_t *stat;
	xsg_list_t *l;

	for (l = stat_list; l; l = l->next) {
		stat = l->data;
		if (tick % stat->update == 0) {
			stat->func(tick);
		}
	}
}

/******************************************************************************
 *
 * host_info
 *
 ******************************************************************************/

static char *
get_host_info_os_name(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_os_name: \"%s\"", host_info->os_name);
		return host_info->os_name;
	} else {
		xsg_debug("get_host_info_os_name: UNKNOWN");
		return NULL;
	}
}

static char *
get_host_info_os_release(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_os_release: \"%s\"",
				host_info->os_release);
		return host_info->os_release;
	} else {
		xsg_debug("get_host_info_os_release: UNKNOWN");
		return NULL;
	}
}

static char *
get_host_info_os_version(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_os_version: \"%s\"",
				host_info->os_version);
		return host_info->os_version;
	} else {
		xsg_debug("get_host_info_os_version: UNKNOWN");
		return NULL;
	}
}

static char *
get_host_info_platform(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_platform: \"%s\"",
				host_info->platform);
		return host_info->platform;
	} else {
		xsg_debug("get_host_info_platform: UNKNOWN");
		return NULL;
	}
}

static char *
get_host_info_hostname(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_hostname: \"%s\"",
				host_info->hostname);
		return host_info->hostname;
	} else {
		xsg_debug("get_host_info_hostname: UNKNOWN");
		return NULL;
	}
}

static double
get_host_info_uptime(void *arg)
{
	if (likely(host_info != NULL)) {
		xsg_debug("get_host_info_uptime: %f",
				(double) host_info->uptime);
		return (double) host_info->uptime;
	} else {
		xsg_debug("get_host_info_uptime: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_host_info(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("os_name")) {
		*str = get_host_info_os_name;
	} else if (xsg_conf_find_command("os_release")) {
		*str = get_host_info_os_release;
	} else if (xsg_conf_find_command("os_version")) {
		*str = get_host_info_os_version;
	} else if (xsg_conf_find_command("platform")) {
		*str = get_host_info_platform;
	} else if (xsg_conf_find_command("hostname")) {
		*str = get_host_info_hostname;
	} else if (xsg_conf_find_command("uptime")) {
		*num = get_host_info_uptime;
	} else {
		xsg_conf_error("os_name, os_release, os_version, platform, "
				"hostname or uptime expected");
	}
}

/******************************************************************************
 *
 * cpu_stats
 *
 ******************************************************************************/

static double
get_cpu_stats_user(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_user: %f", (double) cpu_stats->user);
		return (double) cpu_stats->user;
	} else {
		xsg_debug("get_cpu_stats_user: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_kernel(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_kernel: %f",
				(double) cpu_stats->kernel);
		return (double) cpu_stats->kernel;
	} else {
		xsg_debug("get_cpu_stats_kernel: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_idle(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_idle: %f", (double) cpu_stats->idle);
		return (double) cpu_stats->idle;
	} else {
		xsg_debug("get_cpu_stats_idle: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_iowait(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_iowait: %f",
				(double) cpu_stats->iowait);
		return (double) cpu_stats->iowait;
	} else {
		xsg_debug("get_cpu_stats_iowait: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_swap(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_swap: %f", (double) cpu_stats->swap);
		return (double) cpu_stats->swap;
	} else {
		xsg_debug("get_cpu_stats_swap: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_nice(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_nice: %f", (double) cpu_stats->nice);
		return (double) cpu_stats->nice;
	} else {
		xsg_debug("get_cpu_stats_nice: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_total(void *arg)
{
	if (likely(cpu_stats != NULL)) {
		xsg_debug("get_cpu_stats_total: %f", (double) cpu_stats->total);
		return (double) cpu_stats->total;
	} else {
		xsg_debug("get_cpu_stats_total: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_cpu_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("user")) {
		*num = get_cpu_stats_user;
	} else if (xsg_conf_find_command("kernel")) {
		*num = get_cpu_stats_kernel;
	} else if (xsg_conf_find_command("idle")) {
		*num = get_cpu_stats_idle;
	} else if (xsg_conf_find_command("iowait")) {
		*num = get_cpu_stats_iowait;
	} else if (xsg_conf_find_command("swap")) {
		*num = get_cpu_stats_swap;
	} else if (xsg_conf_find_command("nice")) {
		*num = get_cpu_stats_nice;
	} else if (xsg_conf_find_command("total")) {
		*num = get_cpu_stats_total;
	} else {
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or "
				"total expected");
	}
}

/******************************************************************************
 *
 * cpu_stats_diff
 *
 ******************************************************************************/

static double
get_cpu_stats_diff_user(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_user: %f", (double) diff->user);
		return (double) diff->user;
	} else {
		xsg_debug("get_cpu_stats_diff_user: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_kernel(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_kernel: %f",
				(double) diff->kernel);
		return (double) diff->kernel;
	} else {
		xsg_debug("get_cpu_stats_diff_kernel: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_idle(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_idle: %f", (double) diff->idle);
		return (double) diff->idle;
	} else {
		xsg_debug("get_cpu_stats_diff_idle: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_iowait(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_iowait: %f",
				(double) diff->iowait);
		return (double) diff->iowait;
	} else {
		xsg_debug("get_cpu_stats_diff_iowait: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_swap(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_swap: %f", (double) diff->swap);
		return (double) diff->swap;
	} else {
		xsg_debug("get_cpu_stats_diff_swap: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_nice(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_nice: %f", (double) diff->nice);
		return (double) diff->nice;
	} else {
		xsg_debug("get_cpu_stats_diff_nice: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_stats_diff_total(void *arg)
{
	sg_cpu_stats *diff = (sg_cpu_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_cpu_stats_diff_total: %f", (double) diff->total);
		return (double) diff->total;
	} else {
		xsg_debug("get_cpu_stats_diff_total: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static sg_cpu_stats *
find_cpu_stats_diff(uint64_t update)
{
	cpu_stats_diff_t *d;
	xsg_list_t *l;

	for (l = cpu_stats_diff_list; l; l = l->next) {
		d = l->data;
		if (update == d->update) {
			return d->diff;
		}
	}

	d = xsg_new(cpu_stats_diff_t, 1);
	d->update = update;
	d->diff = xsg_new(sg_cpu_stats, 1);
	d->diff->user = 0;
	d->diff->kernel = 0;
	d->diff->idle = 0;
	d->diff->iowait = 0;
	d->diff->swap = 0;
	d->diff->nice = 0;
	d->diff->total = 0;
	d->diff->systime = 0;

	cpu_stats_diff_list = xsg_list_append(cpu_stats_diff_list, (void *) d);

	return d->diff;
}

static void
parse_cpu_stats_diff(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	*arg = find_cpu_stats_diff(update);

	if (xsg_conf_find_command("user")) {
		*num = get_cpu_stats_diff_user;
	} else if (xsg_conf_find_command("kernel")) {
		*num = get_cpu_stats_diff_kernel;
	} else if (xsg_conf_find_command("idle")) {
		*num = get_cpu_stats_diff_idle;
	} else if (xsg_conf_find_command("iowait")) {
		*num = get_cpu_stats_diff_iowait;
	} else if (xsg_conf_find_command("swap")) {
		*num = get_cpu_stats_diff_swap;
	} else if (xsg_conf_find_command("nice")) {
		*num = get_cpu_stats_diff_nice;
	} else if (xsg_conf_find_command("total")) {
		*num = get_cpu_stats_diff_total;
	} else {
		xsg_conf_error("user, kernel, idle, iowait, swap, nice or "
				"total expected");
	}
}

/******************************************************************************
 *
 * cpu_percents
 *
 ******************************************************************************/

static double
get_cpu_percents_user(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_user: %f", cpu_percents->user);
		return cpu_percents->user;
	} else {
		xsg_debug("get_cpu_percents_user: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_percents_kernel(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_kernel: %f", cpu_percents->kernel);
		return cpu_percents->kernel;
	} else {
		xsg_debug("get_cpu_percents_kernel: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_percents_idle(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_idle: %f", cpu_percents->idle);
		return cpu_percents->idle;
	} else {
		xsg_debug("get_cpu_percents_idle: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_percents_iowait(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_iowait: %f", cpu_percents->iowait);
		return cpu_percents->iowait;
	} else {
		xsg_debug("get_cpu_percents_iowait: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_percents_swap(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_swap: %f", cpu_percents->swap);
		return cpu_percents->swap;
	} else {
		xsg_debug("get_cpu_percents_swap: UNKNOWN");
		return DNAN;
	}
}

static double
get_cpu_percents_nice(void *arg)
{
	if (likely(cpu_percents != NULL)) {
		xsg_debug("get_cpu_percents_nice: %f", cpu_percents->nice);
		return cpu_percents->nice;
	} else {
		xsg_debug("get_cpu_percents_nice: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_cpu_percents(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("user")) {
		*num = get_cpu_percents_user;
	} else if (xsg_conf_find_command("kernel")) {
		*num = get_cpu_percents_kernel;
	} else if (xsg_conf_find_command("idle")) {
		*num = get_cpu_percents_idle;
	} else if (xsg_conf_find_command("iowait")) {
		*num = get_cpu_percents_iowait;
	} else if (xsg_conf_find_command("swap")) {
		*num = get_cpu_percents_swap;
	} else if (xsg_conf_find_command("nice")) {
		*num = get_cpu_percents_nice;
	} else {
		xsg_conf_error("user, kernel, idle, iowait, swap or nice "
				"expected");
	}
}

/******************************************************************************
 *
 * mem_stats
 *
 ******************************************************************************/

static double
get_mem_stats_total(void *arg)
{
	if (likely(mem_stats != NULL)) {
		xsg_debug("get_cpu_percents_nice: %f",
				(double) mem_stats->total);
		return (double) mem_stats->total;
	} else {
		xsg_debug("get_cpu_percents_nice: UNKNOWN");
		return DNAN;
	}
}

static double
get_mem_stats_free(void *arg)
{
	if (likely(mem_stats != NULL)) {
		xsg_debug("get_mem_stats_free: %f", (double) mem_stats->free);
		return (double) mem_stats->free;
	} else {
		xsg_debug("get_mem_stats_free: UNKNOWN");
		return DNAN;
	}
}

static double
get_mem_stats_used(void *arg)
{
	if (likely(mem_stats != NULL)) {
		xsg_debug("get_mem_stats_used: %f", (double) mem_stats->used);
		return (double) mem_stats->used;
	} else {
		xsg_debug("get_mem_stats_used: UNKNOWN");
		return DNAN;
	}
}

static double
get_mem_stats_cache(void *arg)
{
	if (likely(mem_stats != NULL)) {
		xsg_debug("get_mem_stats_cache: %f", (double) mem_stats->cache);
		return (double) mem_stats->cache;
	} else {
		xsg_debug("get_mem_stats_cache: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_mem_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("total")) {
		*num = get_mem_stats_total;
	} else if (xsg_conf_find_command("free")) {
		*num = get_mem_stats_free;
	} else if (xsg_conf_find_command("used")) {
		*num = get_mem_stats_used;
	} else if (xsg_conf_find_command("cache")) {
		*num = get_mem_stats_cache;
	} else {
		xsg_conf_error("total, free, used or cache expected");
	}
}

/******************************************************************************
 *
 * load_stats
 *
 ******************************************************************************/

static double
get_load_stats_min1(void *arg)
{
	if (likely(load_stats != NULL)) {
		xsg_debug("get_load_stats_min1: %f", load_stats->min1);
		return load_stats->min1;
	} else {
		xsg_debug("get_load_stats_min1: UNKNOWN");
		return DNAN;
	}
}

static double
get_load_stats_min5(void *arg)
{
	if (likely(load_stats != NULL)) {
		xsg_debug("get_load_stats_min5: %f", load_stats->min5);
		return load_stats->min5;
	} else {
		xsg_debug("get_load_stats_min5: UNKNOWN");
		return DNAN;
	}
}

static double
get_load_stats_min15(void *arg)
{
	if (likely(load_stats != NULL)) {
		xsg_debug("get_load_stats_min15: %f", load_stats->min15);
		return load_stats->min15;
	} else {
		xsg_debug("get_load_stats_min15: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_load_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("min15")) {
		*num = get_load_stats_min15;
	} else if (xsg_conf_find_command("min5")) {
		*num = get_load_stats_min5;
	} else if (xsg_conf_find_command("min1")) {
		*num = get_load_stats_min1;
	} else {
		xsg_conf_error("min1, min5 or min15 expected");
	}
}

/******************************************************************************
 *
 * user_stats
 *
 ******************************************************************************/

static char *
get_user_stats_name_list(void *arg)
{
	if (likely(user_stats != NULL)) {
		xsg_debug("get_user_stats_name_list: \"%s\"",
				user_stats->name_list);
		return user_stats->name_list;
	} else {
		xsg_debug("get_user_stats_name_list: UNKNOWN");
		return NULL;
	}
}

static double
get_user_stats_num_entries(void *arg)
{
	if (likely(user_stats != NULL)) {
		xsg_debug("get_user_stats_num_entries: %f",
				(double) user_stats->num_entries);
		return (double) user_stats->num_entries;
	} else {
		xsg_debug("get_user_stats_num_entries: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_user_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("num_entries")) {
		*num = get_user_stats_num_entries;
	} else if (xsg_conf_find_command("name_list")) {
		*str = get_user_stats_name_list;
	} else {
		xsg_conf_error("num_entries or name_list expected");
	}
}

/******************************************************************************
 *
 * swap_stats
 *
 ******************************************************************************/

static double
get_swap_stats_total(void *arg)
{
	if (likely(swap_stats != NULL)) {
		xsg_debug("get_swap_stats_total: %f",
				(double) swap_stats->total);
		return (double) swap_stats->total;
	} else {
		xsg_debug("get_swap_stats_total: UNKNOWN");
		return DNAN;
	}
}

static double
get_swap_stats_used(void *arg)
{
	if (likely(swap_stats != NULL)) {
		xsg_debug("get_swap_stats_used: %f", (double) swap_stats->used);
		return (double) swap_stats->used;
	} else {
		xsg_debug("get_swap_stats_used: UNKNOWN");
		return DNAN;
	}
}

static double
get_swap_stats_free(void *arg)
{
	if (likely(swap_stats != NULL)) {
		xsg_debug("get_swap_stats_free: %f", (double) swap_stats->free);
		return (double) swap_stats->free;
	} else {
		xsg_debug("get_swap_stats_free: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_swap_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("total")) {
		*num = get_swap_stats_total;
	} else if (xsg_conf_find_command("used")) {
		*num = get_swap_stats_used;
	} else if (xsg_conf_find_command("free")) {
		*num = get_swap_stats_free;
	} else {
		xsg_conf_error("total, used or free expected");
	}
}

/******************************************************************************
 *
 * fs_stats
 *
 ******************************************************************************/

static sg_fs_stats *
get_fs_stats_for_device_name(char *device_name)
{
	sg_fs_stats key;

	key.device_name = device_name;
	return bsearch(&key, fs_stats_by_device_name, fs_entries,
			sizeof(sg_fs_stats), sg_fs_compare_device_name);
}

static sg_fs_stats *
get_fs_stats_for_mnt_point(char *mnt_point)
{
	sg_fs_stats key;

	key.mnt_point = mnt_point;
	return bsearch(&key, fs_stats_by_mnt_point, fs_entries,
			sizeof(sg_fs_stats), sg_fs_compare_mnt_point);
}

/******************************************************************************/

static char *
get_fs_stats_device_name(void *arg)
{
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
		xsg_debug("get_fs_stats_device_name: fs_stats for \"%s\" not "
				"found", data);
		return NULL;
	}
}

static char *
get_fs_stats_fs_type(void *arg)
{
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
		xsg_debug("get_fs_stats_fs_type: fs_stats for \"%s\" not found",
				data);
		return NULL;
	}
}

static char *
get_fs_stats_mnt_point(void *arg)
{
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
		xsg_debug("get_fs_stats_mnt_point: fs_stats for \"%s\" not "
				"found", data);
		return NULL;
	}
}

static double
get_fs_stats_size(void *arg)
{
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
		xsg_debug("get_fs_stats_size: fs_stats for \"%s\" not found",
				data);
		return DNAN;
	}
}

static double
get_fs_stats_used(void *arg)
{
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
		xsg_debug("get_fs_stats_used: fs_stats for \"%s\" not found",
				data);
		return DNAN;
	}
}

static double
get_fs_stats_avail(void *arg)
{
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
		xsg_debug("get_fs_stats_avail: fs_stats for \"%s\" not found",
				data);
		return DNAN;
	}
}

static double
get_fs_stats_total_inodes(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_total_inodes: %f",
				(double) ret->total_inodes);
		return (double) ret->total_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_total_inodes: %f",
				(double) ret->total_inodes);
		return (double) ret->total_inodes;
	} else {
		xsg_debug("get_fs_stats_total_inodes: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_used_inodes(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_used_inodes: %f",
				(double) ret->used_inodes);
		return (double) ret->used_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_used_inodes: %f",
				(double) ret->used_inodes);
		return (double) ret->used_inodes;
	} else {
		xsg_debug("get_fs_stats_used_inodes: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_free_inodes(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_free_inodes: %f",
				(double) ret->free_inodes);
		return (double) ret->free_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_free_inodes: %f",
				(double) ret->free_inodes);
		return (double) ret->free_inodes;
	} else {
		xsg_debug("get_fs_stats_free_inodes: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_avail_inodes(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_avail_inodes: %f",
				(double) ret->avail_inodes);
		return (double) ret->avail_inodes;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_avail_inodes: %f",
				(double) ret->avail_inodes);
		return (double) ret->avail_inodes;
	} else {
		xsg_debug("get_fs_stats_avail_inodes: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_io_size(void *arg)
{
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
		xsg_debug("get_fs_stats_io_size: fs_stats for \"%s\" not found",
				data);
		return DNAN;
	}
}

static double
get_fs_stats_block_size(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_block_size: %f",
				(double) ret->block_size);
		return (double) ret->block_size;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_block_size: %f",
				(double) ret->block_size);
		return (double) ret->block_size;
	} else {
		xsg_debug("get_fs_stats_block_size: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_total_blocks(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_total_blocks: %f",
				(double) ret->total_blocks);
		return (double) ret->total_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_total_blocks: %f",
				(double) ret->total_blocks);
		return (double) ret->total_blocks;
	} else {
		xsg_debug("get_fs_stats_total_blocks: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_free_blocks(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_free_blocks: %f",
				(double) ret->free_blocks);
		return (double) ret->free_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_free_blocks: %f",
				(double) ret->free_blocks);
		return (double) ret->free_blocks;
	} else {
		xsg_debug("get_fs_stats_free_blocks: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_used_blocks(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_used_blocks: %f",
				(double) ret->used_blocks);
		return (double) ret->used_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_used_blocks: %f",
				(double) ret->used_blocks);
		return (double) ret->used_blocks;
	} else {
		xsg_debug("get_fs_stats_used_blocks: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

static double
get_fs_stats_avail_blocks(void *arg)
{
	sg_fs_stats *ret;
	char *data;

	data = (char *) arg;

	if ((ret = get_fs_stats_for_device_name(data))) {
		xsg_debug("get_fs_stats_avail_blocks: %f",
				(double) ret->avail_blocks);
		return (double) ret->avail_blocks;
	} else if ((ret = get_fs_stats_for_mnt_point(data))) {
		xsg_debug("get_fs_stats_avail_blocks: %f",
				(double) ret->avail_blocks);
		return (double) ret->avail_blocks;
	} else {
		xsg_debug("get_fs_stats_avail_blocks: fs_stats for \"%s\" not "
				"found", data);
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_fs_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	*arg = (void *) xsg_conf_read_string();

	if (!strcmp(*arg, "*")) {
		get_fs_stats(0);

		if (fs_entries > 0) {
			xsg_free(*arg);
			*arg = xsg_strdup(fs_stats_by_device_name[0].device_name);
		}
	}

	if (xsg_conf_find_command("device_name")) {
		*str = get_fs_stats_device_name;
	} else if (xsg_conf_find_command("fs_type")) {
		*str = get_fs_stats_fs_type;
	} else if (xsg_conf_find_command("mnt_point")) {
		*str = get_fs_stats_mnt_point;
	} else if (xsg_conf_find_command("size")) {
		*num = get_fs_stats_size;
	} else if (xsg_conf_find_command("used")) {
		*num = get_fs_stats_used;
	} else if (xsg_conf_find_command("avail")) {
		*num = get_fs_stats_avail;
	} else if (xsg_conf_find_command("total_inodes")) {
		*num = get_fs_stats_total_inodes;
	} else if (xsg_conf_find_command("used_inodes")) {
		*num = get_fs_stats_used_inodes;
	} else if (xsg_conf_find_command("free_inodes")) {
		*num = get_fs_stats_free_inodes;
	} else if (xsg_conf_find_command("avail_inodes")) {
		*num = get_fs_stats_avail_inodes;
	} else if (xsg_conf_find_command("io_size")) {
		*num = get_fs_stats_io_size;
	} else if (xsg_conf_find_command("block_size")) {
		*num = get_fs_stats_block_size;
	} else if (xsg_conf_find_command("total_blocks")) {
		*num = get_fs_stats_total_blocks;
	} else if (xsg_conf_find_command("free_blocks")) {
		*num = get_fs_stats_free_blocks;
	} else if (xsg_conf_find_command("used_blocks")) {
		*num = get_fs_stats_used_blocks;
	} else if (xsg_conf_find_command("avail_blocks")) {
		*num = get_fs_stats_avail_blocks;
	} else {
		xsg_conf_error("device_name, fs_type, mnt_point, size, used, "
				"avail, total_inodes, used_inodes, "
				"free_inodes, avail_inodes, io_size, "
				"block_size, total_blocks, free_blocks, "
				"used_blocks or avail_blocks expected");
	}
}

/******************************************************************************
 *
 * disk_io_stats
 *
 ******************************************************************************/

static sg_disk_io_stats *
get_disk_io_stats_for_disk_name(char *disk_name)
{
	sg_disk_io_stats key;

	key.disk_name = disk_name;
	return bsearch(&key, disk_io_stats, disk_io_entries,
			sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

/******************************************************************************/

static double
get_disk_io_stats_read_bytes(void *arg)
{
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_read_bytes: %f",
				(double) ret->read_bytes);
		return (double) ret->read_bytes;
	} else {
		xsg_debug("get_disk_io_stats_read_bytes: disk_io_stats "
				"for \"%s\" not found", disk_name);
		return DNAN;
	}
}

static double
get_disk_io_stats_write_bytes(void *arg)
{
	sg_disk_io_stats *ret;
	char *disk_name;

	disk_name = (char *) arg;

	if ((ret = get_disk_io_stats_for_disk_name(disk_name))) {
		xsg_debug("get_disk_io_stats_write_bytes: %f",
				(double) ret->write_bytes);
		return (double) ret->write_bytes;
	} else {
		xsg_debug("get_disk_io_stats_write_bytes: disk_io_stats "
				"for \"%s\" not found", disk_name);
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_disk_io_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	*arg = (void *) xsg_conf_read_string();

	if (!strcmp(*arg, "*")) {
		get_disk_io_stats(0);

		if (disk_io_entries > 0) {
			xsg_free(*arg);
			*arg = xsg_strdup(disk_io_stats[0].disk_name);
		}
	}

	if (xsg_conf_find_command("read_bytes")) {
		*num = get_disk_io_stats_read_bytes;
	} else if (xsg_conf_find_command("write_bytes")) {
		*num = get_disk_io_stats_write_bytes;
	} else {
		xsg_conf_error("read_bytes or write_bytes expected");
	}
}

/******************************************************************************
 *
 * disk_io_stats_diff
 *
 ******************************************************************************/

typedef struct _disk_io_stats_diff_arg_t {
	char *disk_name;
	disk_io_stats_diff_t *d;
} disk_io_stats_diff_arg_t;

/******************************************************************************/

static sg_disk_io_stats *
get_disk_io_stats_diff_for_disk_name(disk_io_stats_diff_arg_t *a)
{
	sg_disk_io_stats key;

	key.disk_name = a->disk_name;
	return bsearch(&key, a->d->diff, a->d->entries,
			sizeof(sg_disk_io_stats), sg_disk_io_compare_name);
}

/******************************************************************************/

static double
get_disk_io_stats_diff_read_bytes(void *arg)
{
	disk_io_stats_diff_arg_t *a;
	sg_disk_io_stats *ret;

	a = (disk_io_stats_diff_arg_t *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(a))) {
		xsg_debug("get_disk_io_stats_diff_read_bytes: %f",
				(double) ret->read_bytes);
		return (double) ret->read_bytes;
	} else {
		xsg_debug("get_disk_io_stats_diff_read_bytes: "
				"disk_io_stats_diff for \"%s\" not found",
				a->disk_name);
		return DNAN;
	}
}

static double
get_disk_io_stats_diff_write_bytes(void *arg)
{
	disk_io_stats_diff_arg_t *a;
	sg_disk_io_stats *ret;

	a = (disk_io_stats_diff_arg_t *) arg;

	if ((ret = get_disk_io_stats_diff_for_disk_name(a))) {
		xsg_debug("get_disk_io_stats_diff_write_bytes: %f",
				(double) ret->write_bytes);
		return (double) ret->write_bytes;
	} else {
		xsg_debug("get_disk_io_stats_diff_write_bytes: "
				"disk_io_stats_diff for \"%s\" not found",
				a->disk_name);
		return DNAN;
	}
}

/******************************************************************************/

static disk_io_stats_diff_t *
find_disk_io_stats_diff(uint64_t update)
{
	disk_io_stats_diff_t *d;
	xsg_list_t *l;

	for (l = disk_io_stats_diff_list; l; l = l->next) {
		d = l->data;
		if (update == d->update) {
			return d;
		}
	}

	d = xsg_new(disk_io_stats_diff_t, 1);
	d->update = update;
	d->entries = 0;
	d->diff = NULL;

	disk_io_stats_diff_list = xsg_list_append(disk_io_stats_diff_list,
			(void *) d);

	return d;
}

static void
parse_disk_io_stats_diff(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	disk_io_stats_diff_arg_t *a;
	char *disk_name;

	disk_name = xsg_conf_read_string();

	if (!strcmp(disk_name, "*")) {
		get_disk_io_stats(0);

		if (disk_io_entries > 0) {
			xsg_free(disk_name);
			disk_name = xsg_strdup(disk_io_stats[0].disk_name);
		}
	}

	a = xsg_new(disk_io_stats_diff_arg_t, 1);
	a->disk_name = disk_name;
	a->d = find_disk_io_stats_diff(update);

	*arg = (void *) a;

	if (xsg_conf_find_command("read_bytes")) {
		*num = get_disk_io_stats_diff_read_bytes;
	} else if (xsg_conf_find_command("write_bytes")) {
		*num = get_disk_io_stats_diff_write_bytes;
	} else {
		xsg_conf_error("read_bytes or write_bytes expected");
	}
}

/******************************************************************************
 *
 * network_io_stats
 *
 ******************************************************************************/

static sg_network_io_stats *
get_network_io_stats_for_interface_name(char *interface_name)
{
	sg_network_io_stats key;

	key.interface_name = interface_name;
	return bsearch(&key, network_io_stats, network_io_entries,
			sizeof(sg_network_io_stats),
			sg_network_io_compare_name);
}

/******************************************************************************/

static double
get_network_io_stats_tx(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_tx: %f", (double) ret->tx);
		return (double) ret->tx;
	} else {
		xsg_debug("get_network_io_stats_tx: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_rx(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_rx: %f", (double) ret->rx);
		return (double) ret->rx;
	} else {
		xsg_debug("get_network_io_stats_rx: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_ipackets(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_ipackets: %f",
				(double) ret->ipackets);
		return (double) ret->ipackets;
	} else {
		xsg_debug("get_network_io_stats_ipackets: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_opackets(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_opackets: %f",
				(double) ret->opackets);
		return (double) ret->opackets;
	} else {
		xsg_debug("get_network_io_stats_opackets: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_ierrors(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_ierrors: %f",
				(double) ret->ierrors);
		return (double) ret->ierrors;
	} else {
		xsg_debug("get_network_io_stats_ierrors: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_oerrors(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_oerrors: %f",
				(double) ret->oerrors);
		return (double) ret->oerrors;
	} else {
		xsg_debug("get_network_io_stats_oerrors: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_collisions(void *arg)
{
	sg_network_io_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_io_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_io_stats_collisions: %f",
				(double) ret->collisions);
		return (double) ret->collisions;
	} else {
		xsg_debug("get_network_io_stats_collisions: network_io_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_network_io_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	*arg = (void *) xsg_conf_read_string();

	if (!strcmp(*arg, "*")) {
		get_network_io_stats(0);

		if (network_io_entries > 0) {
			xsg_free(*arg);
			*arg = xsg_strdup(network_io_stats[0].interface_name);
		}
	}

	if (xsg_conf_find_command("tx")) {
		*num = get_network_io_stats_tx;
	} else if (xsg_conf_find_command("rx")) {
		*num = get_network_io_stats_rx;
	} else if (xsg_conf_find_command("ipackets")) {
		*num = get_network_io_stats_ipackets;
	} else if (xsg_conf_find_command("opackets")) {
		*num = get_network_io_stats_opackets;
	} else if (xsg_conf_find_command("ierrors")) {
		*num = get_network_io_stats_ierrors;
	} else if (xsg_conf_find_command("oerrors")) {
		*num = get_network_io_stats_oerrors;
	} else if (xsg_conf_find_command("collisions")) {
		*num = get_network_io_stats_collisions;
	} else {
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors "
				"or collisions expected");
	}
}

/******************************************************************************
 *
 * network_io_stats_diff
 *
 ******************************************************************************/

typedef struct _network_io_stats_diff_arg_t {
	char *interface_name;
	network_io_stats_diff_t *d;
} network_io_stats_diff_arg_t;

/******************************************************************************/

static sg_network_io_stats *
get_network_io_stats_diff_for_interface_name(network_io_stats_diff_arg_t *a)
{
	sg_network_io_stats key;

	key.interface_name = a->interface_name;
	return bsearch(&key, a->d->diff, a->d->entries,
			sizeof(sg_network_io_stats),
			sg_network_io_compare_name);
}

/******************************************************************************/

static double
get_network_io_stats_diff_tx(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_tx: %f", (double) ret->tx);
		return (double) ret->tx;
	} else {
		xsg_debug("get_network_io_stats_diff_tx: network_io_stats_diff "
				"for \"%s\" not found", a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_rx(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_rx: %f", (double) ret->rx);
		return (double) ret->rx;
	} else {
		xsg_debug("get_network_io_stats_diff_rx: network_io_stats_diff "
				"for \"%s\" not found", a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_ipackets(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_ipackets: %f",
				(double) ret->ipackets);
		return (double) ret->ipackets;
	} else {
		xsg_debug("get_network_io_stats_diff_ipackets: "
				"network_io_stats_diff for \"%s\" not found",
				a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_opackets(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_opackets: %f",
				(double) ret->opackets);
		return (double) ret->opackets;
	} else {
		xsg_debug("get_network_io_stats_diff_opackets: "
				"network_io_stats_diff for \"%s\" not found",
				a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_ierrors(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_ierrors: %f",
				(double) ret->ierrors);
		return (double) ret->ierrors;
	} else {
		xsg_debug("get_network_io_stats_diff_ierrors: "
				"network_io_stats_diff for \"%s\" not found",
				a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_oerrors(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_oerrors: %f",
				(double) ret->oerrors);
		return (double) ret->oerrors;
	} else {
		xsg_debug("get_network_io_stats_diff_oerrors: "
				"network_io_stats_diff for \"%s\" not found",
				a->interface_name);
		return DNAN;
	}
}

static double
get_network_io_stats_diff_collisions(void *arg)
{
	network_io_stats_diff_arg_t *a;
	sg_network_io_stats *ret;

	a = (network_io_stats_diff_arg_t *) arg;

	if ((ret = get_network_io_stats_diff_for_interface_name(a))) {
		xsg_debug("get_network_io_stats_diff_collisions: %f",
				(double) ret->collisions);
		return (double) ret->collisions;
	} else {
		xsg_debug("get_network_io_stats_diff_collisions: "
				"network_io_stats_diff for \"%s\" not found",
				a->interface_name);
		return DNAN;
	}
}

/******************************************************************************/

static network_io_stats_diff_t *
find_network_io_stats_diff(uint64_t update)
{
	network_io_stats_diff_t *d;
	xsg_list_t *l;

	for (l = network_io_stats_diff_list; l; l = l->next) {
		d = l->data;
		if (update == d->update) {
			return d;
		}
	}

	d = xsg_new(network_io_stats_diff_t, 1);
	d->update = update;
	d->entries = 0;
	d->diff = NULL;

	network_io_stats_diff_list = xsg_list_append(network_io_stats_diff_list,
			(void *) d);

	return d;
}

static void
parse_network_io_stats_diff(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	network_io_stats_diff_arg_t *a;
	char *interface_name;

	interface_name = xsg_conf_read_string();

	if (!strcmp(interface_name, "*")) {
		get_network_io_stats(0);

		if (network_io_entries > 0) {
			xsg_free(interface_name);
			interface_name = xsg_strdup(network_io_stats[0].interface_name);
		}
	}

	a = xsg_new(network_io_stats_diff_arg_t, 1);
	a->interface_name = interface_name;
	a->d = find_network_io_stats_diff(update);

	*arg = (void *) a;

	if (xsg_conf_find_command("tx")) {
		*num = get_network_io_stats_diff_tx;
	} else if (xsg_conf_find_command("rx")) {
		*num = get_network_io_stats_diff_rx;
	} else if (xsg_conf_find_command("ipackets")) {
		*num = get_network_io_stats_diff_ipackets;
	} else if (xsg_conf_find_command("opackets")) {
		*num = get_network_io_stats_diff_opackets;
	} else if (xsg_conf_find_command("ierrors")) {
		*num = get_network_io_stats_diff_ierrors;
	} else if (xsg_conf_find_command("oerrors")) {
		*num = get_network_io_stats_diff_oerrors;
	} else if (xsg_conf_find_command("collisions")) {
		*num = get_network_io_stats_diff_collisions;
	} else {
		xsg_conf_error("tx, rx, ipackets, opackets, ierrors, oerrors "
				"or collisions expected");
	}
}

/******************************************************************************
 *
 * network_iface_stats
 *
 ******************************************************************************/

static sg_network_iface_stats *
get_network_iface_stats_for_interface_name(char *interface_name)
{
	sg_network_iface_stats key;

	key.interface_name = interface_name;
	return bsearch(&key, network_iface_stats, network_iface_entries,
			sizeof(sg_network_iface_stats),
			sg_network_iface_compare_name);
}

/******************************************************************************/

static double
get_network_iface_stats_speed(void *arg)
{
	sg_network_iface_stats *ret;
	char *interface_name;

	interface_name = (char *) arg;

	if ((ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_speed: %f",
				(double) ret->speed);
		return (double) ret->speed;
	} else {
		xsg_debug("get_network_iface_stats_speed: network_iface_stats "
				"for \"%s\" not found", interface_name);
		return DNAN;
	}
}

typedef struct {
	char *interface_name;
	char *full;
	char *half;
	char *unknown;
} network_iface_stats_duplex_data_t;

static double
get_network_iface_stats_duplex_number(void *arg)
{
	sg_network_iface_stats *ret;
	network_iface_stats_duplex_data_t *data;
	char *interface_name;

	data = (network_iface_stats_duplex_data_t *) arg;
	interface_name = data->interface_name;

	if ((ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_duplex_number: %f",
				(double) ret->duplex);
		return (double) ret->duplex;
	} else {
		xsg_debug("get_network_iface_stats_duplex_number: "
				"network_iface_stats for \"%s\" not found",
				interface_name);
		return DNAN;
	}
}

static char *
get_network_iface_stats_duplex_string(void *arg)
{
	sg_network_iface_stats *ret;
	network_iface_stats_duplex_data_t *data;
	char *interface_name;

	data = (network_iface_stats_duplex_data_t *) arg;
	interface_name = data->interface_name;

	if (!(ret = get_network_iface_stats_for_interface_name(interface_name))) {
		xsg_debug("get_network_iface_stats_duplex_string: "
				"network_iface_stats for \"%s\" not found",
				interface_name);
		return NULL;
	}
	if (ret->duplex == SG_IFACE_DUPLEX_FULL) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"",
				data->full);
		return data->full;
	} else if (ret->duplex == SG_IFACE_DUPLEX_HALF) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"",
				data->half);
		return data->half;
	} else if (ret->duplex == SG_IFACE_DUPLEX_UNKNOWN) {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"",
				data->unknown);
		return data->unknown;
	} else {
		xsg_debug("get_network_iface_stats_duplex_string: \"%s\"",
				data->unknown);
		return data->unknown;
	}
}

/******************************************************************************/

static void
parse_network_iface_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	char *interface_name;

	interface_name = xsg_conf_read_string();

	if (!strcmp(interface_name, "*")) {
		get_network_iface_stats(0);

		if (network_iface_entries > 0) {
			xsg_free(interface_name);
			interface_name = xsg_strdup(network_iface_stats[0].interface_name);
		}
	}

	if (xsg_conf_find_command("speed")) {
		*num = get_network_iface_stats_speed;
		*arg = (void *) interface_name;
	} else if (xsg_conf_find_command("duplex")) {
		network_iface_stats_duplex_data_t *data;

		data = xsg_new(network_iface_stats_duplex_data_t, 1);
		data->interface_name = interface_name;
		data->full = xsg_conf_read_string();
		data->half = xsg_conf_read_string();
		data->unknown = xsg_conf_read_string();

		*num = get_network_iface_stats_duplex_number;
		*str = get_network_iface_stats_duplex_string;
		*arg = (void *) data;
	} else {
		xsg_conf_error("speed or duplex expected");
	}
}

/******************************************************************************
 *
 * page_stats
 *
 ******************************************************************************/

static double
get_page_stats_pages_pagein(void *arg)
{
	if (likely(page_stats != NULL)) {
		xsg_debug("get_page_stats_pages_pagein: %f",
				(double) page_stats->pages_pagein);
		return (double) page_stats->pages_pagein;
	} else {
		xsg_debug("get_page_stats_pages_pagein: UNKNOWN");
		return DNAN;
	}
}

static double
get_page_stats_pages_pageout(void *arg)
{
	if (likely(page_stats != NULL)) {
		xsg_debug("get_page_stats_pages_pageout: %f",
				(double) page_stats->pages_pageout);
		return (double) page_stats->pages_pageout;
	} else {
		xsg_debug("get_page_stats_pages_pageout: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_page_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("pages_pagein")) {
		*num = get_page_stats_pages_pagein;
	} else if (xsg_conf_find_command("pages_pageout")) {
		*num = get_page_stats_pages_pageout;
	} else {
		xsg_conf_error("pages_pagein or pages_pageout expected");
	}
}

/******************************************************************************
 *
 * page_stats_diff
 *
 ******************************************************************************/

static double
get_page_stats_diff_pages_pagein(void *arg)
{
	sg_page_stats *diff = (sg_page_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_page_stats_diff_pages_pagein: %f",
				(double) diff->pages_pagein);
		return (double) diff->pages_pagein;
	} else {
		xsg_debug("get_page_stats_diff_pages_pagein: UNKNOWN");
		return DNAN;
	}
}

static double
get_page_stats_diff_pages_pageout(void *arg)
{
	sg_page_stats *diff = (sg_page_stats *) arg;

	if (likely(diff != NULL)) {
		xsg_debug("get_page_stats_diff_pages_pageout: %f",
				(double) diff->pages_pageout);
		return (double) diff->pages_pageout;
	} else {
		xsg_debug("get_page_stats_diff_pages_pageout: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static sg_page_stats *
find_page_stats_diff(uint64_t update)
{
	page_stats_diff_t *d;
	xsg_list_t *l;

	for (l = page_stats_diff_list; l; l = l->next) {
		d = l->data;
		if (update == d->update) {
			return d->diff;
		}
	}

	d = xsg_new(page_stats_diff_t, 1);
	d->update = update;
	d->diff = xsg_new(sg_page_stats, 1);
	d->diff->pages_pagein = 0;
	d->diff->pages_pageout = 0;
	d->diff->systime = 0;

	page_stats_diff_list = xsg_list_append(page_stats_diff_list,
			(void *) d);

	return d->diff;
}

static void
parse_page_stats_diff(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	*arg = find_page_stats_diff(update);

	if (xsg_conf_find_command("pages_pagein")) {
		*num = get_page_stats_diff_pages_pagein;
	} else if (xsg_conf_find_command("pages_pageout")) {
		*num = get_page_stats_diff_pages_pageout;
	} else {
		xsg_conf_error("pages_pagein or pages_pageout expected");
	}
}

/******************************************************************************
 *
 * process_stats
 *
 ******************************************************************************/

static sg_process_stats *
get_process_stats_orderedby_name(void)
{
	size_t size;

	if (process_stats_by_name) {
		return process_stats_by_name;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_name = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_name, process_stats, size);
	qsort(process_stats_by_name, process_entries, sizeof(sg_process_stats),
			sg_process_compare_name);
	return process_stats_by_name;
}

static sg_process_stats *
get_process_stats_orderedby_pid(void)
{
	size_t size;

	if (process_stats_by_pid) {
		return process_stats_by_pid;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_pid = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_pid, process_stats, size);
	qsort(process_stats_by_pid, process_entries, sizeof(sg_process_stats),
			sg_process_compare_pid);
	return process_stats_by_pid;
}

static sg_process_stats *
get_process_stats_orderedby_uid(void)
{
	size_t size;

	if (process_stats_by_uid) {
		return process_stats_by_uid;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_uid = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_uid, process_stats, size);
	qsort(process_stats_by_uid, process_entries, sizeof(sg_process_stats),
			sg_process_compare_uid);
	return process_stats_by_uid;
}

static sg_process_stats *
get_process_stats_orderedby_gid(void)
{
	size_t size;

	if (process_stats_by_gid) {
		return process_stats_by_gid;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_gid = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_gid, process_stats, size);
	qsort(process_stats_by_gid, process_entries, sizeof(sg_process_stats),
			sg_process_compare_gid);
	return process_stats_by_gid;
}

static sg_process_stats *
get_process_stats_orderedby_size(void)
{
	size_t size;

	if (process_stats_by_size) {
		return process_stats_by_size;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_size = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_size, process_stats, size);
	qsort(process_stats_by_size, process_entries, sizeof(sg_process_stats),
			sg_process_compare_size);
	return process_stats_by_size;
}

static sg_process_stats *
get_process_stats_orderedby_res(void)
{
	size_t size;

	if (process_stats_by_res) {
		return process_stats_by_res;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_res = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_res, process_stats, size);
	qsort(process_stats_by_res, process_entries, sizeof(sg_process_stats),
			sg_process_compare_res);
	return process_stats_by_res;
}

static sg_process_stats *
get_process_stats_orderedby_cpu(void)
{
	size_t size;

	if (process_stats_by_cpu) {
		return process_stats_by_cpu;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_cpu = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_cpu, process_stats, size);
	qsort(process_stats_by_cpu, process_entries, sizeof(sg_process_stats),
			sg_process_compare_cpu);
	return process_stats_by_cpu;
}

static sg_process_stats *
get_process_stats_orderedby_time(void)
{
	size_t size;

	if (process_stats_by_time) {
		return process_stats_by_time;
	}

	size = sizeof(sg_process_stats) * process_entries;
	process_stats_by_time = (sg_process_stats *) xsg_malloc(size);
	memcpy(process_stats_by_time, process_stats, size);
	qsort(process_stats_by_time, process_entries, sizeof(sg_process_stats),
			sg_process_compare_time);
	return process_stats_by_time;
}

/******************************************************************************/

typedef struct {
	sg_process_stats *(*list_func)(void);
	bool ascending;
	uint32_t number;
} process_stats_data_t;

typedef struct {
	sg_process_stats *(*list_func)(void);
	bool ascending;
	uint32_t number;
	char *running;
	char *sleeping;
	char *stopped;
	char *zombie;
	char *unknown;
} process_stats_state_data_t;

static char *
get_process_stats_process_name(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_process_name: process_stats for "
				"number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_process_name: \"%s\"", sg->process_name);

	return sg->process_name;
}

static char *
get_process_stats_proctitle(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_proctitle: process_stats for "
				"number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proctitle: \"%s\"", sg->proctitle);

	return sg->proctitle;
}

static double
get_process_stats_pid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_pid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_pid: %f", (double) sg->pid);

	return (double) sg->pid;
}

static double
get_process_stats_parent(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_parent: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_parent: %f", (double) sg->parent);

	return (double) sg->parent;
}

static double
get_process_stats_pgid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_pgid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_pgid: %f", (double) sg->pgid);

	return (double) sg->pgid;
}

static double
get_process_stats_uid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_uid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_uid: %f", (double) sg->uid);

	return (double) sg->uid;
}

static double
get_process_stats_euid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_euid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_euid: %f", (double) sg->euid);

	return (double) sg->euid;
}

static double
get_process_stats_gid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_gid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_gid: %f", (double) sg->gid);

	return (double) sg->gid;
}

static double
get_process_stats_egid(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_egid: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_egid: %f", (double) sg->egid);

	return (double) sg->egid;
}

static double
get_process_stats_proc_size(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_proc_size: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proc_size: %f", (double) sg->proc_size);

	return (double) sg->proc_size;
}

static double
get_process_stats_proc_resident(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_proc_resident: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_proc_resident: %f",
			(double) sg->proc_resident);

	return (double) sg->proc_resident;
}

static double
get_process_stats_time_spent(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_time_spent: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_time_spent: %f", (double) sg->time_spent);

	return (double) sg->time_spent;
}

static double
get_process_stats_cpu_percent(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_cpu_percent: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_cpu_percent: %f", sg->cpu_percent);

	return sg->cpu_percent;
}

static double
get_process_stats_nice(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_data_t *data;
	uint32_t index;

	data = (process_stats_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_nice: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_nice: %f", (double) sg->nice);

	return (double) sg->nice;
}

static double
get_process_stats_state_number(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_state_data_t *data;
	uint32_t index;

	data = (process_stats_state_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_state_number: process_stats for "
				"number %"PRIu32" not found", data->number);
		return DNAN;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	xsg_debug("get_process_stats_state_number: %f", (double) sg->state);

	return (double) sg->state;
}

static char *
get_process_stats_state_string(void *arg)
{
	sg_process_stats *sg_list, *sg;
	process_stats_state_data_t *data;
	uint32_t index;

	data = (process_stats_state_data_t *) arg;

	if (data->number >= process_entries) {
		xsg_debug("get_process_stats_state_string: process_stats for "
				"number %"PRIu32" not found", data->number);
		return NULL;
	}

	if (data->ascending) {
		index = data->number;
	} else {
		index = process_entries - data->number;
	}

	sg_list = (data->list_func)();
	sg = sg_list + index;

	switch (sg->state) {
	case SG_PROCESS_STATE_RUNNING:
		xsg_debug("get_process_stats_state_string: \"%s\"",
				data->running);
		return data->running;
	case SG_PROCESS_STATE_SLEEPING:
		xsg_debug("get_process_stats_state_string: \"%s\"",
				data->sleeping);
		return data->sleeping;
	case SG_PROCESS_STATE_STOPPED:
		xsg_debug("get_process_stats_state_string: \"%s\"",
				data->stopped);
		return data->stopped;
	case SG_PROCESS_STATE_ZOMBIE:
		xsg_debug("get_process_stats_state_string: \"%s\"",
				data->zombie);
		return data->zombie;
	case SG_PROCESS_STATE_UNKNOWN:
	default:
		xsg_debug("get_process_stats_state_string: \"%s\"",
				data->unknown);
		return data->unknown;
	}
}

/******************************************************************************/

static void
parse_process_stats(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	sg_process_stats *(*list_func)(void) = NULL;
	bool ascending = TRUE;
	uint32_t number = 0;

	if (xsg_conf_find_command("process_name")) {
		*str = get_process_stats_process_name;
	} else if (xsg_conf_find_command("proctitle")) {
		*str = get_process_stats_proctitle;
	} else if (xsg_conf_find_command("pid")) {
		*num = get_process_stats_pid;
	} else if (xsg_conf_find_command("parent")) {
		*num = get_process_stats_parent;
	} else if (xsg_conf_find_command("pgid")) {
		*num = get_process_stats_pgid;
	} else if (xsg_conf_find_command("uid")) {
		*num = get_process_stats_uid;
	} else if (xsg_conf_find_command("euid")) {
		*num = get_process_stats_euid;
	} else if (xsg_conf_find_command("gid")) {
		*num = get_process_stats_gid;
	} else if (xsg_conf_find_command("egid")) {
		*num = get_process_stats_egid;
	} else if (xsg_conf_find_command("proc_size")) {
		*num = get_process_stats_proc_size;
	} else if (xsg_conf_find_command("proc_resident")) {
		*num = get_process_stats_proc_resident;
	} else if (xsg_conf_find_command("time_spent")) {
		*num = get_process_stats_time_spent;
	} else if (xsg_conf_find_command("cpu_percent")) {
		*num = get_process_stats_cpu_percent;
	} else if (xsg_conf_find_command("nice")) {
		*num = get_process_stats_nice;
	} else if (xsg_conf_find_command("state")) {
		*num = get_process_stats_state_number;
		*str = get_process_stats_state_string;
	} else {
		xsg_conf_error("process_name, proctitle, pid, parent, pgid, "
				"uid, euid, gid, egid, proc_size, "
				"proc_resident, time_spent, cpu_percent, "
				"nice or state expected");
	}

	if (xsg_conf_find_command("name")) {
		list_func = get_process_stats_orderedby_name;
	} else if (xsg_conf_find_command("pid")) {
		list_func = get_process_stats_orderedby_pid;
	} else if (xsg_conf_find_command("uid")) {
		list_func = get_process_stats_orderedby_uid;
	} else if (xsg_conf_find_command("gid")) {
		list_func = get_process_stats_orderedby_gid;
	} else if (xsg_conf_find_command("size")) {
		list_func = get_process_stats_orderedby_size;
	} else if (xsg_conf_find_command("res")) {
		list_func = get_process_stats_orderedby_res;
	} else if (xsg_conf_find_command("cpu")) {
		list_func = get_process_stats_orderedby_cpu;
	} else if (xsg_conf_find_command("time")) {
		list_func = get_process_stats_orderedby_time;
	} else {
		xsg_conf_error("name, pid, uid, gid, size, res, cpu or time "
				"expected");
	}

	if (xsg_conf_find_command("ascending")) {
		ascending = TRUE;
	} else if (xsg_conf_find_command("descending")) {
		ascending = FALSE;
	} else {
		xsg_conf_error("ascending or descending expected");
	}

	number = xsg_conf_read_uint();

	if (*str == get_process_stats_state_string) {
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

static double
get_process_count_total(void *arg)
{
	if (likely(process_count != NULL)) {
		xsg_debug("get_process_count_total: %f",
				(double) process_count->total);
		return (double) process_count->total;
	} else {
		xsg_debug("get_process_count_total: UNKNOWN");
		return DNAN;
	}
}

static double
get_process_count_running(void *arg)
{
	if (likely(process_count != NULL)) {
		xsg_debug("get_process_count_running: %f",
				(double) process_count->running);
		return (double) process_count->running;
	} else {
		xsg_debug("get_process_count_running: UNKNOWN");
		return DNAN;
	}
}

static double
get_process_count_sleeping(void *arg)
{
	if (likely(process_count != NULL)) {
		xsg_debug("get_process_count_sleeping: %f",
				(double) process_count->sleeping);
		return (double) process_count->sleeping;
	} else {
		xsg_debug("get_process_count_sleeping: UNKNOWN");
		return DNAN;
	}
}

static double
get_process_count_stopped(void *arg)
{
	if (likely(process_count != NULL)) {
		xsg_debug("get_process_count_stopped: %f",
				(double) process_count->stopped);
		return (double) process_count->stopped;
	} else {
		xsg_debug("get_process_count_stopped: UNKNOWN");
		return DNAN;
	}
}

static double
get_process_count_zombie(void *arg)
{
	if (likely(process_count != NULL)) {
		xsg_debug("get_process_count_zombie: %f",
				(double) process_count->zombie);
		return (double) process_count->zombie;
	} else {
		xsg_debug("get_process_count_zombie: UNKNOWN");
		return DNAN;
	}
}

/******************************************************************************/

static void
parse_process_count(
	uint64_t update,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	if (xsg_conf_find_command("total")) {
		*num = get_process_count_total;
	} else if (xsg_conf_find_command("running")) {
		*num = get_process_count_running;
	} else if (xsg_conf_find_command("sleeping")) {
		*num = get_process_count_sleeping;
	} else if (xsg_conf_find_command("stopped")) {
		*num = get_process_count_stopped;
	} else if (xsg_conf_find_command("zombie")) {
		*num = get_process_count_zombie;
	} else {
		xsg_conf_error("total, running, sleeping, stopped or zombie "
				"expected");
	}
}

/******************************************************************************
 *
 * Module functions
 *
 ******************************************************************************/

static void
init_stats(void)
{
	static bool first_time = TRUE;

	if (!first_time) {
		return;
	}

	first_time = FALSE;

	xsg_message("running sg_init()");
	sg_init();

	xsg_message("running sg_snapshot()");
	sg_snapshot();

	update_stats(0);
}

static void
shutdown_stats(void)
{
	xsg_message("running sg_shutdown()");
	sg_shutdown();
}

/******************************************************************************/

static void
parse_statgrab(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	char *(**str)(void *),
	void **arg
)
{
	xsg_main_add_init_func(init_stats);
	xsg_main_add_update_func(update_stats);
	xsg_main_add_shutdown_func(shutdown_stats);

	if (xsg_conf_find_command("host_info")){
		parse_host_info(update, num, str, arg);
		add_stat(update, get_host_info);
	} else if (xsg_conf_find_command("cpu_stats")) {
		parse_cpu_stats(update, num, str, arg);
		add_stat(update, get_cpu_stats);
	} else if (xsg_conf_find_command("cpu_stats_diff")) {
		parse_cpu_stats_diff(update, num, str, arg);
		add_stat(update, get_cpu_stats_diff);
	} else if (xsg_conf_find_command("cpu_percents")) {
		parse_cpu_percents(update, num, str, arg);
		add_stat(update, get_cpu_percents);
	} else if (xsg_conf_find_command("mem_stats")) {
		parse_mem_stats(update, num, str, arg);
		add_stat(update, get_mem_stats);
	} else if (xsg_conf_find_command("load_stats")) {
		parse_load_stats(update, num, str, arg);
		add_stat(update, get_load_stats);
	} else if (xsg_conf_find_command("user_stats")) {
		parse_user_stats(update, num, str, arg);
		add_stat(update, get_user_stats);
	} else if (xsg_conf_find_command("swap_stats")) {
		parse_swap_stats(update, num, str, arg);
		add_stat(update, get_swap_stats);
	} else if (xsg_conf_find_command("fs_stats")) {
		parse_fs_stats(update, num, str, arg);
		add_stat(update, get_fs_stats);
	} else if (xsg_conf_find_command("disk_io_stats")) {
		parse_disk_io_stats(update, num, str, arg);
		add_stat(update, get_disk_io_stats);
	} else if (xsg_conf_find_command("disk_io_stats_diff")) {
		parse_disk_io_stats_diff(update, num, str, arg);
		add_stat(update, get_disk_io_stats_diff);
	} else if (xsg_conf_find_command("network_io_stats")) {
		parse_network_io_stats(update, num, str, arg);
		add_stat(update, get_network_io_stats);
	} else if (xsg_conf_find_command("network_io_stats_diff")) {
		parse_network_io_stats_diff(update, num, str, arg);
		add_stat(update, get_network_io_stats_diff);
	} else if (xsg_conf_find_command("network_iface_stats")) {
		parse_network_iface_stats(update, num, str, arg);
		add_stat(update, get_network_iface_stats);
	} else if (xsg_conf_find_command("page_stats")) {
		parse_page_stats(update, num, str, arg);
		add_stat(update, get_page_stats);
	} else if (xsg_conf_find_command("page_stats_diff")) {
		parse_page_stats_diff(update, num, str, arg);
		add_stat(update, get_page_stats_diff);
	} else if (xsg_conf_find_command("process_stats")) {
		parse_process_stats(update, num, str, arg);
		add_stat(update, get_process_stats);
	} else if (xsg_conf_find_command("process_count")) {
		parse_process_count(update, num, str, arg);
		add_stat(update, get_process_count);
	} else {
		xsg_conf_error("host_info, cpu_stats, cpu_stats_diff, "
				"cpu_percents, mem_stats, load_stats, "
				"user_stats, swap_stats, fs_stats, "
				"disk_io_stats, disk_io_stats_diff, "
				"network_io_stats, network_io_stats_difff, "
				"network_iface_stats, page_stats, "
				"page_stats_diff, process_stats or "
				"process_count expected");
	}
}

static void
help_align(xsg_string_t *string)
{
	char *s = string->str + string->len;
	int n = 0;

	while (s > string->str && s[0] != '\n') {
		s--;
		n++;
	}

	while (n++ <= 64) {
		xsg_string_append_c(string, ' ');
	}
}

static void
help2n0(xsg_string_t *string, char *s1, char *s2, double num)
{
	xsg_string_append_printf(string, "N %s:%s:%s ",
			XSG_MODULE_NAME, s1, s2);
	help_align(string);
	xsg_string_append_printf(string, "%.0f\n", num);
}

static void
help3n0(xsg_string_t *string, char *s1, char *s2, char *s3, double num)
{
	xsg_string_append_printf(string, "N %s:%s:%s:%s ",
			XSG_MODULE_NAME, s1, s2, s3);
	help_align(string);
	xsg_string_append_printf(string, "%.0f\n", num);
}

static void
help2n2(xsg_string_t *string, char *s1, char *s2, double num)
{
	xsg_string_append_printf(string, "N %s:%s:%s ",
			XSG_MODULE_NAME, s1, s2);
	help_align(string);
	xsg_string_append_printf(string, "%.2f\n", num);
}

static void
help2s(xsg_string_t *string, char *s1, char *s2, char *str)
{
	xsg_string_append_printf(string, "S %s:%s:%s ",
			XSG_MODULE_NAME, s1, s2);
	help_align(string);
	xsg_string_append_printf(string, "%s\n", str);
}

static void
help3s(xsg_string_t *string, char *s1, char *s2, char *s3, char *str)
{
	xsg_string_append_printf(string, "S %s:%s:%s:%s ",
			XSG_MODULE_NAME, s1, s2, s3);
	help_align(string);
	xsg_string_append_printf(string, "%s\n", str);
}

static const char *
help_statgrab(void)
{
	static xsg_string_t *string = NULL;
	int i;

	init_stats();

	if (string == NULL) {
		string = xsg_string_new(NULL);
	} else {
		xsg_string_truncate(string, 0);
	}

	get_fs_stats(0);

	for (i = 0; i < fs_entries; i++) {
		char name[128];

		snprintf(name, sizeof(name), "STATGRAB_DEVICE%d", i);
		xsg_setenv(name, fs_stats_by_device_name[i].device_name, TRUE);

		xsg_string_append_printf(string, "%s:          %s\n", name,
				fs_stats_by_device_name[i].device_name);
	}

	for (i = 0; i < fs_entries; i++) {
		char name[128];

		snprintf(name, sizeof(name), "STATGRAB_MNTPOINT%d", i);
		xsg_setenv(name, fs_stats_by_mnt_point[i].mnt_point, TRUE);

		xsg_string_append_printf(string, "%s:        %s\n", name,
				fs_stats_by_mnt_point[i].mnt_point);
	}

	get_disk_io_stats(0);

	for (i = 0; i < disk_io_entries; i++) {
		char name[128];

		snprintf(name, sizeof(name), "STATGRAB_DISK%d", i);
		xsg_setenv(name, disk_io_stats[i].disk_name, TRUE);

		xsg_string_append_printf(string, "%s:            %s\n", name,
				disk_io_stats[i].disk_name);
	}

	get_network_iface_stats(0);

	for (i = 0; i < network_iface_entries; i++) {
		char name[128];

		snprintf(name, sizeof(name), "STATGRAB_INTERFACE%d", i);
		xsg_setenv(name, network_iface_stats[i].interface_name, TRUE);

		xsg_string_append_printf(string, "%s:       %s\n", name,
				network_iface_stats[i].interface_name);
	}

	xsg_string_append_c(string, '\n');

	get_host_info(0);
	help2s(string, "host_info", "os_name", get_host_info_os_name(NULL));
	help2s(string, "host_info", "os_release", get_host_info_os_release(NULL));
	help2s(string, "host_info", "os_version", get_host_info_os_version(NULL));
	help2s(string, "host_info", "platform", get_host_info_platform(NULL));
	help2s(string, "host_info", "hostname", get_host_info_hostname(NULL));
	help2n0(string, "host_info", "uptime", get_host_info_uptime(NULL));
	xsg_string_append_c(string, '\n');

	get_cpu_stats(0);
	help2n0(string, "cpu_stats", "user", get_cpu_stats_user(NULL));
	help2n0(string, "cpu_stats", "kernel", get_cpu_stats_kernel(NULL));
	help2n0(string, "cpu_stats", "idle", get_cpu_stats_idle(NULL));
	help2n0(string, "cpu_stats", "iowait", get_cpu_stats_iowait(NULL));
	help2n0(string, "cpu_stats", "swap", get_cpu_stats_swap(NULL));
	help2n0(string, "cpu_stats" ,"nice", get_cpu_stats_nice(NULL));
	help2n0(string, "cpu_stats", "total", get_cpu_stats_total(NULL));
	xsg_string_append_c(string, '\n');

	get_cpu_stats_diff(0);
	help2n0(string, "cpu_stats_diff", "user", 0.0);
	help2n0(string, "cpu_stats_diff", "kernel", 0.0);
	help2n0(string, "cpu_stats_diff", "idle", 0.0);
	help2n0(string, "cpu_stats_diff", "iowait", 0.0);
	help2n0(string, "cpu_stats_diff", "swap", 0.0);
	help2n0(string, "cpu_stats_diff", "nice", 0.0);
	help2n0(string, "cpu_stats_diff", "total", 0.0);
	xsg_string_append_c(string, '\n');

	get_cpu_percents(0);
	help2n0(string, "cpu_percents", "user", DNAN);
	help2n0(string, "cpu_percents", "kernel", DNAN);
	help2n0(string, "cpu_percents", "idle", DNAN);
	help2n0(string, "cpu_percents", "iowait", DNAN);
	help2n0(string, "cpu_percents", "swap", DNAN);
	help2n0(string, "cpu_percents", "nice", DNAN);
	xsg_string_append_c(string, '\n');

	get_mem_stats(0);
	help2n0(string, "mem_stats", "total", get_mem_stats_total(NULL));
	help2n0(string, "mem_stats", "free", get_mem_stats_free(NULL));
	help2n0(string, "mem_stats", "used", get_mem_stats_used(NULL));
	help2n0(string, "mem_stats", "cache", get_mem_stats_cache(NULL));
	xsg_string_append_c(string, '\n');

	get_load_stats(0);
	help2n2(string, "load_stats", "min1", get_load_stats_min1(NULL));
	help2n2(string, "load_stats", "min5", get_load_stats_min5(NULL));
	help2n2(string, "load_stats", "min15", get_load_stats_min15(NULL));
	xsg_string_append_c(string, '\n');

	get_user_stats(0);
	help2s(string, "user_stats", "name_list", get_user_stats_name_list(NULL));
	help2n0(string, "user_stats", "num_entries", get_user_stats_num_entries(NULL));
	xsg_string_append_c(string, '\n');

	get_swap_stats(0);
	help2n0(string, "swap_stats", "total", get_swap_stats_total(NULL));
	help2n0(string, "swap_stats", "used", get_swap_stats_used(NULL));
	help2n0(string, "swap_stats", "free", get_swap_stats_free(NULL));
	xsg_string_append_c(string, '\n');

	get_fs_stats(0);
	for (i = 0; i < fs_entries + fs_entries; i++) {
		char *s;

		if (i < fs_entries)
			s = fs_stats_by_device_name[i].device_name;
		else
			s = fs_stats_by_mnt_point[i - fs_entries].mnt_point;

		help3s(string, "fs_stats", s, "device_name",
				get_fs_stats_device_name(s));
		help3s(string, "fs_stats", s, "fs_type",
				get_fs_stats_fs_type(s));
		help3s(string, "fs_stats", s, "mnt_point",
				get_fs_stats_mnt_point(s));
		help3n0(string, "fs_stats", s, "size",
				get_fs_stats_size(s));
		help3n0(string, "fs_stats", s, "used",
				get_fs_stats_used(s));
		help3n0(string, "fs_stats", s, "avail",
				get_fs_stats_avail(s));
		help3n0(string, "fs_stats", s, "total_inodes",
				get_fs_stats_total_inodes(s));
		help3n0(string, "fs_stats", s, "used_inodes",
				get_fs_stats_used_inodes(s));
		help3n0(string, "fs_stats", s, "free_inodes",
				get_fs_stats_free_inodes(s));
		help3n0(string, "fs_stats", s, "avail_inodes",
				get_fs_stats_avail_inodes(s));
		help3n0(string, "fs_stats", s, "io_size",
				get_fs_stats_io_size(s));
		help3n0(string, "fs_stats", s, "block_size",
				get_fs_stats_block_size(s));
		help3n0(string, "fs_stats", s, "total_blocks",
				get_fs_stats_total_blocks(s));
		help3n0(string, "fs_stats", s, "free_blocks",
				get_fs_stats_free_blocks(s));
		help3n0(string, "fs_stats", s, "used_blocks",
				get_fs_stats_used_blocks(s));
		help3n0(string, "fs_stats", s, "avail_blocks",
				get_fs_stats_avail_blocks(s));
		xsg_string_append_c(string, '\n');
	}

	get_disk_io_stats(0);
	for (i = 0; i < disk_io_entries; i++) {
		char *s = disk_io_stats[i].disk_name;

		help3n0(string, "disk_io_stats", s, "read_bytes",
				get_disk_io_stats_read_bytes(s));
		help3n0(string, "disk_io_stats", s, "write_bytes",
				get_disk_io_stats_write_bytes(s));
		xsg_string_append_c(string, '\n');
	}
	for (i = 0; i < disk_io_entries; i++) {
		char *s = disk_io_stats[i].disk_name;

		help3n0(string, "disk_io_stats_diff", s, "read_bytes", 0.0);
		help3n0(string, "disk_io_stats_diff", s, "write_bytes", 0.0);
		xsg_string_append_c(string, '\n');
	}

	get_network_io_stats(0);
	for (i = 0; i < network_io_entries; i++) {
		char *s = network_io_stats[i].interface_name;

		help3n0(string, "network_io_stats", s, "tx",
				get_network_io_stats_tx(s));
		help3n0(string, "network_io_stats", s, "rx",
				get_network_io_stats_rx(s));
		help3n0(string, "network_io_stats", s, "ipackets",
				get_network_io_stats_ipackets(s));
		help3n0(string, "network_io_stats", s, "opackets",
				get_network_io_stats_opackets(s));
		help3n0(string, "network_io_stats", s, "ierrors",
				get_network_io_stats_ierrors(s));
		help3n0(string, "network_io_stats", s, "oerrors",
				get_network_io_stats_oerrors(s));
		help3n0(string, "network_io_stats", s, "collisions",
				get_network_io_stats_collisions(s));
		xsg_string_append_c(string, '\n');
	}
	for (i = 0; i < network_io_entries; i++) {
		char *s = network_io_stats[i].interface_name;

		help3n0(string, "network_io_stats_diff", s, "tx", 0.0);
		help3n0(string, "network_io_stats_diff", s, "rx", 0.0);
		help3n0(string, "network_io_stats_diff", s, "ipackets", 0.0);
		help3n0(string, "network_io_stats_diff", s, "opackets", 0.0);
		help3n0(string, "network_io_stats_diff", s, "ierrors", 0.0);
		help3n0(string, "network_io_stats_diff", s, "oerrors", 0.0);
		help3n0(string, "network_io_stats_diff", s, "collisions", 0.0);
		xsg_string_append_c(string, '\n');
	}

	get_network_iface_stats(0);
	for (i = 0; i < network_iface_entries; i++) {
		char *s = network_iface_stats[i].interface_name;
		network_iface_stats_duplex_data_t data = {
			s, "FULL", "HALF", "UNKNOWN"
		};

		help3n0(string, "network_iface_stats", s, "speed",
				get_network_iface_stats_speed(s));
		help3n0(string, "network_iface_stats", s, "duplex",
				get_network_iface_stats_duplex_number(&data));
		help3s(string, "network_iface_stats", s, "duplex:FULL:HALF:UNKNOWN",
				get_network_iface_stats_duplex_string(&data));
		xsg_string_append_c(string, '\n');
	}

	get_page_stats(0);
	help2n0(string, "page_stats", "pages_pagein",
			get_page_stats_pages_pagein(NULL));
	help2n0(string, "page_stats", "pages_pageout",
			get_page_stats_pages_pageout(NULL));
	xsg_string_append_c(string, '\n');
	help2n0(string, "page_stats_diff", "pages_pagein", 0.0);
	help2n0(string, "page_stats_diff", "pages_pageout", 0.0);
	xsg_string_append_c(string, '\n');

	xsg_string_append_printf(string, "S %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "process_name",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "S %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "proctitle",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "pid",
			"<ordered_by>", "{ascending|descending} ", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "parent",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "pgid",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "uid",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "euid",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "gid",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "egid",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "proc_size",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "proc_resident",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "time_spent",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "cpu_percent",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "N %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "nice",
			"<ordered_by>", "{ascending|descending}", "<number>");
	xsg_string_append_printf(string, "X %s:%s:%s:%s:%s:%s\n",
			XSG_MODULE_NAME, "process_stats", "state",
			"<ordered_by>", "{ascending|descending}",
			"<number>:<running>:<sleeping>:<stopped>:<zombie>:<unknown>");
	xsg_string_append_c(string, '\n');

	get_process_count(0);
	help2n0(string, "process_count", "total",
			get_process_count_total(NULL));
	help2n0(string, "process_count", "running",
			get_process_count_running(NULL));
	help2n0(string, "process_count", "sleeping",
			get_process_count_sleeping(NULL));
	help2n0(string, "process_count", "stopped",
			get_process_count_stopped(NULL));
	help2n0(string, "process_count", "zombie",
			get_process_count_zombie(NULL));

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_statgrab, help_statgrab, "libstatgrab (get system statistics)");

