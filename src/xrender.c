/* xrender.c
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
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <string.h>

#include "xrender.h"
//#include "argb.h"

/******************************************************************************/

static Display *display = NULL;

/******************************************************************************/

typedef unsigned long Picture;
typedef unsigned long PictFormat;

typedef struct {
	short red;
	short redMask;
	short green;
	short greenMask;
	short blue;
	short blueMask;
	short alpha;
	short alphaMask;
} XRenderDirectFormat;

typedef struct {
	PictFormat id;
	int type;
	int depth;
	XRenderDirectFormat direct;
	Colormap colormap;
} XRenderPictFormat;

typedef struct {
	int repeat;
	Picture alpha_map;
	int alpha_x_origin;
	int alpha_y_origin;
	int clip_x_origin;
	int clip_y_origin;
	Pixmap clip_mask;
	Bool graphics_exposures;
	int subwindow_mode;
	int poly_edge;
	int poly_mode;
	Atom dither;
	Bool component_alpha;
} XRenderPictureAttributes;

/******************************************************************************/

// xsg_xrender_init
static Bool (*XRenderQueryExtension)(Display *dpy, int *event_basep, int *error_basep) = NULL;
static Status (*XRenderQueryVersion)(Display *dpy, int *major_version, int *minor_version) = NULL;
static Bool (*XCompositeQueryExtension)(Display *dpy, int *event_basep, int *error_basep) = NULL;
static Status (*XCompositeQueryVersion)(Display *dpy, int *major_version, int *minor_version) = NULL;

// xsg_xrender_find_visual
static XRenderPictFormat *(*XRenderFindVisualFormat)(Display *dpy, _Xconst Visual *visual) = NULL;

// xsg_xrender_redirect
static void (*XCompositeRedirectSubwindows)(Display *dpy, Window window, int update) = NULL;

// xsg_xrender_render
static XRenderPictFormat *(*XRenderFindStandardFormat)(Display *dpy, int format) = NULL;
static Picture (*XRenderCreatePicture)(Display *dpy, Drawable drawable, _Xconst XRenderPictFormat *format, unsigned long valuemask, _Xconst XRenderPictureAttributes *attributes) = NULL;
static void (*XRenderComposite)(Display *dpy, int op, Picture src, Picture mask, Picture dst, int src_x, int src_y, int mask_x, int mask_y, int dst_x, int dst_y, unsigned int width, unsigned int height);
static void (*XRenderFreePicture)(Display *dpy, Picture picture) = NULL;

/******************************************************************************/

static bool am_big_endian(void) {
	long one = 1;
	return !(*((char *)(&one)));
}

/******************************************************************************/

