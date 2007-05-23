/* window.c
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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <Imlib2.h>

#ifdef ENABLE_XRENDER
# include <X11/extensions/Xrender.h>
# include <X11/extensions/Xcomposite.h>
#endif

#include "window.h"
#include "widgets.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef struct {
	char *name;
	char *class;
	char *resource;
	char *geometry;
	int flags;
	int win_gravity;
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	bool sticky;
	bool skip_taskbar;
	bool skip_pager;
	int layer;
	bool decorations;
	bool override_redirect;
	Imlib_Color background;
	bool copy_from_parent;
	bool copy_from_root;
	unsigned int cache_size;
	unsigned int font_cache_size;
	bool xshape;
	bool argb_visual;
	Display *display;
	Visual *visual;
	int depth;
	int screen;
	Window id;
	Imlib_Updates updates;
	Pixmap mask;
	bool show;
	uint64_t show_update;
	uint32_t show_var_id;
} window_t;

static window_t window = {
	name: "xsysguard",
	class: "xsysguard",
	resource: "xsysguard",
	geometry: "64x64+128+128",
	flags: 0,
	win_gravity: NorthWestGravity,
	xoffset: 128,
	yoffset: 128,
	width: 64,
	height: 64,
	sticky: FALSE,
	skip_taskbar: FALSE,
	skip_pager: FALSE,
	layer: 0,
	decorations: TRUE,
	override_redirect: FALSE,
	background: { 0 },
	copy_from_parent: FALSE,
	copy_from_root: FALSE,
	cache_size: 4194304,
	font_cache_size: 2097152,
	xshape: FALSE,
	argb_visual: FALSE,
	display: NULL,
	visual: NULL,
	depth: 0,
	screen: 0,
	id: 0,
	updates: 0,
	mask: 0,
	show: FALSE,
	show_update: 0,
	show_var_id: 0xffffffff,
};

/******************************************************************************
 *
 * parse configuration
 *
 ******************************************************************************/

