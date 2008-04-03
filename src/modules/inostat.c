/* inostat.c
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

#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include <xsysguard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <alloca.h>

/******************************************************************************/

typedef struct _inostat_t {
	char *filename;
	unsigned varc;
	xsg_var_t **varv;
	xsg_main_poll_t poll;
	int wd;
} inostat_t;

/******************************************************************************/

static xsg_list_t *inostat_list = NULL;

/******************************************************************************/

#define DECL_GET(field)							\
static double								\
get_inostat_##field(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(stat((const char *) arg, &buf) != 0)) {		\
		return DNAN;						\
	} else {							\
		return (double) buf.st_##field;				\
	}								\
}

DECL_GET(dev)
DECL_GET(ino)
DECL_GET(mode)
DECL_GET(nlink)
DECL_GET(uid)
DECL_GET(gid)
DECL_GET(rdev)
DECL_GET(size)
DECL_GET(blksize)
DECL_GET(blocks)
DECL_GET(atime)
DECL_GET(mtime)
DECL_GET(ctime)

#undef DECL_GET

/******************************************************************************/

#define DECL_GET(type)							\
static double								\
get_inostat_##type(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(stat((const char *) arg, &buf) != 0)) {		\
		return DNAN;						\
	} else {							\
		return (S_##type(buf.st_mode)) ? 1.0 : 0.0;		\
	}								\
}

DECL_GET(ISREG)
DECL_GET(ISDIR)
DECL_GET(ISCHR)
DECL_GET(ISBLK)
DECL_GET(ISFIFO)
DECL_GET(ISLNK)
DECL_GET(ISSOCK)

#undef DECL_GET

/******************************************************************************/