void xsg_xrender_init(Display *dpy) {
	static bool ok = FALSE;
	void *librender, *libcomposite;
	int opcode = 0, event = 0, error = 0;
	int major, minor;

	if (ok)
		return;
	else
		ok = TRUE;

	if ((librender = dlopen("libXrender.so", RTLD_LAZY | RTLD_LOCAL)) == NULL)
		xsg_error("Cannot open libXrender.so: %s", dlerror());

	if ((libcomposite = dlopen("libXcomposite.so", RTLD_LAZY | RTLD_LOCAL)) == NULL)
		xsg_error("Cannot open libXcomposite.so: %s", dlerror());

	if ((XRenderQueryExtension = (Bool (*)(Display *, int *, int *)) (intptr_t) dlsym(librender, "XRenderQueryExtension")) == NULL)
		xsg_error("Cannot find symbol XRenderQueryExtension: %s", dlerror());

	if ((XRenderQueryVersion = (Status (*)(Display *, int *, int *)) (intptr_t) dlsym(librender, "XRenderQueryVersion")) == NULL)
		xsg_error("Cannot find symbol XRenderQueryVersion: %S", dlerror());

	if ((XCompositeQueryExtension = (Bool (*)(Display *, int *, int *)) (intptr_t) dlsym(libcomposite, "XCompositeQueryExtension")) == NULL)
		xsg_error("Cannot find symbol XCompositeQueryExtension: %s", dlerror());

	if ((XCompositeQueryVersion = (Status (*)(Display *, int *, int *)) (intptr_t) dlsym(libcomposite, "XCompositeQueryVersion")) == NULL)
		xsg_error("Cannot find symbol XCompositeQueryVersion: %s", dlerror());

	if ((XRenderFindVisualFormat = (XRenderPictFormat *(*)(Display *, _Xconst Visual *)) (intptr_t) dlsym(librender, "XRenderFindVisualFormat")) == NULL)
		xsg_error("Cannot find symbol XRenderFindVisualFormat: %s", dlerror());

	if ((XCompositeRedirectSubwindows = (void (*)(Display *, Window, int)) (intptr_t) dlsym(libcomposite, "XCompositeRedirectSubwindows")) == NULL)
		xsg_error("Cannot find symbol XCompositeRedirectSubwindows: %s", dlerror());

	if ((XRenderFindStandardFormat = (XRenderPictFormat *(*)(Display *, int)) (intptr_t) dlsym(librender, "XRenderFindStandardFormat")) == NULL)
		xsg_error("Cannot find symbol XRenderFindStandardFormat: %s", dlerror());

	if ((XRenderCreatePicture = (Picture (*)(Display *, Drawable, _Xconst XRenderPictFormat *, unsigned long, _Xconst XRenderPictureAttributes *)) (intptr_t) dlsym(librender, "XRenderCreatePicture")) == NULL)
		xsg_error("Cannot find symbol XRenderCreatePicture: %s", dlerror());

	if ((XRenderComposite = (void (*)(Display *, int, Picture, Picture, Picture, int, int, int, int, int, int, unsigned int, unsigned int)) (intptr_t) dlsym(librender, "XRenderComposite")) == NULL)
		xsg_error("Cannot find symbol XRenderComposite: %s", dlerror());

	if ((XRenderFreePicture = (void (*)(Display *, Picture)) (intptr_t) dlsym(librender, "XRenderFreePicture")) == NULL)
		xsg_error("Cannot find symbol XRenderFreePicture: %s", dlerror());

	display = dpy;

	if (!XQueryExtension(display, "RENDER", &opcode, &event, &error))
		xsg_error("XQueryExtension RENDER failed: opcode=%d, event=%d, error=%d", opcode, event, error);

	if (!XRenderQueryExtension(display, &event, &error))
		xsg_error("XRenderQueryExtension failed: event=%d, error=%d", event, error);

	XRenderQueryVersion(display, &major, &minor);
	xsg_message("XRenderQueryVersion: %d.%d", major, minor);

	if (!XQueryExtension(display, "Composite", &opcode, &event, &error))
		xsg_error("XQueryExtension COMPOSITE failed: opcode=%d, event=%d, error=%d", opcode, event, error);

	if (!XCompositeQueryExtension(display, &event, &error))
		xsg_error("XCompositeQueryExtension failed: event=%d, error=%d", event, error);

	XCompositeQueryVersion(display, &major, &minor);
	xsg_message("XCompositeQueryVersion: %d.%d", major, minor);
}

/******************************************************************************/

Visual *xsg_xrender_find_visual(int screen) {
	XVisualInfo *xvi;
	XVisualInfo template;
	int nvi, i;
	XRenderPictFormat *format;
	Visual *visual = NULL;

	template.screen = screen;
	template.depth = 32;
	template.class = TrueColor;

	xvi = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask | VisualClassMask, &template, &nvi);

	if (xvi == NULL)
		xsg_error("Cannot find an argb visual");

	for (i = 0; i < nvi; i++) {
		format = XRenderFindVisualFormat(display, xvi[i].visual);
		if (format->type == 1 && format->direct.alphaMask) {
			visual = xvi[i].visual;
			break;
		}
	}

	XFree(xvi);

	if (visual == NULL)
		xsg_error("Cannot find an argb visual");

	return visual;
}

/******************************************************************************/

void xsg_xrender_redirect(Window window) {
	XCompositeRedirectSubwindows(display, window, 0);
}

/******************************************************************************/

static XImage *create_ximage(Visual *visual, unsigned depth, unsigned width, unsigned height) {
	XImage *ximage;

	ximage = XCreateImage(display, visual, depth, ZPixmap, 0, NULL, width, height, 32, 0);
	ximage->data = xsg_malloc(ximage->bytes_per_line * ximage->height);

	if (am_big_endian())
		ximage->byte_order = MSBFirst;
	else
		ximage->byte_order = LSBFirst;

	return ximage;
}

