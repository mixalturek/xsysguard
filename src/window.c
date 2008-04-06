/* window.c
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
#include <signal.h>
#include <string.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#include <Imlib2.h>

#include "window.h"
#include "argb.h"
#include "xrender.h"
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

	int position_overwrite;
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
	xsg_main_timeout_t copy_from_parent_timeout;

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

	unsigned int button_exit;
	unsigned int button_move;

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

static void
copy_from_parent_timeout(void *arg, bool time_error);

xsg_window_t *
xsg_window_new(char *config_name, int flags, int xoffset, int yoffset)
{
	xsg_window_t *window;

	window = xsg_new(xsg_window_t, 1);

	window->config = xsg_strdup(config_name);

	window->name = xsg_strdup("xsysguard");
	window->class = xsg_strdup("xsysguard");
	window->resource = xsg_strdup("xsysguard");

	window->position_overwrite = !!flags;
	window->flags = flags | WidthValue | HeightValue;
	window->gravity = NorthWestGravity;

	window->xoffset = xoffset; /* 64 */
	window->yoffset = yoffset; /* 64 */
	window->width = 128;
	window->height = 128;

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
	window->copy_from_parent_timeout.tv.tv_sec = 0;
	window->copy_from_parent_timeout.tv.tv_usec = 0;
	window->copy_from_parent_timeout.func = copy_from_parent_timeout;
	window->copy_from_parent_timeout.arg = window;

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

	window->button_exit = 0;
	window->button_move = 0;

	window->widget_list = NULL;

	window_list = xsg_list_append(window_list, window);

	return window;
}

/******************************************************************************/

void
xsg_window_add_widget(xsg_window_t *window, xsg_widget_t *widget)
{
	window->widget_list = xsg_list_append(window->widget_list, widget);
}

char *
xsg_window_get_config_name(xsg_window_t *window)
{
	return window->config;
}

/******************************************************************************
 *
 * parse configuration
 *
 ******************************************************************************/