#define DECL_GET(flag)							\
static double								\
get_inostat_##flag(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(stat((const char *) arg, &buf) != 0)) {		\
		return DNAN;						\
	} else {							\
		return ((S_##flag) & buf.st_mode) ? 1.0 : 0.0;		\
	}								\
}

DECL_GET(ISUID)
DECL_GET(ISGID)
DECL_GET(ISVTX)
DECL_GET(IRUSR)
DECL_GET(IWUSR)
DECL_GET(IXUSR)
DECL_GET(IRGRP)
DECL_GET(IWGRP)
DECL_GET(IXGRP)
DECL_GET(IROTH)
DECL_GET(IWOTH)
DECL_GET(IXOTH)

#undef DECL_GET

/******************************************************************************/

static void
update_inostat(inostat_t *s)
{
	unsigned int i;

	for (i = 0; i < s->varc; i++) {
		xsg_var_dirty(s->varv[i]);
	}
}

static void
read_inostat_event(void *arg, xsg_main_poll_events_t events)
{
	inostat_t *s = (inostat_t *) arg;
	struct inotify_event event;
	ssize_t n;

	do {
		/* FIXME: n < sizeof(event) */
		n = read(s->poll.fd, &event, sizeof(event));
	} while (n == -1 && errno == EINTR);

	if (event.len > 0) {
		char *buffer;

		buffer = (char *) alloca(event.len);

		do {
			/* FIXME: n < event.len */
			n = read(s->poll.fd, buffer, event.len);
		} while (n == -1 && errno == EINTR);
	}

	update_inostat(s);
}

static void
add_inostat(const char *filename, xsg_var_t *var)
{
	xsg_list_t *l;
	inostat_t *s;
	int ifd;

	for (l = inostat_list; l; l = l->next) {
		s = (inostat_t *) l->data;

		if (strcmp(s->filename, filename) == 0) {
			s->varc++;
			s->varv = xsg_renew(xsg_var_t *, s->varv, s->varc);
			s->varv[s->varc - 1] = var;
			return;
		}
	}

	s = xsg_new(inostat_t, 1);

	s->filename = xsg_strdup(filename);
	s->varv = xsg_new(xsg_var_t *, 1);
	s->varc = 1;

	s->varv[0] = var;

	ifd = inotify_init();

	if (ifd < 0) {
		xsg_conf_error("inotify_init failed: %s", strerror(errno));
	}

	xsg_set_cloexec_flag(ifd, TRUE);

	s->poll.fd = ifd;
	s->poll.events = XSG_MAIN_POLL_READ;
	s->poll.func = read_inostat_event;
	s->poll.arg = s;
	s->wd = inotify_add_watch(ifd, s->filename, IN_ATTRIB
			| IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF | IN_UNMOUNT);

	if (s->wd < 0) {
		xsg_conf_error("inotify_add_watch failed: %s", strerror(errno));
	}

	xsg_main_add_poll(&s->poll);

	inostat_list = xsg_list_append(inostat_list, (void *) s);
}

/******************************************************************************/

static void
init_inostat(void)
{
	xsg_list_t *l;
	inostat_t *s;

	for (l = inostat_list; l; l = l->next) {
		s = (inostat_t *) l->data;

		update_inostat(s);
	}
}

/******************************************************************************/


static void
parse_inostat(
	uint64_t update,
	xsg_var_t *var,
	double (**num)(void *),
	const char *(**str)(void *),
	void **arg
)
{
	char *filename;

	filename = xsg_conf_read_string();

	arg = (void *) filename;

	if (xsg_conf_find_command("dev")) {
		*num = get_inostat_dev;
	} else if (xsg_conf_find_command("ino")) {
		*num = get_inostat_ino;
	} else if (xsg_conf_find_command("mode")) {
		*num = get_inostat_mode;
	} else if (xsg_conf_find_command("nlink")) {
		*num = get_inostat_nlink;
	} else if (xsg_conf_find_command("uid")) {
		*num = get_inostat_uid;
	} else if (xsg_conf_find_command("gid")) {
		*num = get_inostat_gid;
	} else if (xsg_conf_find_command("rdev")) {
		*num = get_inostat_rdev;
	} else if (xsg_conf_find_command("size")) {
		*num = get_inostat_size;
	} else if (xsg_conf_find_command("blksize")) {
		*num = get_inostat_blksize;
	} else if (xsg_conf_find_command("blocks")) {
		*num = get_inostat_blocks;
	} else if (xsg_conf_find_command("atime")) {
		*num = get_inostat_atime;
	} else if (xsg_conf_find_command("mtime")) {
		*num = get_inostat_mtime;
	} else if (xsg_conf_find_command("ctime")) {
		*num = get_inostat_ctime;
	} else if (xsg_conf_find_command("isreg")) {
		*num = get_inostat_ISREG;
	} else if (xsg_conf_find_command("isdir")) {
		*num = get_inostat_ISDIR;
	} else if (xsg_conf_find_command("ischr")) {
		*num = get_inostat_ISCHR;
	} else if (xsg_conf_find_command("isblk")) {
		*num = get_inostat_ISBLK;
	} else if (xsg_conf_find_command("isfifo")) {
		*num = get_inostat_ISFIFO;
	} else if (xsg_conf_find_command("islnk")) {
		*num = get_inostat_ISLNK;
	} else if (xsg_conf_find_command("issock")) {
		*num = get_inostat_ISSOCK;
	} else if (xsg_conf_find_command("isuid")) {
		*num = get_inostat_ISUID;
	} else if (xsg_conf_find_command("isgid")) {
		*num = get_inostat_ISGID;
	} else if (xsg_conf_find_command("isvtx")) {
		*num = get_inostat_ISVTX;
	} else if (xsg_conf_find_command("irusr")) {
		*num = get_inostat_IRUSR;
	} else if (xsg_conf_find_command("iwusr")) {
		*num = get_inostat_IWUSR;
	} else if (xsg_conf_find_command("ixusr")) {
		*num = get_inostat_IXUSR;
	} else if (xsg_conf_find_command("irgrp")) {
		*num = get_inostat_IRGRP;
	} else if (xsg_conf_find_command("iwgrp")) {
		*num = get_inostat_IWGRP;
	} else if (xsg_conf_find_command("ixgrp")) {
		*num = get_inostat_IXGRP;
	} else if (xsg_conf_find_command("iroth")) {
		*num = get_inostat_IROTH;
	} else if (xsg_conf_find_command("iwoth")) {
		*num = get_inostat_IWOTH;
	} else if (xsg_conf_find_command("ixoth")) {
		*num = get_inostat_IXOTH;
	} else {
		xsg_conf_error("dev, ino, mode, nlink, uid, gid, rdev, size, "
				"blksize, blocks, atime, mtime, ctime, isreg, "
				"isdir, ischr, isblk, isfifo, islnk, issock, "
				"isuid, isgid, isvtx, irusr, iwusr, ixusr, "
				"irgrp, iwgrp, ixgrp, iroth, iwoth or ixoth "
				"expected");
	}

	add_inostat(filename, var);

	xsg_main_add_init_func(init_inostat);
}

static const char *
help_inostat(void)
{
	static xsg_string_t *string = NULL;

	if (string != NULL) {
		return string->str;
	}

	string = xsg_string_new(NULL);

#define PRINT(n) \
	xsg_string_append_printf(string, "N %s:%s:%s\n", XSG_MODULE_NAME, \
			"<filename>", #n)

	PRINT(dev);
	PRINT(ino);
	PRINT(mode);
	PRINT(nlink);
	PRINT(uid);
	PRINT(gid);
	PRINT(rdev);
	PRINT(size);
	PRINT(blksize);
	PRINT(blocks);
	PRINT(atime);
	PRINT(mtime);
	PRINT(ctime);
	PRINT(isreg);
	PRINT(isdir);
	PRINT(ischr);
	PRINT(isblk);
	PRINT(isfifo);
	PRINT(islnk);
	PRINT(issock);
	PRINT(isuid);
	PRINT(isgid);
	PRINT(isvtx);
	PRINT(irusr);
	PRINT(iwusr);
	PRINT(ixusr);
	PRINT(irgrp);
	PRINT(iwgrp);
	PRINT(ixgrp);
	PRINT(iroth);
	PRINT(iwoth);
	PRINT(ixoth);

#undef PRINT

	return string->str;
}

/******************************************************************************/

XSG_MODULE(parse_inostat, help_inostat, "get file status using inotify");

