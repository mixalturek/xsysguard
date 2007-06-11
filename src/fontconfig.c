/* fontconfig.c
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
#include <dlfcn.h>

/******************************************************************************/

typedef int			FcBool;
typedef unsigned char		FcChar8;
typedef struct _FcConfig	FcConfig;
typedef struct _FcStrList	FcStrList;

/******************************************************************************/

static FcBool (*FcInit)(void) = NULL;
static FcStrList *(*FcConfigGetFontDirs)(FcConfig *config) = NULL;
static FcChar8 *(*FcStrListNext)(FcStrList *list) = NULL;
static void (*FcStrListDone)(FcStrList *list) = NULL;
static void (*FcFini)(void) = NULL;

/******************************************************************************/

#define CHECK(var) if (!(var)) { \
	xsg_message("Cannot load libfontconfig: %s", dlerror()); \
	dlclose(lib); \
	return FALSE; \
}

char **xsg_fontconfig_get_path(void) {
	xsg_list_t *l, *list = NULL;
	FcStrList *fc_list;
	FcChar8 *dir;
	unsigned n = 0;
	char **path;
	void *lib;

	lib = dlopen("libfontconfig.so", RTLD_LAZY | RTLD_LOCAL);

	if (!lib) {
		xsg_message("Cannot load libfontconfig: %s", dlerror());
		return NULL;
	}

	FcInit = dlsym(lib, "FcInit");
	CHECK(FcInit);
	FcConfigGetFontDirs = dlsym(lib, "FcConfigGetFontDirs");
	CHECK(FcConfigGetFontDirs);
	FcStrListNext = dlsym(lib, "FcStrListNext");
	CHECK(FcStrListNext);
	FcStrListDone = dlsym(lib, "FcStrListDone");
	CHECK(FcStrListDone);
	FcFini = dlsym(lib, "FcFini");
	CHECK(FcFini);

	if (!FcInit()) {
		xsg_message("FcInit() failed");
		dlclose(lib);
		return NULL;
	}

	if ((fc_list = FcConfigGetFontDirs(NULL)) == NULL) {
		xsg_message("FcConfigGetFontDirs() returned NULL");
		dlclose(lib);
		return NULL;
	}

	while ((dir = FcStrListNext(fc_list)) != NULL) {
		list = xsg_list_prepend(list, xsg_strdup((char *) dir));
		n++;
	}

	FcStrListDone(fc_list);
	FcFini();

	dlclose(lib);

	path = xsg_new(char *, n + 1);

	path[n] = NULL;
	for (l = list; l != NULL; l = l->next)
		path[--n] = l->data;

	xsg_list_free(list);

	return path;
}

