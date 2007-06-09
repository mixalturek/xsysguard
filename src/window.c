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
#include <signal.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <Imlib2.h>

#ifdef ENABLE_XRENDER
# include <X11/extensions/Xrender.h>
# include <X11/extensions/Xcomposite.h>
#endif

#include "window.h"
#include "update.h"
#include "widgets.h"
#include "imlib.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

struct _xsg_window_t {
	char *config;
	char *name;
	char *class;
	char *resource;
	char *geometry;
	int flags;
	int gravity;

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

	Imlib_Color background_color;
	Imlib_Image background_image;
	bool background_image_update;

	bool copy_from_parent;
	Window copy_from_parent_window;

	bool copy_from_root;
	int copy_from_root_xoffset;
	int copy_from_root_yoffset;

	unsigned xshape;
	bool argb_visual;
	Visual *visual;
	Colormap colormap;
	int depth;

	Window window;

	Pixmap pixmap;
	Pixmap mask;

	Imlib_Updates xexpose_updates;
	xsg_list_t *updates;

	bool visible;
	uint64_t visible_update;
	xsg_var_t *visible_var;

	xsg_list_t *widget_list;
};

/******************************************************************************/

static xsg_list_t *window_list = NULL;

static Display *display = NULL;
static int screen = 0;
static xsg_main_poll_t poll = { 0 };

/******************************************************************************
 *
 * window_new
 *
 ******************************************************************************/

xsg_window_t *xsg_window_new(char *config_name) {
	xsg_window_t *window;

	window = xsg_new(xsg_window_t, 1);

	window->config = config_name;

	window->name = "xsysguard";
	window->class = "xsysguard";
	window->resource = "xsysguard";

	window->geometry = "64x64+128+128";
	window->flags = 0;
	window->gravity = NorthWestGravity;

	window->xoffset = 128;
	window->yoffset = 128;
	window->width = 64;
	window->height = 64;

	window->sticky = FALSE;
	window->skip_taskbar = FALSE;
	window->skip_pager = FALSE;
	window->layer = 0;
	window->decorations = TRUE;
	window->override_redirect = FALSE;

	window->background_color.red = 0;
	window->background_color.green = 0;
	window->background_color.blue = 0;
	window->background_color.alpha = 0xff;

	window->background_image = NULL;
	window->background_image_update = 0;

	window->copy_from_parent = FALSE;
	window->copy_from_parent_window = None;

	window->copy_from_root = FALSE;
	window->copy_from_root_xoffset = 0;
	window->copy_from_root_yoffset = 0;

	window->xshape = 0;
	window->argb_visual = FALSE;
	window->visual = NULL;
	window->colormap = 0;
	window->depth = 0;

	window->window = None;

	window->pixmap = 0;
	window->mask = 0;

	window->xexpose_updates = 0;
	window->updates = NULL;

	window->visible = FALSE;
	window->visible_update = 0;
	window->visible_var = NULL;

	window->widget_list = NULL;

	window_list = xsg_list_append(window_list, window);

	return window;
}

/******************************************************************************/

void xsg_window_add_widget(xsg_window_t *window, xsg_widget_t *widget) {
	window->widget_list = xsg_list_append(window->widget_list, widget);
}

char *xsg_window_get_config_name(xsg_window_t *window) {
	return window->config;
}

/******************************************************************************
 *
 * parse configuration
 *
 ******************************************************************************/