void xsg_window_parse_name() {
	window.name = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_class() {
	window.class = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_resource() {
	window.resource = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_geometry() {
	window.geometry = xsg_conf_read_string();
	xsg_conf_read_newline();
	window.flags = XParseGeometry(window.geometry, &window.xoffset, &window.yoffset, &window.width, &window.height);
}

void xsg_window_parse_sticky() {
	window.sticky = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_skip_taskbar() {
	window.skip_taskbar = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_skip_pager() {
	window.skip_pager = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_layer() {
	if (xsg_conf_find_command("Above"))
		window.layer = 1;
	else if (xsg_conf_find_command("Normal"))
		window.layer = 0;
	else if (xsg_conf_find_command("Below"))
		window.layer = -1;
	else
		xsg_conf_error("Above, Normal or Below");
	xsg_conf_read_newline();
}

void xsg_window_parse_decorations() {
	window.decorations = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_override_redirect() {
	window.override_redirect = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_background() {
	if (xsg_conf_find_command("CopyFromParent"))
		window.copy_from_parent = TRUE;
	else if (xsg_conf_find_command("CopyFromRoot"))
		window.copy_from_root = TRUE;
	else if (xsg_conf_find_command("Color"))
		window.background = xsg_imlib_uint2color(xsg_conf_read_color());
	else
		xsg_conf_error("CopyFromParent, CopyFromRoot or Color");
	xsg_conf_read_newline();
}

void xsg_window_parse_cache_size() {
	window.cache_size = xsg_conf_read_uint();
	xsg_conf_read_newline();
}

void xsg_window_parse_font_cache_size() {
	window.font_cache_size = xsg_conf_read_uint();
	xsg_conf_read_newline();
}

void xsg_window_parse_xshape() {
	window.xshape = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_argb_visual() {
	window.argb_visual = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_show() {
	window.show_update = xsg_conf_read_uint();
	window.show_var_id = xsg_var_parse(0xffffffff, window.show_update);
}

/******************************************************************************
 *
 * set xatom
 *
 ******************************************************************************/

void set_xatom(const char *type, const char *property) {
	XEvent xev;
	Atom type_atom;
	Atom property_atom;

	type_atom = XInternAtom(window.display, type, FALSE);
	property_atom = XInternAtom(window.display, property, FALSE);

	xsg_message("Setting Xatom \"%s\": \"%s\"", type, property);

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = window.id;
	xev.xclient.message_type = type_atom;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = property_atom;
	xev.xclient.data.l[2] = 0;

	XSendEvent(window.display, XRootWindow(window.display, window.screen), FALSE, SubstructureNotifyMask, &xev);
}

/******************************************************************************
 *
 * XRender
 *
 ******************************************************************************/

#ifndef ENABLE_XRENDER

static Visual *find_argb_visual() {
	xsg_error("Compiled without XRender support")
	return NULL;
}

static void xrender_check() {
	xsg_error("Compiled without XRender support")
}

static void xrender_init() {
	xsg_error("Compiled without XRender support")
}

static void xrender(int xoffset, int yoffset) {
	xsg_error("Compiled without XRender support")
}

#else

static Visual *find_argb_visual() {
	XVisualInfo *xvi;
	XVisualInfo template;
	int nvi, i;
	XRenderPictFormat *format;
	Visual *visual = NULL;

	template.screen = window.screen;
	template.depth = 32;
	template.class = TrueColor;

	xvi = XGetVisualInfo(window.display, VisualScreenMask | VisualDepthMask | VisualClassMask, &template, &nvi);

	if (xvi == NULL)
		xsg_error("Cannot find an argb visual.");

	for (i = 0; i < nvi; i++) {
		format = XRenderFindVisualFormat(window.display, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask) {
			visual = xvi[i].visual;
			break;
		}
	}

	XFree(xvi);

	if (visual == NULL)
		xsg_error("Cannot find an argb visual.");

	return visual;
}

static void xrender_check() {
	int render_event;
	int render_error;
	int composite_opcode;
	int composite_event;
	int composite_error;
	int composite_major;
	int composite_minor;

	if (!XRenderQueryExtension(window.display, &render_event, &render_error))
		xsg_error("No render extension found.");

	if (!XQueryExtension(window.display, COMPOSITE_NAME, &composite_opcode, &composite_event, &composite_error))
		xsg_error("No composite extension found.");

	XCompositeQueryVersion(window.display, &composite_major, &composite_minor);

	xsg_message("Composite extension found: %d.%d", composite_major, composite_minor);
}

static void xrender_init() {
	XCompositeRedirectSubwindows(window.display, window.id, CompositeRedirectAutomatic);
}

static void xrender_pixmaps(Pixmap *colors, Pixmap *alpha, int xoffset, int yoffset) {
	DATA32 *data;
	DATA32 *colors_data;
	DATA32 *alpha_data;
	DATA8 *mask_data = NULL;
	XImage *colors_image;
	XImage *alpha_image;
	XImage *mask_image = NULL;
	unsigned int width;
	unsigned int height;
	int i;
	GC gc, mask_gc;

	width = imlib_image_get_width();
	height = imlib_image_get_height();
	data = imlib_image_get_data_for_reading_only();

	colors_data = xsg_new(DATA32, width * height);
	alpha_data = xsg_new(DATA32, width * height);

	colors_image = XCreateImage(window.display, window.visual, 32, ZPixmap, 0, (char *) colors_data,
			width, height, 32, width * 4);

	alpha_image = XCreateImage(window.display, window.visual, 32, ZPixmap, 0, (char *) alpha_data,
			width, height, 32, width * 4);

	if (window.xshape) {

		mask_data = xsg_new(DATA8, width * height);

		mask_image = xsg_new(XImage, 1);
		mask_image->width = width;
		mask_image->height = height;
		mask_image->xoffset = 0;
		mask_image->format = ZPixmap;
		mask_image->data = (char *) mask_data;
		mask_image->byte_order = LSBFirst;
		mask_image->bitmap_unit = 32;
		mask_image->bitmap_bit_order = LSBFirst;
		mask_image->bitmap_pad = 8;
		mask_image->depth = 1;
		mask_image->bytes_per_line = 0;
		mask_image->bits_per_pixel = 8;
		mask_image->red_mask = 0xff;
		mask_image->green_mask = 0xff;
		mask_image->blue_mask = 0xff;
		XInitImage(mask_image);
	}

	for (i = 0; i < (width * height); i++) {

		A_VAL(colors_data + i) = 0xff;
		R_VAL(colors_data + i) = R_VAL(data + i);
		G_VAL(colors_data + i) = G_VAL(data + i);
		B_VAL(colors_data + i) = B_VAL(data + i);

		A_VAL(alpha_data + i) = A_VAL(data + i);
		R_VAL(alpha_data + i) = 0;
		G_VAL(alpha_data + i) = 0;
		B_VAL(alpha_data + i) = 0;

		if (window.xshape)
			mask_data[i] = (A_VAL(data + i) == 0) ? 0 : 1;
	}

	*colors = XCreatePixmap(window.display, window.id, width, height, 32);

	*alpha = XCreatePixmap(window.display, window.id, width, height, 32);

	gc = XCreateGC(window.display, window.id, 0, 0);

	XPutImage(window.display, *colors, gc, colors_image, 0, 0, 0, 0, width, height);

	XPutImage(window.display, *alpha, gc, alpha_image, 0, 0, 0, 0, width, height);

	XDestroyImage(colors_image);
	XDestroyImage(alpha_image);

	XFreeGC(window.display, gc);

	if (window.xshape) {
		mask_gc = XCreateGC(window.display, window.mask, 0, 0);
		XPutImage(window.display, window.mask, mask_gc, mask_image, 0, 0,
				xoffset, yoffset, window.width, window.height);
		xsg_free(mask_data);
		xsg_free(mask_image);
		XFreeGC(window.display, mask_gc);
	}
}

static void xrender(int xoffset, int yoffset) {
	Pixmap colors;
	Pixmap alpha;
	Picture root_picture;
	Picture colors_picture;
	Picture alpha_picture;
	XRenderPictFormat *pict_format;
	XRenderPictureAttributes root_pict_attrs;
	unsigned int width;
	unsigned int height;

	width = imlib_image_get_width();
	height = imlib_image_get_height();

	xrender_pixmaps(&colors, &alpha, xoffset, yoffset);

	pict_format = XRenderFindStandardFormat(window.display,	PictStandardARGB32);

	root_pict_attrs.subwindow_mode = IncludeInferiors;

	root_picture = XRenderCreatePicture(window.display, window.id, XRenderFindVisualFormat(window.display, window.visual),
			CPSubwindowMode, &root_pict_attrs);

	colors_picture = XRenderCreatePicture(window.display, colors, pict_format, 0, 0);

	alpha_picture = XRenderCreatePicture(window.display, alpha, pict_format, 0, 0);

	XRenderComposite(window.display, PictOpSrc, colors_picture, alpha_picture, root_picture, 0, 0, 0, 0,
			xoffset, yoffset, width, height);

	XRenderFreePicture(window.display, colors_picture);
	XRenderFreePicture(window.display, alpha_picture);

	XFreePixmap(window.display, colors);
	XFreePixmap(window.display, alpha);
}

#endif /* ENABLE_XRENDER */

/******************************************************************************
 *
 * render onto window
 *
 ******************************************************************************/

void xsg_window_render(void) {
	int up_x, up_y, up_w, up_h;
	Imlib_Updates update;
	Imlib_Image buffer;

	window.updates = imlib_updates_merge_for_rendering(window.updates, window.width, window.height);

	for (update = window.updates; update; update = imlib_updates_get_next(update)) {

		imlib_updates_get_coordinates(update, &up_x, &up_y, &up_w, &up_h);

		xsg_message("Render (x=%d, y=%d, width=%d, height=%d)", up_x, up_y, up_w, up_h);

		buffer = imlib_create_image(up_w, up_h);
		imlib_context_set_image(buffer);
		imlib_image_set_has_alpha(1);
		imlib_image_clear_color(window.background.red, window.background.green,
				window.background.blue, window.background.alpha);
		/* TODO grab_root / parent */

		xsg_widgets_render(buffer, up_x, up_y, up_w, up_h);

		imlib_context_set_image(buffer);
		imlib_context_set_blend(0);

		if (window.argb_visual)
			xrender(up_x, up_y);
		else
			T(imlib_render_image_on_drawable(up_x, up_y));

		imlib_context_set_image(buffer);
		imlib_context_set_blend(1);
		imlib_free_image();
	}

	if (window.updates)
		imlib_updates_free(window.updates);

	window.updates = 0;
}

/******************************************************************************/

void xsg_window_render_xshape(void) {
	if (window.xshape)
		XShapeCombineMask(window.display, window.id, ShapeBounding, 0, 0, window.mask, ShapeSet);
}

/******************************************************************************/

static void update_show(void) {
	bool show;

	show = window.show;

	if (window.show_update != 0)
		window.show = (xsg_var_get_num(window.show_var_id) == 0.0) ? FALSE : TRUE;
	else
		window.show = TRUE;

	if (window.show != show) {
		if (window.show) {
			xsg_debug("XMapWindow");

			XMapWindow(window.display, window.id);

			if (window.sticky)
				set_xatom("_NET_WM_STATE", "_NET_WM_STATE_STICKY");

			if (window.skip_taskbar)
				set_xatom("_NET_WM_STATE", "_NET_WM_STATE_SKIP_TASKBAR");

			if (window.skip_pager)
				set_xatom("_NET_WM_STATE", "_NET_WM_STATE_SKIP_PAGER");

			if (window.layer > 0)
				set_xatom("_NET_WM_STATE", "_NET_WM_STATE_ABOVE");
			else if (window.layer < 0)
				set_xatom("_NET_WM_STATE", "_NET_WM_STATE_BELOW");

		} else {
			xsg_debug("XUnmapWindow");

			XUnmapWindow(window.display, window.id);
		}
	}
}

/******************************************************************************
 *
 * async events
 *
 ******************************************************************************/

void xsg_window_update_widget(uint32_t widget_id, uint32_t var_id) {
	if (widget_id == 0xffffffff) {
		update_show();
		xsg_window_update_append_rect(0, 0, window.width, window.height);
		xsg_window_render();
		xsg_window_render_xshape();
	} else {
		xsg_widgets_update(widget_id, var_id);
	}
}

/******************************************************************************
 *
 * update func
 *
 ******************************************************************************/

static void update(uint64_t count) {
	if (window.show_update && (count % window.show_update) == 0)
		update_show();
}

/******************************************************************************/

void xsg_window_update_append_rect(int xoffset, int yoffset, int width, int height) {
	window.updates = imlib_update_append_rect(window.updates, xoffset, yoffset, width, height);
}

/******************************************************************************
 *
 * hide window decorations
 *
 ******************************************************************************/

typedef struct {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long inputMode;
	unsigned long status;
} MotifWmHints;

void hide_decorations() {
	Atom hints_atom;
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	MotifWmHints *hints;
	MotifWmHints **hints_pointer = &hints;
	MotifWmHints new_hints = { 0 };

	hints_atom = XInternAtom(window.display, "_MOTIF_WM_HINTS", FALSE);

	XGetWindowProperty(window.display, window.id, hints_atom, 0, sizeof(MotifWmHints)/sizeof(long), False, AnyPropertyType,
			&type, &format, &nitems, &bytes_after, (unsigned char **) hints_pointer);

	if (type == None)
		hints = &new_hints;

	hints->flags |= (1L << 1);
	hints->decorations = 0;

	XChangeProperty(window.display, window.id, hints_atom, hints_atom, 32,
			PropModeReplace, (unsigned char *) hints, sizeof(MotifWmHints)/sizeof(long));

	if (hints != &new_hints)
		XFree(hints);
}

/******************************************************************************
 *
 * size hints
 *
 ******************************************************************************/

static void set_size_hints() {
	XSizeHints *size_hints;

	size_hints = XAllocSizeHints();

	size_hints->flags = PMinSize | PMaxSize | PSize | USPosition | PWinGravity;

	size_hints->min_width = window.width;
	size_hints->max_width = window.width;
	size_hints->min_height = window.height;
	size_hints->max_height = window.height;
	size_hints->height = window.height;
	size_hints->width = window.width;
	size_hints->x = window.xoffset;
	size_hints->y = window.yoffset;
	size_hints->win_gravity = window.win_gravity;

	XSetWMNormalHints(window.display, window.id, size_hints);

	XFree(size_hints);
}

/******************************************************************************
 *
 * class hints
 *
 ******************************************************************************/

static void set_class_hints() {
	XClassHint *class_hint;

	class_hint = XAllocClassHint();

	class_hint->res_name = window.resource;
	class_hint->res_class = window.class;

	XSetClassHint(window.display, window.id, class_hint);

	XFree(class_hint);
}

/******************************************************************************
 *
 * X11 event handler
 *
 ******************************************************************************/

static void handle_xevents(void *arg, xsg_main_poll_t events) {
	XEvent event;

	while (XPending(window.display)) {
		XNextEvent(window.display, &event);
		if (event.type == Expose) {
			xsg_message("XExpose (x=%d, y=%d, width=%d, height=%d)",
					event.xexpose.x, event.xexpose.y,
					event.xexpose.width, event.xexpose.height);
			window.updates = imlib_update_append_rect(window.updates,
					event.xexpose.x, event.xexpose.y,
					event.xexpose.width, event.xexpose.height);
		} else {
			xsg_message("XEvent (type=%d)", event.type);
		}
	}

	if (window.updates)
		xsg_window_render();
}

/******************************************************************************
 *
 * initialize window
 *
 ******************************************************************************/

void xsg_window_init() {
	Colormap colormap;
	XSetWindowAttributes attrs;
	unsigned long valuemask;

	if ((window.display = XOpenDisplay(NULL)) == NULL)
		xsg_error("Cannot open display");

	window.screen = XDefaultScreen(window.display);

	if (window.flags & XNegative) {
		window.xoffset += DisplayWidth(window.display, window.screen) - window.width;
		window.win_gravity = NorthEastGravity;
	}

	if (window.flags & YNegative) {
		window.yoffset += DisplayHeight(window.display, window.screen) - window.height;
		window.win_gravity = (window.win_gravity == NorthEastGravity ? SouthEastGravity : SouthWestGravity);
	}

	if (window.argb_visual) {
		xrender_check();
		window.visual = find_argb_visual();
		window.depth = 32;
	} else {
		window.visual = imlib_get_best_visual(window.display, window.screen, &window.depth);
	}

	if (window.xshape) {
		int event_base, error_base;

		if (!XShapeQueryExtension(window.display, &event_base, &error_base))
			xsg_error("No xshape extension found");
	}

	colormap = XCreateColormap(window.display, RootWindow(window.display, window.screen), window.visual, AllocNone);

	attrs.background_pixel = 0;
	attrs.colormap = colormap;
	valuemask = CWBackPixel | CWColormap;

	if (window.override_redirect) {
		attrs.override_redirect = 1;
		valuemask |= CWOverrideRedirect;
	}

	window.id = XCreateWindow(window.display, XRootWindow(window.display, window.screen),
			window.xoffset, window.yoffset, window.width, window.height, 0,
			window.depth, InputOutput, window.visual, valuemask, &attrs);

	set_size_hints();
	set_class_hints();

	XStoreName(window.display, window.id, window.name);

	XSelectInput(window.display, window.id, ExposureMask);

	if (window.argb_visual)
		xrender_init();

	if (!window.decorations)
		hide_decorations();

	imlib_context_set_blend(1);
	imlib_context_set_dither(0);
	imlib_context_set_dither_mask(0);
	imlib_context_set_anti_alias(1);
	imlib_context_set_display(window.display);
	imlib_context_set_visual(window.visual);
	imlib_context_set_colormap(colormap);
	imlib_context_set_drawable(window.id);
	imlib_set_cache_size(window.cache_size);
	imlib_set_font_cache_size(window.font_cache_size);

	if (window.xshape) {
		window.mask = XCreatePixmap(window.display, window.id, window.width, window.height, 1);
		imlib_context_set_mask(window.mask);
	}

	window.updates = imlib_update_append_rect(window.updates, 0, 0, window.width, window.height);

	xsg_main_add_update_func(update);

	xsg_widgets_init();

	xsg_main_add_poll_func(ConnectionNumber(window.display), handle_xevents, NULL,
			XSG_MAIN_POLL_READ | XSG_MAIN_POLL_EXCEPT);

	update_show();
}


