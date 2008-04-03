/* lstat.c
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

/******************************************************************************/

#define DECL_GET(field)							\
static double								\
get_lstat_##field(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(lstat((const char *) arg, &buf) != 0)) {		\
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
get_lstat_##type(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(lstat((const char *) arg, &buf) != 0)) {		\
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
get_lstat_##flag(void *arg)						\
{									\
	struct stat buf;						\
									\
	if (unlikely(lstat((const char *) arg, &buf) != 0)) {		\
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
parse_lstat(
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
		*num = get_lstat_dev;
	} else if (xsg_conf_find_command("ino")) {
		*num = get_lstat_ino;
	} else if (xsg_conf_find_command("mode")) {
		*num = get_lstat_mode;
	} else if (xsg_conf_find_command("nlink")) {
		*num = get_lstat_nlink;
	} else if (xsg_conf_find_command("uid")) {
		*num = get_lstat_uid;
	} else if (xsg_conf_find_command("gid")) {
		*num = get_lstat_gid;
	} else if (xsg_conf_find_command("rdev")) {
		*num = get_lstat_rdev;
	} else if (xsg_conf_find_command("size")) {
		*num = get_lstat_size;
	} else if (xsg_conf_find_command("blksize")) {
		*num = get_lstat_blksize;
	} else if (xsg_conf_find_command("blocks")) {
		*num = get_lstat_blocks;
	} else if (xsg_conf_find_command("atime")) {
		*num = get_lstat_atime;
	} else if (xsg_conf_find_command("mtime")) {
		*num = get_lstat_mtime;
	} else if (xsg_conf_find_command("ctime")) {
		*num = get_lstat_ctime;
	} else if (xsg_conf_find_command("isreg")) {
		*num = get_lstat_ISREG;
	} else if (xsg_conf_find_command("isdir")) {
		*num = get_lstat_ISDIR;
	} else if (xsg_conf_find_command("ischr")) {
		*num = get_lstat_ISCHR;
	} else if (xsg_conf_find_command("isblk")) {
		*num = get_lstat_ISBLK;
	} else if (xsg_conf_find_command("isfifo")) {
		*num = get_lstat_ISFIFO;
	} else if (xsg_conf_find_command("islnk")) {
		*num = get_lstat_ISLNK;
	} else if (xsg_conf_find_command("issock")) {
		*num = get_lstat_ISSOCK;
	} else if (xsg_conf_find_command("isuid")) {
		*num = get_lstat_ISUID;
	} else if (xsg_conf_find_command("isgid")) {
		*num = get_lstat_ISGID;
	} else if (xsg_conf_find_command("isvtx")) {
		*num = get_lstat_ISVTX;
	} else if (xsg_conf_find_command("irusr")) {
		*num = get_lstat_IRUSR;
	} else if (xsg_conf_find_command("iwusr")) {
		*num = get_lstat_IWUSR;
	} else if (xsg_conf_find_command("ixusr")) {
		*num = get_lstat_IXUSR;
	} else if (xsg_conf_find_command("irgrp")) {
		*num = get_lstat_IRGRP;
	} else if (xsg_conf_find_command("iwgrp")) {
		*num = get_lstat_IWGRP;
	} else if (xsg_conf_find_command("ixgrp")) {
		*num = get_lstat_IXGRP;
	} else if (xsg_conf_find_command("iroth")) {
		*num = get_lstat_IROTH;
	} else if (xsg_conf_find_command("iwoth")) {
		*num = get_lstat_IWOTH;
	} else if (xsg_conf_find_command("ixoth")) {
		*num = get_lstat_IXOTH;
	} else {
		xsg_conf_error("dev, ino, mode, nlink, uid, gid, rdev, size, "
				"blksize, blocks, atime, mtime, ctime, isreg, "
				"isdir, ischr, isblk, isfifo, islnk, issock, "
				"isuid, isgid, isvtx, irusr, iwusr, ixusr, "
				"irgrp, iwgrp, ixgrp, iroth, iwoth or ixoth "
				"expected");
	}
}

static const char *
help_lstat(void)
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

XSG_MODULE(parse_lstat, help_lstat, "get symbolic link status");