void xsg_window_parse_name(xsg_window_t *window) {
	window->name = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_class(xsg_window_t *window) {
	window->class = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_resource(xsg_window_t *window) {
	window->resource = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_window_parse_geometry(xsg_window_t *window) {
	window->geometry = xsg_conf_read_string();
	xsg_conf_read_newline();
	window->flags = XParseGeometry(window->geometry, &window->xoffset, &window->yoffset,
			&window->width, &window->height);
}

void xsg_window_parse_sticky(xsg_window_t *window) {
	window->sticky = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_skip_taskbar(xsg_window_t *window) {
	window->skip_taskbar = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_skip_pager(xsg_window_t *window) {
	window->skip_pager = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_layer(xsg_window_t *window) {
	if (xsg_conf_find_command("Above"))
		window->layer = 1;
	else if (xsg_conf_find_command("Normal"))
		window->layer = 0;
	else if (xsg_conf_find_command("Below"))
		window->layer = -1;
	else
		xsg_conf_error("Above, Normal or Below expected");
	xsg_conf_read_newline();
}

void xsg_window_parse_decorations(xsg_window_t *window) {
	window->decorations = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_override_redirect(xsg_window_t * window) {
	window->override_redirect = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_background(xsg_window_t * window) {
	if (xsg_conf_find_command("CopyFromParent")) {
		window->copy_from_parent = TRUE;
		window->background_image_update = xsg_conf_read_uint();
	} else if (xsg_conf_find_command("CopyFromRoot")) {
		window->copy_from_root = TRUE;
		window->background_image_update = xsg_conf_read_uint();
	} else if (xsg_conf_find_command("Color")) {
		window->background_color = xsg_imlib_uint2color(xsg_conf_read_color());
	} else {
		xsg_conf_error("CopyFromParent, CopyFromRoot or Color expected");
	}
	xsg_conf_read_newline();
}

void xsg_window_parse_xshape(xsg_window_t *window) {
	uint64_t val = xsg_conf_read_uint();

	window->xshape = MIN(val, 255);
	xsg_conf_read_newline();
}

void xsg_window_parse_argb_visual(xsg_window_t *window) {
	window->argb_visual = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_window_parse_visible(xsg_window_t *window) {
	window->visible_update = xsg_conf_read_uint();
	window->visible_var = xsg_var_parse(window, NULL, window->visible_update);
}

/******************************************************************************
 *
 * set xatom
 *
 ******************************************************************************/

static void set_xatom(xsg_window_t *window, const char *type, const char *property) {
	XEvent xev;
	Atom type_atom;
	Atom property_atom;

	type_atom = XInternAtom(display, type, FALSE);
	property_atom = XInternAtom(display, property, FALSE);

	xsg_message("%s: Setting Xatom %s: %s", window->config, type, property);

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = window->window;
	xev.xclient.message_type = type_atom;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = property_atom;
	xev.xclient.data.l[2] = 0;

	XSendEvent(display, XRootWindow(display, screen), FALSE, SubstructureNotifyMask, &xev);
}

/******************************************************************************
 *
 * grab background
 *
 ******************************************************************************/

static void grab_background(xsg_window_t *window, Window src_window) {
	Imlib_Image background;
	int status;
	int x, y;
	XEvent event;
	Window src;
	XSetWindowAttributes attrs;

	if (unlikely(src_window == None))
		return;

	status = XTranslateCoordinates(display, window->window, src_window, 0, 0, &x, &y, &src);

	if (unlikely(!status))
		return;

	attrs.background_pixmap = ParentRelative;
	attrs.backing_store = Always;
	attrs.override_redirect = True;
	attrs.event_mask = ExposureMask;

	src = XCreateWindow(display, src_window, x, y, window->width, window->height, 0,
			window->depth, CopyFromParent, window->visual,
			CWBackPixmap | CWBackingStore | CWOverrideRedirect | CWEventMask,
			&attrs);

	if (unlikely(!src))
		return;

	imlib_context_set_drawable(src);
	imlib_context_set_visual(window->visual);
	imlib_context_set_colormap(window->colormap);

	XGrabServer(display);
	XMapRaised(display, src);
	XSync(display, False);

	do {
		XWindowEvent(display, src, ExposureMask, &event);
	} while (event.type != Expose);

	background = imlib_create_image_from_drawable(0, 0, 0, window->width, window->height, 0);

	XUngrabServer(display);
	XDestroyWindow(display, src);

	if (background) {
		if (window->background_image) {
			imlib_context_set_image(window->background_image);
			imlib_free_image();
		}
		window->background_image = background;
		xsg_window_update_append_rect(window, 0, 0, window->width, window->height);
	}
}

static bool check_root_background(xsg_window_t *window) {
	static Pixmap old_pixmap = None;
	static Atom id = None;
	Pixmap pixmap = None;
	Window root;
	int status;
	Atom act_type;
	int act_format;
	unsigned long nitems, bytes_after;
	unsigned char *prop = NULL;
	int x, y;
	Window src;

	root = RootWindow(display, screen);

	status = XTranslateCoordinates(display, window->window, root, 0, 0, &x, &y, &src);

	if (status != None) {
		if ((x != window->copy_from_root_xoffset) || (y != window->copy_from_root_yoffset)) {
			window->copy_from_root_xoffset = x;
			window->copy_from_root_yoffset = y;
			return TRUE;
		}
	}

	if (id == None)
		id = XInternAtom(display, "_XROOTMAP_ID", True);

	if (id == None)
		return FALSE;

	status = XGetWindowProperty(display, root, id, 0, 1, False, XA_PIXMAP, &act_type, &act_format,
			&nitems, &bytes_after, &prop);

	if (status == Success && prop != NULL) {
		pixmap = *((Pixmap *)prop);
		XFree(prop);
		if(old_pixmap != pixmap) {
			old_pixmap = pixmap;
			return TRUE;
		}
	}

	return FALSE;
}

static void grab_root_background(xsg_window_t *window) {
	grab_background(window, RootWindow(display, screen));
}

static bool check_parent_background(xsg_window_t *window) {
	Status status;
	unsigned int nchildren;
	Window *children = NULL;
	Window root;
	Window parent = None;

	status = XQueryTree(display, window->window, &root, &parent, &children, &nchildren);

	if (parent == None)
		return FALSE;

	if (children != NULL)
		XFree(children);

	if (parent != window->copy_from_parent_window) {
		window->copy_from_parent_window = parent;
		xsg_message("%s: New parent window is: 0x%lx", window->config, (unsigned long) parent);
		return TRUE;
	} else {
		return FALSE;
	}
}

static void grab_parent_background(xsg_window_t *window) {
	check_parent_background(window); // update window->copy_from_parent_window
	grab_background(window, window->copy_from_parent_window);
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

	template.screen = screen;
	template.depth = 32;
	template.class = TrueColor;

	xvi = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask | VisualClassMask, &template, &nvi);

	if (xvi == NULL)
		xsg_error("Cannot find an argb visual.");

	for (i = 0; i < nvi; i++) {
		format = XRenderFindVisualFormat(display, xvi[i].visual);
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
	static bool checked = FALSE;
	int render_event;
	int render_error;
	int composite_opcode;
	int composite_event;
	int composite_error;
	int composite_major;
	int composite_minor;

	if (checked)
		return;

	checked = TRUE;

	if (!XRenderQueryExtension(display, &render_event, &render_error))
		xsg_error("No render extension found.");

	if (!XQueryExtension(display, COMPOSITE_NAME, &composite_opcode, &composite_event, &composite_error))
		xsg_error("No composite extension found.");

	XCompositeQueryVersion(display, &composite_major, &composite_minor);

	xsg_message("Composite extension found: %d.%d", composite_major, composite_minor);
}

static void xrender_init(xsg_window_t *window) {
	XCompositeRedirectSubwindows(display, window->window, CompositeRedirectAutomatic);
}

static void xrender_pixmaps(xsg_window_t *window, Pixmap *colors, Pixmap *alpha, int xoffset, int yoffset) {
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

	colors_image = XCreateImage(display, window->visual, 32, ZPixmap, 0, (char *) colors_data,
			width, height, 32, width * 4);

	alpha_image = XCreateImage(display, window->visual, 32, ZPixmap, 0, (char *) alpha_data,
			width, height, 32, width * 4);

	if (window->xshape) {

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

		if (window->xshape)
			mask_data[i] = (A_VAL(data + i) == 0) ? 0 : 1;
	}

	*colors = XCreatePixmap(display, window->window, width, height, 32);

	*alpha = XCreatePixmap(display, window->window, width, height, 32);

	gc = XCreateGC(display, window->window, 0, 0);

	XPutImage(display, *colors, gc, colors_image, 0, 0, 0, 0, width, height);

	XPutImage(display, *alpha, gc, alpha_image, 0, 0, 0, 0, width, height);

	XDestroyImage(colors_image);
	XDestroyImage(alpha_image);

	XFreeGC(display, gc);

	if (window->xshape) {
		mask_gc = XCreateGC(display, window->mask, 0, 0);
		XPutImage(display, window->mask, mask_gc, mask_image, 0, 0,
				xoffset, yoffset, window->width, window->height);
		xsg_free(mask_data);
		xsg_free(mask_image);
		XFreeGC(display, mask_gc);
	}
}

static void xrender(xsg_window_t *window, int xoffset, int yoffset) {
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

	xrender_pixmaps(window, &colors, &alpha, xoffset, yoffset);

	pict_format = XRenderFindStandardFormat(display, PictStandardARGB32);

	root_pict_attrs.subwindow_mode = IncludeInferiors;

	root_picture = XRenderCreatePicture(display, window->window, XRenderFindVisualFormat(display, window->visual),
			CPSubwindowMode, &root_pict_attrs);

	colors_picture = XRenderCreatePicture(display, colors, pict_format, 0, 0);

	alpha_picture = XRenderCreatePicture(display, alpha, pict_format, 0, 0);

	XRenderComposite(display, PictOpSrc, colors_picture, alpha_picture, root_picture, 0, 0, 0, 0,
			xoffset, yoffset, width, height);

	XRenderFreePicture(display, colors_picture);
	XRenderFreePicture(display, alpha_picture);

	XFreePixmap(display, colors);
	XFreePixmap(display, alpha);

	if (window->xshape)
		XShapeCombineMask(display, window->window, ShapeBounding, 0, 0, window->mask, ShapeSet);
}

#endif /* ENABLE_XRENDER */

/******************************************************************************
 *
 * render window
 *
 ******************************************************************************/

static void render(xsg_window_t *window) {
	Imlib_Image buffer;
	xsg_list_t *update;

	if (!window->visible)
		return;

	xsg_debug("%s: Render...", window->config);

	imlib_context_set_visual(window->visual);
	imlib_context_set_colormap(window->colormap);

	if (window->xshape > 0 && !window->argb_visual) {
		imlib_context_set_drawable(window->pixmap);
		imlib_context_set_mask(window->mask);
		imlib_context_set_mask_alpha_threshold(window->xshape);
	} else {
		imlib_context_set_drawable(window->window);
		imlib_context_set_mask(0);

		if (window->xexpose_updates) {
			Imlib_Updates xexpose_update;

			window->xexpose_updates = imlib_updates_merge_for_rendering(window->xexpose_updates, window->width, window->height);

			for (xexpose_update = window->xexpose_updates; xexpose_update; xexpose_update = imlib_updates_get_next(xexpose_update)) {
				int x, y, w, h;

				imlib_updates_get_coordinates(xexpose_update, &x, &y, &w, &h);
				window->updates = xsg_update_append_rect(window->updates, x, y, w, h);
			}

			imlib_updates_free(window->xexpose_updates);
			window->xexpose_updates = 0;
		}
	}

	if (window->updates == NULL)
		return;

	for (update = window->updates; update; update = update->next) {
		int up_x, up_y, up_w, up_h;
		xsg_list_t *l;

		xsg_update_get_coordinates(update, &up_x, &up_y, &up_w, &up_h);

		xsg_debug("%s: Render x=%d, y=%d, width=%d, height=%d", window->config, up_x, up_y, up_w, up_h);

		if (window->background_image) {
			imlib_context_set_image(window->background_image);
			buffer = imlib_create_cropped_image(up_x, up_y, up_w, up_h);
			imlib_context_set_image(buffer);
			imlib_image_set_has_alpha(1);
		} else {
			buffer = imlib_create_image(up_w, up_h);
			imlib_context_set_image(buffer);
			imlib_image_set_has_alpha(1);
			imlib_image_clear_color(window->background_color.red, window->background_color.green,
					window->background_color.blue, window->background_color.alpha);
		}

		/* TODO grab_root / parent */

		for (l = window->widget_list; l; l = l->next)
			xsg_widgets_render(l->data, buffer, up_x, up_y, up_w, up_h);

		imlib_context_set_image(buffer);
		imlib_context_set_blend(0);

		if (window->argb_visual)
			xrender(window, up_x, up_y);
		else
			imlib_render_image_on_drawable(up_x, up_y);

		imlib_context_set_image(buffer);
		imlib_context_set_blend(1);
		imlib_free_image();
	}

	xsg_update_free(window->updates);
	window->updates = NULL;

	if (window->xshape > 0 && !window->argb_visual) {
		XSetWindowBackgroundPixmap(display, window->window, window->pixmap);
		XShapeCombineMask(display, window->window, ShapeBounding, 0, 0, window->mask, ShapeSet);
		XClearWindow(display, window->window);
		//XSync(display, False);
	}
}

void xsg_window_render(void) {
	xsg_list_t *l;

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		render(window);
	}
}


/******************************************************************************/

static void update_visible(xsg_window_t *window) {
	bool visible;

	visible = window->visible;

	if (window->visible_update != 0)
		window->visible = (xsg_var_get_num(window->visible_var) == 0.0) ? FALSE : TRUE;
	else
		window->visible = TRUE;

	if (window->visible != visible) {
		if (window->visible) {
			xsg_debug("%s: XMapWindow", window->config);

			XMapWindow(display, window->window);

			if (window->sticky)
				set_xatom(window, "_NET_WM_STATE", "_NET_WM_STATE_STICKY");

			if (window->skip_taskbar)
				set_xatom(window, "_NET_WM_STATE", "_NET_WM_STATE_SKIP_TASKBAR");

			if (window->skip_pager)
				set_xatom(window, "_NET_WM_STATE", "_NET_WM_STATE_SKIP_PAGER");

			if (window->layer > 0)
				set_xatom(window, "_NET_WM_STATE", "_NET_WM_STATE_ABOVE");
			else if (window->layer < 0)
				set_xatom(window, "_NET_WM_STATE", "_NET_WM_STATE_BELOW");

			xsg_window_update_append_rect(window, 0, 0, window->width, window->height);
		} else {
			xsg_debug("%s: XUnmapWindow", window->config);

			XUnmapWindow(display, window->window);
		}
	}
}

/******************************************************************************
 *
 * update append rect
 *
 ******************************************************************************/

void xsg_window_update_append_rect(xsg_window_t *window, int xoffset, int yoffset, int width, int height) {
	window->updates = xsg_update_append_rect(window->updates, xoffset, yoffset, width, height);
}

/******************************************************************************
 *
 * async update func
 *
 ******************************************************************************/

void xsg_window_update(xsg_window_t *window, xsg_widget_t *widget, xsg_var_t *var) {
	if (widget == NULL)
		update_visible(window);
	else
		xsg_widgets_update(widget, var);
}

/******************************************************************************
 *
 * tick update func
 *
 ******************************************************************************/

static void update(uint64_t tick) {
	xsg_list_t *l;

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		if (window->copy_from_root)
			if (check_root_background(window) || ((window->background_image_update != 0) && (tick % window->background_image_update) == 0))
				grab_root_background(window);

		if (window->copy_from_parent)
			if (check_parent_background(window) || ((window->background_image_update != 0) && (tick % window->background_image_update) == 0))
				grab_parent_background(window);

		if ((window->visible_update != 0) && (tick % window->visible_update) == 0)
			update_visible(window);
	}

	xsg_window_render();
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

void hide_decorations(xsg_window_t *window) {
	Atom hints_atom;
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	MotifWmHints *hints;
	MotifWmHints **hints_pointer = &hints;
	MotifWmHints new_hints = { 0 };

	hints_atom = XInternAtom(display, "_MOTIF_WM_HINTS", FALSE);

	XGetWindowProperty(display, window->window, hints_atom, 0, sizeof(MotifWmHints)/sizeof(long), False, AnyPropertyType,
			&type, &format, &nitems, &bytes_after, (unsigned char **) hints_pointer);

	if (type == None)
		hints = &new_hints;

	hints->flags |= (1L << 1);
	hints->decorations = 0;

	xsg_message("%s: Hiding window decorations", window->config);

	XChangeProperty(display, window->window, hints_atom, hints_atom, 32,
			PropModeReplace, (unsigned char *) hints, sizeof(MotifWmHints)/sizeof(long));

	if (hints != &new_hints)
		XFree(hints);
}

/******************************************************************************
 *
 * size hints
 *
 ******************************************************************************/

static void set_size_hints(xsg_window_t *window) {
	XSizeHints *size_hints;

	size_hints = XAllocSizeHints();

	size_hints->flags = PMinSize | PMaxSize | PSize | USPosition | PWinGravity;

	size_hints->min_width = window->width;
	size_hints->max_width = window->width;
	size_hints->min_height = window->height;
	size_hints->max_height = window->height;
	size_hints->height = window->height;
	size_hints->width = window->width;
	size_hints->x = window->xoffset;
	size_hints->y = window->yoffset;
	size_hints->win_gravity = window->gravity;

	xsg_debug("%s: Setting size hints", window->config);

	XSetWMNormalHints(display, window->window, size_hints);

	XFree(size_hints);
}

/******************************************************************************
 *
 * class hints
 *
 ******************************************************************************/

static void set_class_hints(xsg_window_t *window) {
	XClassHint *class_hint;

	class_hint = XAllocClassHint();

	class_hint->res_name = window->resource;
	class_hint->res_class = window->class;

	xsg_debug("%s: Setting class hints", window->config);

	XSetClassHint(display, window->window, class_hint);

	XFree(class_hint);
}

/******************************************************************************
 *
 * X11 event handler
 *
 ******************************************************************************/

static void handle_xevents(void *arg, xsg_main_poll_events_t events) {
	xsg_list_t *l;
	XEvent event;

	while (XPending(display)) {
		XNextEvent(display, &event);
		if (event.type == Expose) {
			for (l = window_list; l; l = l->next) {
				xsg_window_t *window = l->data;

				if (window->window == event.xexpose.window) {
					window->xexpose_updates = imlib_update_append_rect(window->xexpose_updates,
							event.xexpose.x, event.xexpose.y,
							event.xexpose.width, event.xexpose.height);
				}
			}
		}
	}

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		if (window->xshape > 0) {
			if (window->xexpose_updates != 0) {
				XClearWindow(display, window->window);
				XSync(display, False);

				imlib_updates_free(window->xexpose_updates);
				window->xexpose_updates = 0;
			}
		} else {
			if (window->xexpose_updates != 0)
				render(window);
		}
	}
}

/******************************************************************************
 *
 * io error handler
 *
 ******************************************************************************/

static int io_error_handler(Display *display) {
	xsg_error("X connection broken");
	return 0;
}

/******************************************************************************
 *
 * signal handler
 *
 ******************************************************************************/

static bool need_render_all = FALSE;

static void signal_handler(int signum) {
	if (signum == SIGHUP)
		need_render_all = TRUE;
}

static void signal_cleanup(int signum) {
	xsg_list_t *l;

	if (need_render_all) {
		need_render_all = FALSE;

		xsg_message("Rendering all windows");

		for (l = window_list; l; l = l->next) {
			xsg_window_t *window = l->data;

			if (window->copy_from_root)
				grab_root_background(window);
			if (window->copy_from_parent)
				grab_parent_background(window);

			xsg_window_update_append_rect(window, 0, 0, window->width, window->height);
		}
	}

	xsg_window_render();
}

/******************************************************************************
 *
 * initialize window
 *
 ******************************************************************************/

void xsg_window_init() {
	xsg_list_t *l;

	XSetIOErrorHandler(io_error_handler);

	if (display == NULL)
		display = XOpenDisplay(NULL);

	if (unlikely(display == NULL))
		xsg_error("Cannot open display");

	if (screen == 0)
		screen = XDefaultScreen(display);

	imlib_context_set_blend(1);
	imlib_context_set_dither(0);
	imlib_context_set_dither_mask(0);
	imlib_context_set_anti_alias(1);
	imlib_context_set_display(display);

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;
		XSetWindowAttributes attrs;
		unsigned long valuemask;

		if (window->flags & XNegative) {
			window->xoffset += DisplayWidth(display, screen) - window->width;
			window->gravity = NorthEastGravity;
		}

		if (window->flags & YNegative) {
			window->yoffset += DisplayHeight(display, screen) - window->height;
			window->gravity = (window->gravity == NorthEastGravity ? SouthEastGravity : SouthWestGravity);
		}

		if (window->argb_visual) {
			xrender_check();
			window->visual = find_argb_visual();
			window->depth = 32;
		} else {
			window->visual = imlib_get_best_visual(display, screen, &window->depth);
		}

		if (window->xshape) {
			int event_base, error_base;

			if (!XShapeQueryExtension(display, &event_base, &error_base))
				xsg_error("%s: No xshape extension found", window->config);
		}

		window->colormap = XCreateColormap(display, RootWindow(display, screen), window->visual, AllocNone);

		attrs.background_pixel = 0;
		attrs.colormap = window->colormap;
		valuemask = CWBackPixel | CWColormap;

		if (window->override_redirect) {
			attrs.override_redirect = 1;
			valuemask |= CWOverrideRedirect;
		}

		window->window = XCreateWindow(display, XRootWindow(display, screen),
			window->xoffset, window->yoffset, window->width, window->height, 0,
			window->depth, InputOutput, window->visual, valuemask, &attrs);

		set_size_hints(window);
		set_class_hints(window);

		XStoreName(display, window->window, window->name);

		XSelectInput(display, window->window, ExposureMask);

		if (window->argb_visual)
			xrender_init(window);

		if (!window->decorations)
			hide_decorations(window);


		if (window->xshape > 0) {
			window->mask = XCreatePixmap(display, window->window, window->width, window->height, 1);
			if (!window->argb_visual)
				window->pixmap = XCreatePixmap(display, window->window, window->width, window->height, window->depth);
		}

		window->updates = xsg_update_append_rect(window->updates, 0, 0, window->width, window->height);
	}

	xsg_main_add_update_func(update);

	xsg_widgets_init();

	poll.fd = ConnectionNumber(display);
	poll.events = XSG_MAIN_POLL_READ;
	poll.func = handle_xevents;
	poll.arg = NULL;

	xsg_main_add_poll(&poll);

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		if (window->copy_from_root) {
			check_root_background(window);
			grab_root_background(window);
		}

		if (window->copy_from_parent) {
			check_parent_background(window);
			grab_parent_background(window);
		}

		update_visible(window);
	}

	xsg_main_add_signal_cleanup(signal_cleanup);
	xsg_main_add_signal_handler(signal_handler);
}

bool xsg_window_color_lookup(char *name, uint32_t *color) {
	Colormap cm;
	XColor c;

	XSetIOErrorHandler(io_error_handler);

	if (display == NULL)
		display = XOpenDisplay(NULL);

	if (unlikely(display == NULL))
		xsg_error("Cannot open display");

	if (screen == 0)
		screen = XDefaultScreen(display);

	cm = DefaultColormap(display, screen);

	if (XParseColor(display, cm, name, &c) == 0)
		return FALSE;

	A_VAL(color) = 0xff;
	R_VAL(color) = c.red >> 8;
	G_VAL(color) = c.green >> 8;
	B_VAL(color) = c.blue >> 8;

	return TRUE;
}