void
xsg_window_parse_name(xsg_window_t *window)
{
	xsg_free(window->name);
	window->name = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void
xsg_window_parse_class(xsg_window_t *window)
{
	xsg_free(window->class);
	window->class = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void
xsg_window_parse_resource(xsg_window_t *window)
{
	xsg_free(window->resource);
	window->resource = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void
xsg_window_parse_size(xsg_window_t *window)
{
	window->width = xsg_conf_read_uint();
	window->height = xsg_conf_read_uint();
	xsg_conf_read_newline();
}

void
xsg_window_parse_position(xsg_window_t *window)
{
	int xoffset, yoffset, flags;

	flags = XValue | YValue;

	if (xsg_conf_read_offset(&xoffset)) {
		flags |= XNegative;
	}
	if (xsg_conf_read_offset(&yoffset)) {
		flags |= YNegative;
	}

	xsg_conf_read_newline();

	if (!window->position_overwrite) {
		window->flags &= ~XNegative;
		window->flags &= ~YNegative;
		window->flags |= flags;
		window->xoffset = xoffset;
		window->yoffset = yoffset;
	}
}

void
xsg_window_parse_sticky(xsg_window_t *window)
{
	window->sticky = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_skip_taskbar(xsg_window_t *window)
{
	window->skip_taskbar = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_skip_pager(xsg_window_t *window)
{
	window->skip_pager = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_layer(xsg_window_t *window)
{
	if (xsg_conf_find_command("Above")) {
		window->layer = 1;
	} else if (xsg_conf_find_command("Normal")) {
		window->layer = 0;
	} else if (xsg_conf_find_command("Below")) {
		window->layer = -1;
	} else {
		xsg_conf_error("Above, Normal or Below expected");
	}
	xsg_conf_read_newline();
}

void
xsg_window_parse_decorations(xsg_window_t *window)
{
	window->decorations = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_override_redirect(xsg_window_t * window)
{
	window->override_redirect = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_background(xsg_window_t * window)
{
	if (xsg_conf_find_command("CopyFromParent")) {
		window->copy_from_parent = TRUE;
		window->background_image_update = xsg_conf_read_uint();
	} else if (xsg_conf_find_command("CopyFromRoot")) {
		window->copy_from_root = TRUE;
		window->background_image_update = xsg_conf_read_uint();
	} else if (xsg_conf_find_command("Color")) {
		xsg_imlib_uint2color(xsg_conf_read_color(),
				&window->background_color);
	} else {
		xsg_conf_error("CopyFromParent, CopyFromRoot or Color "
				"expected");
	}
	xsg_conf_read_newline();
}

void
xsg_window_parse_xshape(xsg_window_t *window)
{
	uint64_t val = xsg_conf_read_uint();

	window->xshape = MIN(val, 255);
	xsg_conf_read_newline();
}

void
xsg_window_parse_argb_visual(xsg_window_t *window)
{
	window->argb_visual = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void
xsg_window_parse_visible(xsg_window_t *window)
{
	window->visible_update = xsg_conf_read_uint();
	window->visible_var = xsg_var_parse_num(window->visible_update, window,
			NULL);
	xsg_conf_read_newline();
}

void
xsg_window_parse_mouse(xsg_window_t *window)
{
	unsigned int button;

	button = xsg_conf_read_uint();

	if (xsg_conf_find_command("Exit")) {
		window->button_exit = button;
	} else if (xsg_conf_find_command("Move")) {
		window->button_move = button;
	} else {
		xsg_conf_error("Exit or Move expected");
	}
	xsg_conf_read_newline();
}

/******************************************************************************
 *
 * set xatom
 *
 ******************************************************************************/

static void
set_xatom(xsg_window_t *window, const char *type, const char *property)
{
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

	XSendEvent(display, XRootWindow(display, screen), FALSE,
			SubstructureNotifyMask, &xev);
}

/******************************************************************************
 *
 * grab background
 *
 ******************************************************************************/

static void
grab_background(xsg_window_t *window, Window src_window)
{
	Imlib_Image background;
	int status;
	int x, y;
	Window src;
	XSetWindowAttributes attrs;

	if (unlikely(src_window == None)) {
		return;
	}

	status = XTranslateCoordinates(display, window->window, src_window,
			0, 0, &x, &y, &src);

	if (unlikely(!status)) {
		return;
	}

	attrs.background_pixmap = ParentRelative;
	attrs.backing_store = Always;
	attrs.override_redirect = True;
	attrs.event_mask = ExposureMask;

	src = XCreateWindow(display, src_window, x, y, window->width,
			window->height, 0, window->depth, CopyFromParent,
			window->visual,	CWBackPixmap | CWBackingStore
			| CWOverrideRedirect | CWEventMask, &attrs);

	if (unlikely(!src)) {
		return;
	}

	imlib_context_set_drawable(src);
	imlib_context_set_visual(window->visual);
	imlib_context_set_colormap(window->colormap);

	XMapRaised(display, src);
	XSync(display, False);

	background = imlib_create_image_from_drawable(0, 0, 0, window->width,
			window->height, 0);

	XDestroyWindow(display, src);

	if (background) {
		if (window->background_image) {
			imlib_context_set_image(window->background_image);
			imlib_free_image();
		}
		window->background_image = background;
		xsg_window_update_append_rect(window, 0, 0, window->width,
				window->height);
	}
}

static bool
check_root_background(xsg_window_t *window)
{
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

	status = XTranslateCoordinates(display, window->window, root, 0, 0,
			&x, &y, &src);

	if (status != None) {
		if ((x != window->copy_from_root_xoffset)
		 || (y != window->copy_from_root_yoffset)) {
			window->copy_from_root_xoffset = x;
			window->copy_from_root_yoffset = y;
			return TRUE;
		}
	}

	if (id == None) {
		id = XInternAtom(display, "_XROOTMAP_ID", True);
	}

	if (id == None) {
		return FALSE;
	}

	status = XGetWindowProperty(display, root, id, 0, 1, False, XA_PIXMAP,
			&act_type, &act_format, &nitems, &bytes_after, &prop);

	if (status == Success && prop != NULL) {
		pixmap = *((Pixmap *)prop);
		XFree(prop);
		if (old_pixmap != pixmap) {
			old_pixmap = pixmap;
			return TRUE;
		}
	}

	return FALSE;
}

static void
grab_root_background(xsg_window_t *window)
{
	Window root = RootWindow(display, screen);

	xsg_message("%s: Grabbing root window: 0x%lx", window->config,
			(unsigned long) root);

	grab_background(window, root);
}

static void
grab_parent_background(xsg_window_t *window)
{
	Status status;
	unsigned int nchildren;
	Window *children = NULL;
	Window root;
	Window parent = None;

	status = XQueryTree(display, window->window, &root, &parent, &children,
			&nchildren);

	if (children != NULL) {
		XFree(children);
	}

	if (parent == None) {
		xsg_warning("%s: cannot determine parent window",
				window->config);
		return;
	}

	xsg_message("%s: grabbing parent window: 0x%lx", window->config,
			(unsigned long) parent);

	grab_background(window, parent);
}

static void
copy_from_parent_timeout(void *arg, bool time_error)
{
	xsg_window_t *window = arg;
	grab_parent_background(window);
	xsg_main_remove_timeout(&window->copy_from_parent_timeout);
}

/******************************************************************************
 *
 * render window
 *
 ******************************************************************************/

static void
render(xsg_window_t *window)
{
	Imlib_Image buffer;
	xsg_list_t *update;

	if (!window->visible) {
		return;
	}

	xsg_debug("%s: render...", window->config);

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

			window->xexpose_updates
				= imlib_updates_merge_for_rendering(
						window->xexpose_updates,
						window->width, window->height);

			for (xexpose_update = window->xexpose_updates;
			     xexpose_update;
			     xexpose_update = imlib_updates_get_next(
						xexpose_update)) {
				int x, y, w, h;

				imlib_updates_get_coordinates(xexpose_update,
						&x, &y, &w, &h);
				window->updates = xsg_update_append_rect(
						window->updates, x, y, w, h);
			}

			imlib_updates_free(window->xexpose_updates);
			window->xexpose_updates = 0;
		}
	}

	if (window->updates == NULL) {
		return;
	}

	for (update = window->updates; update; update = update->next) {
		int up_x = 0, up_y = 0, up_w = 0, up_h = 0;
		xsg_list_t *l;

		xsg_update_get_coordinates(update, &up_x, &up_y, &up_w, &up_h);

		xsg_debug("%s: render x=%d, y=%d, width=%d, height=%d",
				window->config, up_x, up_y, up_w, up_h);

		if (window->background_image) {
			imlib_context_set_image(window->background_image);
			buffer = imlib_create_cropped_image(up_x, up_y,
					up_w, up_h);
			imlib_context_set_image(buffer);
			imlib_image_set_has_alpha(1);
		} else {
			buffer = imlib_create_image(up_w, up_h);
			imlib_context_set_image(buffer);
			imlib_image_set_has_alpha(1);
			imlib_image_clear_color(window->background_color.red,
					window->background_color.green,
					window->background_color.blue,
					window->background_color.alpha);
		}

		for (l = window->widget_list; l; l = l->next) {
			xsg_widgets_render(l->data, buffer, up_x, up_y,
					up_w, up_h);
		}

		imlib_context_set_image(buffer);
		imlib_context_set_blend(0);

		if (window->argb_visual) {
			xsg_xrender_render(window->window, window->visual,
					window->mask, window->xshape,
					imlib_image_get_data_for_reading_only(),
					up_x, up_y, up_w, up_h);
		} else {
			imlib_render_image_on_drawable(up_x, up_y);
		}

		imlib_context_set_image(buffer);
		imlib_context_set_blend(1);
		imlib_free_image();
	}

	xsg_update_free(window->updates);
	window->updates = NULL;

	if (window->xshape > 0) {
		if (window->argb_visual) {
			XShapeCombineMask(display, window->window,
					ShapeBounding, 0, 0, window->mask,
					ShapeSet);
		} else {
			XSetWindowBackgroundPixmap(display, window->window,
					window->pixmap);
			XShapeCombineMask(display, window->window,
					ShapeBounding, 0, 0, window->mask,
					ShapeSet);
			XClearWindow(display, window->window);
			/* XSync(display, False); */
		}
	}
}

/******************************************************************************
 *
 * X11 event handler
 *
 ******************************************************************************/

static void
gettimeofday_and_add(struct timeval *tv, time_t tv_sec, suseconds_t tv_usec)
{
	if (unlikely(tv == NULL)) {
		return;
	}

	xsg_gettimeofday(tv, NULL);

	tv->tv_sec += tv_sec;
	tv->tv_sec += tv_usec / 1000000;
	tv->tv_usec += tv_usec % 1000000;

	if (tv->tv_usec >= 1000000) {
		++tv->tv_sec;
		tv->tv_usec -= 1000000;
	}
}

static void
handle_move_event(xsg_window_t *window, XEvent *event)
{
	static int orig_x, orig_y, press_x, press_y;
	static Window win = None;

	switch (event->type) {
	case ButtonPress: {
		Window *children, parent, root, child;
		XWindowAttributes win_attr;
		unsigned int nchildren;
		int win_x, win_y;
		unsigned int mask;

		win = event->xbutton.window;

		XGetWindowAttributes(display, win, &win_attr);

		XQueryTree(display, win, &root, &parent,
				 &children, &nchildren);

		if (children) {
			XFree(children);
		}

		XTranslateCoordinates(display, parent, root,
				win_attr.x, win_attr.y,
				&orig_x, &orig_y, &child);

		XQueryPointer(display, RootWindow(display, screen),
				&root, &child, &press_x, &press_y,
				&win_x, &win_y, &mask);
		break;
	}
	case ButtonRelease:
		win = None;
		break;
	case MotionNotify: {
		Window root, child;
		int win_x, win_y;
		unsigned int mask;
		int move_x, move_y;

		if (win != event->xmotion.window) {
			break;
		}

		xsg_debug("moving window...");

		XQueryPointer(display, RootWindow(display, screen),
				&root, &child, &move_x, &move_y,
				&win_x, &win_y, &mask);
		XMoveWindow(display, win,
				orig_x + (move_x - press_x),
				orig_y + (move_y - press_y));
		break;
	}
	default:
		break;
	}
}

static void
handle_xevent(void)
{
	xsg_list_t *l;
	XEvent event;

	XNextEvent(display, &event);
	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		switch (event.type) {
		case Expose:
			if (window->window == event.xexpose.window) {
				window->xexpose_updates
					= imlib_update_append_rect(
						window->xexpose_updates,
						event.xexpose.x,
						event.xexpose.y,
						event.xexpose.width,
						event.xexpose.height);
				xsg_debug("%s: received Expose event: x=%d, "
						"y=%d, w=%d, h=%d",
						window->config,
						event.xexpose.x,
						event.xexpose.y,
						event.xexpose.width,
						event.xexpose.height);
			}
			break;
		case ReparentNotify:
			if (window->window == event.xreparent.window) {
				xsg_message("%s: received ReparentNotify "
						"event. new parent is: 0x%lx",
						window->config,
						(unsigned long)
						event.xreparent.parent);
				if (window->copy_from_parent) {
					gettimeofday_and_add(
						&window->copy_from_parent_timeout.tv,
						0, 100 * 1000);
					xsg_main_add_timeout(
						&window->copy_from_parent_timeout);
				}
			}
			break;
		case ButtonPress:
			if (window->window != event.xbutton.window) {
				break;
			}
			xsg_debug("received XEvent: ButtonPress %u",
					(unsigned) event.xbutton.button);
			if (event.xbutton.button == window->button_exit) {
				xsg_error("pressed mouse button %u: exiting...",
						window->button_exit);
			}
			if (event.xbutton.button == window->button_move) {
				handle_move_event(window, &event);
			}
			break;
		case ButtonRelease:
			if (window->window != event.xbutton.window) {
				break;
			}
			xsg_debug("received XEvent: ButtonRelease %u",
					(unsigned) event.xbutton.button);
			if (event.xbutton.button == window->button_move) {
				handle_move_event(window, &event);
			}
			break;
		case MotionNotify:
			if (window->window != event.xmotion.window) {
				break;
			}
			xsg_debug("received XEvent: MotionNotify");
			handle_move_event(window, &event);
			break;
		default:
			break;
		}
	}
}

static void
handle_xevents(void *arg, xsg_main_poll_events_t events)
{
	xsg_list_t *l;

	while (XPending(display)) {
		while (XPending(display)) {
			handle_xevent();
		}

		for (l = window_list; l; l = l->next) {
			xsg_window_t *window = l->data;

			if (window->argb_visual) {
				if (window->xexpose_updates != 0) {
					render(window);
				}
			} else if (window->xshape > 0) {
				if (window->xexpose_updates != 0) {
					XClearWindow(display, window->window);
					XSync(display, False);
					imlib_updates_free(
						window->xexpose_updates);
					window->xexpose_updates = 0;
				}
			} else {
				if (window->xexpose_updates != 0) {
					render(window);
				}
			}
		}
	}
}

/******************************************************************************
 *
 * render all windows
 *
 ******************************************************************************/

void
xsg_window_render(void)
{
	xsg_list_t *l;

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		render(window);
	}

	handle_xevents(NULL, 0);
}

/******************************************************************************
 *
 * update visible
 *
 ******************************************************************************/

static void
update_visible(xsg_window_t *window)
{
	bool visible;

	visible = window->visible;

	if (window->visible_update != 0) {
		window->visible = (xsg_var_get_num(window->visible_var) == 0.0)
			? FALSE : TRUE;
	} else {
		window->visible = TRUE;
	}

	if (window->visible != visible) {
		if (window->visible) {
			xsg_debug("%s: XMapWindow", window->config);

			XMapWindow(display, window->window);

			if (window->sticky) {
				set_xatom(window, "_NET_WM_STATE",
						"_NET_WM_STATE_STICKY");
			}

			if (window->skip_taskbar) {
				set_xatom(window, "_NET_WM_STATE",
						"_NET_WM_STATE_SKIP_TASKBAR");
			}

			if (window->skip_pager) {
				set_xatom(window, "_NET_WM_STATE",
						"_NET_WM_STATE_SKIP_PAGER");
			}

			if (window->layer > 0) {
				set_xatom(window, "_NET_WM_STATE",
						"_NET_WM_STATE_ABOVE");
			} else if (window->layer < 0) {
				set_xatom(window, "_NET_WM_STATE",
						"_NET_WM_STATE_BELOW");
			}

			xsg_window_update_append_rect(window, 0, 0,
					window->width, window->height);
		} else {
			xsg_debug("%s: XUnmapWindow", window->config);

			XUnmapWindow(display, window->window);
		}
	}
}

/******************************************************************************
 *
 * tick update func
 *
 ******************************************************************************/

void
xsg_window_update(uint64_t tick)
{
	xsg_list_t *l;

	for (l = window_list; l; l = l->next) {
		xsg_window_t *window = l->data;

		if (window->copy_from_root) {
			if (check_root_background(window)
			 || ((window->background_image_update != 0)
			  && (tick % window->background_image_update) == 0)) {
				grab_root_background(window);
			}
		}

		if (window->copy_from_parent) {
			if ((window->background_image_update != 0)
			 && (tick % window->background_image_update) == 0) {
				grab_parent_background(window);
			}
		}

		if ((window->visible_update != 0)
		 && (tick % window->visible_update) == 0) {
			update_visible(window);
		}
	}

	xsg_window_render();
}


/******************************************************************************
 *
 * update append rect
 *
 ******************************************************************************/

void
xsg_window_update_append_rect(
	xsg_window_t *window,
	int xoffset,
	int yoffset,
	int width,
	int height
)
{
	window->updates = xsg_update_append_rect(window->updates, xoffset,
			yoffset, width, height);
}

/******************************************************************************
 *
 * async update func
 *
 ******************************************************************************/

void
xsg_window_update_var(
	xsg_window_t *window,
	xsg_widget_t *widget,
	xsg_var_t *var
)
{
	if (widget == NULL) {
		update_visible(window);
		xsg_window_render();
	} else {
		xsg_widgets_update_var(widget, var);
	}
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

static void
hide_decorations(xsg_window_t *window)
{
	Atom hints_atom;
	Atom type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	MotifWmHints *hints;
	MotifWmHints **hints_pointer = &hints;
	MotifWmHints new_hints = { 0 };

	hints_atom = XInternAtom(display, "_MOTIF_WM_HINTS", FALSE);

	XGetWindowProperty(display, window->window, hints_atom, 0,
			sizeof(MotifWmHints)/sizeof(long), False,
			AnyPropertyType, &type, &format, &nitems,
			&bytes_after, (unsigned char **) hints_pointer);

	if (type == None) {
		hints = &new_hints;
	}

	hints->flags |= (1L << 1);
	hints->decorations = 0;

	xsg_message("%s: hiding window decorations", window->config);

	XChangeProperty(display, window->window, hints_atom, hints_atom, 32,
			PropModeReplace, (unsigned char *) hints,
			sizeof(MotifWmHints)/sizeof(long));

	if (hints != &new_hints) {
		XFree(hints);
	}
}

/******************************************************************************
 *
 * size hints
 *
 ******************************************************************************/

static void
set_size_hints(xsg_window_t *window)
{
	XSizeHints *size_hints;

	size_hints = XAllocSizeHints();

	size_hints->flags = PMinSize | PMaxSize | PSize | USPosition
			| PWinGravity;

	size_hints->min_width = window->width;
	size_hints->max_width = window->width;
	size_hints->min_height = window->height;
	size_hints->max_height = window->height;
	size_hints->height = window->height;
	size_hints->width = window->width;
	size_hints->x = window->xoffset;
	size_hints->y = window->yoffset;
	size_hints->win_gravity = window->gravity;

	xsg_debug("%s: setting size hints", window->config);

	XSetWMNormalHints(display, window->window, size_hints);

	XFree(size_hints);
}

/******************************************************************************
 *
 * class hints
 *
 ******************************************************************************/

static void
set_class_hints(xsg_window_t *window)
{
	XClassHint *class_hint;

	class_hint = XAllocClassHint();

	class_hint->res_name = window->resource;
	class_hint->res_class = window->class;

	xsg_debug("%s: setting class hints", window->config);

	XSetClassHint(display, window->window, class_hint);

	XFree(class_hint);
}

/******************************************************************************
 *
 * error handler
 *
 ******************************************************************************/

static int
io_error_handler(Display *dpy)
{
	xsg_error("X connection broken");
	return 0;
}

static int
error_handler(Display *dpy, XErrorEvent *event)
{
	char buf[1024];

	XGetErrorText(dpy, event->error_code, buf, sizeof(buf));
	xsg_error("XError: %s", buf);
	return 0;
}

/******************************************************************************
 *
 * signal handler
 *
 ******************************************************************************/

static void
signal_handler(int signum)
{
	xsg_list_t *l;

	if (signum == SIGHUP) {
		xsg_message("Rendering all windows");

		for (l = window_list; l; l = l->next) {
			xsg_window_t *window = l->data;

			if (window->copy_from_root) {
				grab_root_background(window);
			}
			if (window->copy_from_parent) {
				grab_parent_background(window);
			}

			xsg_window_update_append_rect(window, 0, 0,
					window->width, window->height);
		}
	}

	xsg_window_render();
}

/******************************************************************************
 *
 * initialize window
 *
 ******************************************************************************/

void
xsg_window_init(void)
{
	xsg_list_t *l;

	XSetIOErrorHandler(io_error_handler);
	XSetErrorHandler(error_handler);

	if (display == NULL) {
		display = XOpenDisplay(NULL);
	}

	if (unlikely(display == NULL)) {
		xsg_error("Cannot open display");
	}

	xsg_set_cloexec_flag(ConnectionNumber(display), TRUE);

	if (screen == 0) {
		screen = XDefaultScreen(display);
	}

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
			window->xoffset += DisplayWidth(display, screen)
				- window->width;
			window->gravity = NorthEastGravity;
		}

		if (window->flags & YNegative) {
			window->yoffset += DisplayHeight(display, screen)
				- window->height;
			window->gravity = window->gravity == NorthEastGravity
					? SouthEastGravity : SouthWestGravity;
		}

		if (window->argb_visual) {
			xsg_xrender_init(display);
			window->visual = xsg_xrender_find_visual(screen);
			window->depth = 32;
		} else {
			window->visual = imlib_get_best_visual(display, screen,
					&window->depth);
		}

		if (window->xshape) {
			int event_base, error_base;

			if (!XShapeQueryExtension(display, &event_base,
						&error_base)) {
				xsg_error("%s: No xshape extension found",
						window->config);
			}
		}

		window->colormap = XCreateColormap(display,
				RootWindow(display, screen), window->visual,
				AllocNone);

		attrs.event_mask = ExposureMask | StructureNotifyMask
			| ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;
		attrs.background_pixel = 0;
		attrs.border_pixel = 0;
		attrs.colormap = window->colormap;
		valuemask = CWEventMask | CWBackPixel | CWBorderPixel
			| CWColormap;

		if (window->override_redirect) {
			attrs.override_redirect = 1;
			valuemask |= CWOverrideRedirect;
		}

		window->window = XCreateWindow(display,
				XRootWindow(display, screen),
				window->xoffset, window->yoffset,
				window->width, window->height, 0,
				window->depth, InputOutput, window->visual,
				valuemask, &attrs);

		set_size_hints(window);
		set_class_hints(window);

		XStoreName(display, window->window, window->name);

		if (window->argb_visual) {
			xsg_xrender_redirect(window->window);
		}

		if (!window->decorations) {
			hide_decorations(window);
		}

		if (window->xshape > 0) {
			window->mask = XCreatePixmap(display, window->window,
					window->width, window->height, 1);
			if (!window->argb_visual) {
				window->pixmap = XCreatePixmap(display,
						window->window,
						window->width, window->height,
						window->depth);
			}
		}

		window->updates = xsg_update_append_rect(window->updates, 0, 0,
				window->width, window->height);
	}

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
			grab_parent_background(window);
		}

		update_visible(window);
	}

	xsg_main_add_signal_handler(signal_handler, SIGUSR1);
	xsg_main_add_signal_handler(signal_handler, SIGUSR2);
	xsg_main_add_signal_handler(signal_handler, SIGHUP);
}

/******************************************************************************
 *
 * color lookup
 *
 ******************************************************************************/

bool
xsg_window_color_lookup(char *name, uint32_t *color)
{
	Colormap cm;
	XColor c;

	XSetIOErrorHandler(io_error_handler);
	XSetErrorHandler(error_handler);

	if (display == NULL) {
		display = XOpenDisplay(NULL);
	}

	if (unlikely(display == NULL)) {
		xsg_error("cannot open display");
	}

	xsg_set_cloexec_flag(ConnectionNumber(display), TRUE);

	if (screen == 0) {
		screen = XDefaultScreen(display);
	}

	cm = DefaultColormap(display, screen);

	if (XParseColor(display, cm, name, &c) == 0) {
		return FALSE;
	}

	A_VAL(color) = 0xff;
	R_VAL(color) = c.red >> 8;
	G_VAL(color) = c.green >> 8;
	B_VAL(color) = c.blue >> 8;

	return TRUE;
}

/******************************************************************************
 *
 * extract position form config name
 *
 ******************************************************************************/

char *
xsg_window_extract_position(
	char *str,
	int *flags_return,
	int *x_return,
	int *y_return
)
{
	int flags = XValue | YValue;
	size_t len;
	char *s;
	len = strlen(str);

	s = str + len - 1;

	if (!isdigit(*s)) {
		*flags_return = 0;
		*x_return = 64;
		*y_return = 64;
		return xsg_strdup(str);
	}

	while (s > str && isdigit(*s)) {
		s--;
	}

	*y_return = atoi(s + 1);

	if (*s == '-') {
		flags |= YNegative;
		*y_return = -*y_return;
	} else if (*s != '+' || s == str) {
		*flags_return = 0;
		*x_return = 64;
		*y_return = 64;
		return xsg_strdup(str);
	}

	s--;

	if (!isdigit(*s)) {
		*flags_return = 0;
		*x_return = 64;
		*y_return = 64;
		return xsg_strdup(str);
	}

	while (s > str && isdigit(*s)) {
		s--;
	}

	*x_return = atoi(s + 1);

	if (*s == '-') {
		flags |= XNegative;
		*x_return = -*x_return;
	} else if (*s != '+' || s == str) {
		*flags_return = 0;
		*x_return = 64;
		*y_return = 64;
		return xsg_strdup(str);
	}

	*flags_return = flags;

	return xsg_strndup(str, s - str);
}