/******************************************************************************/

void xsg_xrender_render(Window window, Visual *visual, Pixmap mask, unsigned xshape, uint32_t *data, int xoffset, int yoffset, unsigned width, unsigned height) {
	XImage *ximage, *alpha_ximage;
	Pixmap pixmap, alpha_pixmap;
	Picture picture, alpha_picture, root_picture;
	XRenderPictFormat *picture_format;
	XRenderPictureAttributes root_picture_attrs;
	unsigned i;
	XGCValues gcv;
	GC gc;

	gcv.graphics_exposures = False;

	xsg_debug("XRender xoffset=%d, yoffset=%d, width=%d, height=%d", xoffset, yoffset, width, height);

	if (xshape) {
		XImage *mask_ximage;
		unsigned x, y;
		uint32_t *d;
		GC mask_gc;

		xsg_debug("Render xshape mask");

		mask_ximage = create_ximage(visual, 1, width, height);

		memset(mask_ximage->data, 0, mask_ximage->bytes_per_line * mask_ximage->height);

		d = data;

		for (y = 0; y < height; y++) {
			uint8_t *m = (uint8_t *) mask_ximage->data + y * mask_ximage->bytes_per_line;

			for (x = 0; x < width; x++) {
				if ((*d >> 24) >= xshape) {
					if (am_big_endian())
						*m |= (1 << (0x7 - (x & 0x7)));
					else
						*m |= (1 << (x & 0x7));
				}
				if ((x & 0x7) == 0x7)
					m++;
				d++;
			}
		}

		mask_gc = XCreateGC(display, mask, GCGraphicsExposures, &gcv);

		XPutImage(display, mask, mask_gc, mask_ximage, 0, 0, xoffset, yoffset, width, height);

		XFreeGC(display, mask_gc);

		XDestroyImage(mask_ximage);
	}

	ximage = create_ximage(visual, 32, width, height);
	alpha_ximage = create_ximage(visual, 32, width, height);

	for (i = 0; i < (width * height); i++) {
		//A_VAL((uint32_t *) ximage->data + i) = 0xff;
		//R_VAL((uint32_t *) ximage->data + i) = R_VAL(data + i);
		//G_VAL((uint32_t *) ximage->data + i) = G_VAL(data + i);
		//B_VAL((uint32_t *) ximage->data + i) = B_VAL(data + i);
		((uint32_t *)ximage->data)[i] = data[i];

		//A_VAL((uint32_t *) alpha_ximage->data + i) = A_VAL(data + i);
		//R_VAL((uint32_t *) alpha_ximage->data + i) = 0x00;
		//G_VAL((uint32_t *) alpha_ximage->data + i) = 0x00;
		//B_VAL((uint32_t *) alpha_ximage->data + i) = 0x00;
		((uint32_t *)alpha_ximage->data)[i] = data[i];
	}

	pixmap = XCreatePixmap(display, window, width, height, 32);
	alpha_pixmap = XCreatePixmap(display, window, width, height, 32);

	gc = XCreateGC(display, window, GCGraphicsExposures, &gcv);

	XPutImage(display, pixmap, gc, ximage, 0, 0, 0, 0, width, height);
	XPutImage(display, alpha_pixmap, gc, alpha_ximage, 0, 0, 0, 0, width, height);

	XFreeGC(display, gc);

	XDestroyImage(ximage);
	XDestroyImage(alpha_ximage);

	picture_format = XRenderFindStandardFormat(display, 0);

	root_picture_attrs.subwindow_mode = IncludeInferiors;

	root_picture = XRenderCreatePicture(display, window, XRenderFindVisualFormat(display, visual),
			(1 << 8), &root_picture_attrs);

	picture = XRenderCreatePicture(display, pixmap, picture_format, 0, 0);
	alpha_picture = XRenderCreatePicture(display, alpha_pixmap, picture_format, 0, 0);

	XRenderComposite(display, 1, picture, alpha_picture, root_picture, 0, 0, 0, 0,
			xoffset, yoffset, width, height);

	XRenderFreePicture(display, picture);
	XRenderFreePicture(display, alpha_picture);

	XFreePixmap(display, pixmap);
	XFreePixmap(display, alpha_pixmap);

	XSync(display, False);

	xsg_debug("XRender finished");
}


