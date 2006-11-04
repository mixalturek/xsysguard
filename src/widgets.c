/* widgets.c
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
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef ENABLE_XRENDER
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#endif

#include "widgets.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

#ifndef HOME_IMAGE_DIR
#define HOME_IMAGE_DIR g_build_filename(g_get_home_dir(), ".xsysguard", "modules", NULL)
#endif

#ifndef IMAGE_DIR
#define IMAGE_DIR "/usr/local/share/xsysguard/images"
#endif

#define ARGB_ALPHA_IDX	3
#define ARGB_RED_IDX	2
#define ARGB_GREEN_IDX	1
#define ARGB_BLUE_IDX	0

#define ALL_VARS 0xffff

typedef union {
	uint32_t uint;
	struct {
		unsigned char a;
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} argb;
	struct {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
	} rgba;
} color_t;

/******************************************************************************/

typedef struct _widget_t widget_t;

struct _widget_t {
	uint64_t update;
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	void (*render_func)(widget_t *widget, Imlib_Image buffer, int x, int y, bool solid_bg);
	void (*update_func)(widget_t *widget, uint16_t var_id);
	void (*scroll_func)(widget_t *widget);
	void *data;
};

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
	uint32_t background;
	bool copy_from_parent;
	bool copy_from_root;
	unsigned int cache_size;
	bool xshape;
	bool argb_visual;
	Display *display;
	Visual *visual;
	int depth;
	int screen;
	Window id;
	Imlib_Updates updates;
	Pixmap mask;
} window_t;

/******************************************************************************/

static xsg_list *widget_list = NULL;

static window_t window = {
	"xsysguard",      /* name */
	"xsysguard",      /* class */
	"xsysguard",      /* resource */
	"64x64+128+128",  /* geometry */
	0,                /* flags */
	NorthWestGravity, /* win_gravity */
	128,              /* xoffset */
	128,              /* yoffset */
	64,               /* width */
	64,               /* height */
	FALSE,            /* sticky */
	FALSE,            /* skip_taskbar */
	FALSE,            /* skip_pager */
	0,                /* layer */
	TRUE,             /* decorations */
	0x00000000,       /* background */
	FALSE,            /* copy_from_parent */
	FALSE,            /* copy_from_root */
	1024,             /* cache_size */
	FALSE,            /* xshape */
	FALSE,            /* argb_visual */
	NULL,             /* display */
	NULL,             /* visual */
	0,                /* depth */
	0,                /* screen */
	0,                /* id */
	0,                /* updates */
	0,                /* mask */
};

/******************************************************************************
 *
 * misc functions
 *
 ******************************************************************************/

static void image_set_color(DATA32 color) {
	DATA32 *data;
	unsigned int width;
	unsigned int height;
	unsigned int i;

	width = imlib_image_get_width();
	height = imlib_image_get_height();
	data = imlib_image_get_data();

	for (i = 0; i < (width * height); i++)
		data[i] = color;

	imlib_image_put_back_data(data);
}

static widget_t *get_widget(uint16_t widget_id) {
	widget_t *widget;

	widget = xsg_list_nth_data(widget_list, (unsigned int) widget_id);

	if (!widget)
		g_error("Invalid widget id: %u", widget_id);

	return widget;
}

static Imlib_Image load_image(const char *filename, bool throw_error) {
	Imlib_Image image = NULL;
	static char **pathv = NULL;
	char **p;
	const char *env;
	char *file;

	if (!pathv) {
		env = g_getenv("XSYSGUARD_IMAGE_PATH");

		if (env) {
			pathv = g_strsplit_set(env, ":", 0);
			for (p = pathv; *p; p++)
				if (*p[0] == '~')
					*p = g_build_filename(g_get_home_dir(), *p, NULL);
		} else {
			pathv = g_new0(char *, 3);
			pathv[0] = HOME_IMAGE_DIR;
			pathv[1] = IMAGE_DIR;
			pathv[2] = NULL;
		}
	}

	for (p = pathv; *p; p++) {
		g_message("Searching for image '%s' in '%s'.", filename, *p);
		file = g_build_filename(*p, filename, NULL);
		if (g_file_test(file, G_FILE_TEST_IS_REGULAR)) {
			image = imlib_load_image(file);
		}
		if (image) {
			g_message("Found image '%s'.", file);
			g_free(file);
			return image;
		}
		g_free(file);
	}

	if (throw_error)
		g_error("Cannot find image '%s'.", filename);

	return NULL;
}

static Imlib_Color uint2color(uint32_t u) {
	Imlib_Color color = { 0, 0, 0 };
	color_t c;

	c.uint = u;
	color.alpha = c.argb.a & 0xff;
	color.red = c.argb.r & 0xff;
	color.green = c.argb.g & 0xff;
	color.blue = c.argb.b & 0xff;

	return color;
}

static void blend_mask(Imlib_Image image, Imlib_Image mask) {
	DATA32 *image_data;
	DATA32 *mask_data;
	unsigned int image_width, image_height;
	unsigned int mask_width, mask_height;
	unsigned int width, height;
	unsigned int x, y;

	imlib_context_set_image(mask);
	mask_width = imlib_image_get_width();
	mask_height = imlib_image_get_height();
	mask_data = imlib_image_get_data_for_reading_only();

	imlib_context_set_image(image);
	image_width = imlib_image_get_width();
	image_height = imlib_image_get_height();
	image_data = imlib_image_get_data();

	width = MIN(mask_width, image_height);
	height = MIN(mask_height, image_height);

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			unsigned char *image;
			unsigned char *mask;
			image = (unsigned char *) image_data + x + y * image_height;
			mask = (unsigned char *) mask_data + x + y * mask_height;

			image[ARGB_BLUE_IDX] *= mask[ARGB_BLUE_IDX] / 0xff;
			image[ARGB_GREEN_IDX] *= mask[ARGB_BLUE_IDX] / 0xff;
			image[ARGB_RED_IDX] *= mask[ARGB_RED_IDX] / 0xff;
			image[ARGB_ALPHA_IDX] *= mask[ARGB_ALPHA_IDX] / 0xff;
		}
	}

	imlib_image_put_back_data(image_data);
}

/******************************************************************************
 *
 * parse configuration
 *
 ******************************************************************************/

void xsg_widgets_parse_name() {
	window.name = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_class() {
	window.class = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_resource() {
	window.resource = xsg_conf_read_string();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_geometry() {
	window.geometry = xsg_conf_read_string();
	xsg_conf_read_newline();
	window.flags = XParseGeometry(window.geometry, &window.xoffset,
			&window.yoffset, &window.width, &window.height);
}

void xsg_widgets_parse_sticky() {
	window.sticky = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_skip_taskbar() {
	window.skip_taskbar = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_skip_pager() {
	window.skip_pager = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_layer() {
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

void xsg_widgets_parse_decorations() {
	window.decorations = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_background() {
	if (xsg_conf_find_command("CopyFromParent"))
		window.copy_from_parent = TRUE;
	else if (xsg_conf_find_command("CopyFromRoot"))
		window.copy_from_root = TRUE;
	else if (xsg_conf_find_command("Color"))
		window.background = xsg_conf_read_color();
	else
		xsg_conf_error("CopyFromParent, CopyFromRoot or Color");
	xsg_conf_read_newline();
}

void xsg_widgets_parse_cache_size() {
	window.cache_size = xsg_conf_read_uint();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_xshape() {
	window.xshape = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

void xsg_widgets_parse_argb_visual() {
	window.argb_visual = xsg_conf_read_boolean();
	xsg_conf_read_newline();
}

/******************************************************************************
 *
 * additional function for 'ImlibPolygon' type
 *
 ******************************************************************************/

typedef struct {
	int x, y;
} point_t;

typedef struct {
	point_t *points;
	int pointcount;
	int lx, rx;
	int ty, by;
} poly_t;

static ImlibPolygon poly_copy(ImlibPolygon polygon, int xoffset, int yoffset) {
	poly_t *poly;
	poly_t *new_poly;
	int i;

	poly = (poly_t *) polygon;
	new_poly = (poly_t *) malloc(sizeof(poly_t));
	new_poly->points = (point_t *) malloc(sizeof(point_t) * poly->pointcount);
	new_poly->pointcount = poly->pointcount;
	new_poly->lx = poly->lx + xoffset;
	new_poly->rx = poly->rx + xoffset;
	new_poly->ty = poly->ty + yoffset;
	new_poly->by = poly->by + yoffset;

	for (i = 0; i < poly->pointcount; i++) {
		new_poly->points[i].x = poly->points[i].x + xoffset;
		new_poly->points[i].y = poly->points[i].y + yoffset;
	}

	return (ImlibPolygon) new_poly;
}

/******************************************************************************
 *
 * angle stuff
 *
 ******************************************************************************/

typedef struct {
	int xoffset;
	int yoffset;
	unsigned int width;
	unsigned int height;
	double angle;
	int angle_x;
	int angle_y;
} angle_t;

static angle_t *parse_angle(double a, int xoffset, int yoffset, unsigned int *width, unsigned int *height) {
	angle_t *angle;
	double arc, sa, ca;
	unsigned int w, h;

	arc = a / 180.0 * G_PI;

	sa = sin(arc);
	ca = cos(arc);

	w = *width;
	h = *height;

	if (sa > 0.0)
		xoffset += sa * h;
	else
		yoffset -= sa * w;

	if (ca < 0.0) {
		xoffset -= ca * w;
		yoffset -= ca * h;
	}

	angle = g_new0(angle_t, 1);

	angle->xoffset = xoffset;
	angle->yoffset = yoffset;
	angle->width = *width;
	angle->height = *height;
	angle->angle = a;
	angle->angle_x = w * ca;
	angle->angle_y = w * sa;

	*width = ceil(fabs(sa * h) + fabs(ca * w));
	*height = ceil(fabs(ca * h) + fabs(sa *w));

	return angle;
}

static bool blend_at_angle(Imlib_Image buffer, Imlib_Image image, angle_t *angle, int up_x, int up_y) {
	imlib_context_set_image(image);

	if (angle->angle == 0.0) {
		return FALSE;
	} else if (angle->angle == 90.0) {
		imlib_image_orientate(1);
		return FALSE;
	} else if (angle->angle == 180.0) {
		imlib_image_orientate(2);
		return FALSE;
	} else if (angle->angle == 270.0) {
		imlib_image_orientate(3);
		return FALSE;
	}

	imlib_context_set_image(buffer);

	imlib_blend_image_onto_image_at_angle(image, 1, 0, 0,
			angle->width, angle->height,
			angle->xoffset - up_x, angle->yoffset - up_y,
			angle->angle_x, angle->angle_y);

	return TRUE;
}

/******************************************************************************
 *
 * text alignment
 *
 ******************************************************************************/

typedef enum {
	TOP_LEFT,
	TOP_CENTER,
	TOP_RIGHT,
	CENTER_LEFT,
	CENTER,
	CENTER_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_CENTER,
	BOTTOM_RIGHT
} alignment_t;

static alignment_t parse_alignment() {

	if (xsg_conf_find_command("TopLeft"))
		return TOP_LEFT;
	else if (xsg_conf_find_command("TopCenter"))
		return TOP_CENTER;
	else if (xsg_conf_find_command("TopRight"))
		return TOP_RIGHT;
	else if (xsg_conf_find_command("CenterLeft"))
		return CENTER_LEFT;
	else if (xsg_conf_find_command("Center"))
		return CENTER;
	else if (xsg_conf_find_command("CenterRight"))
		return CENTER_RIGHT;
	else if (xsg_conf_find_command("BottomLeft"))
		return BOTTOM_LEFT;
	else if (xsg_conf_find_command("BottomCenter"))
		return BOTTOM_CENTER;
	else if (xsg_conf_find_command("BottomRight"))
		return BOTTOM_RIGHT;
	else
		xsg_conf_error("TopLeft, TopCenter, TopRight, CenterLeft, Center, "
				"CenterRight, BottomLeft, BottomCenter or BottomRight");

	return CENTER;
}

/******************************************************************************
 *
 * text stuff
 *
 ******************************************************************************/

typedef struct {
	uint16_t var_id;
	double mult;
	double add;
	uint8_t type;
	union {
		int64_t i;
		uint64_t u;
		double d;
		char *s;
	} value;
} text_var_t;

static void parse_format(char *format, xsg_list *var_list) {
	/* TODO */
}

static void string_format(GString *buffer, char *format, xsg_list *var_list) {
	/* TODO */
}

/******************************************************************************
 *
 * XRender
 *
 ******************************************************************************/

#ifndef ENABLE_XRENDER

#define XRENDER_ERROR g_error("Compiled without XRender support")

static Visual *find_argb_visual() { XRENDER_ERROR; return NULL; }
static void xrender_check() { XRENDER_ERROR; }
static void xrender_init() { XRENDER_ERROR; }
static void xrender(int xoffset, int yoffset) { XRENDER_ERROR; }

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

	xvi = XGetVisualInfo(window.display, VisualScreenMask | VisualDepthMask | VisualClassMask,
			&template, &nvi);

	if (xvi == NULL)
		g_error("Cannot find an argb visual.");

	for (i = 0; i < nvi; i++) {
		format = XRenderFindVisualFormat(window.display, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask) {
			visual = xvi[i].visual;
			break;
		}
	}

	XFree(xvi);

	if (visual == NULL)
		g_error("Cannot find an argb visual.");

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
		g_error("No render extension found.");

	if (!XQueryExtension(window.display, COMPOSITE_NAME, &composite_opcode, &composite_event, &composite_error))
		g_error("No composite extension found.");

	XCompositeQueryVersion(window.display, &composite_major, &composite_minor);

	g_message("Composite extension found: %d.%d", composite_major, composite_minor);
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
	unsigned char *c;
	unsigned char *colors_c;
	unsigned char *alpha_c;
	unsigned char a, r, g, b;
	int i;
	GC gc, mask_gc;

	width = imlib_image_get_width();
	height = imlib_image_get_height();
	data = imlib_image_get_data_for_reading_only();

	colors_data = g_new(DATA32, width * height);
	alpha_data = g_new(DATA32, width * height);

	colors_image = XCreateImage(window.display, window.visual,
			32, ZPixmap, 0,	(char *) colors_data,
			width, height, 32, width * 4);

	alpha_image = XCreateImage(window.display, window.visual,
			32, ZPixmap, 0, (char *) alpha_data,
			width, height, 32, width * 4);

	if (window.xshape) {

		mask_data = g_new(DATA8, width * height);

		mask_image = g_new(XImage, 1);
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

		c = (unsigned char *) (data + i);
		b = c[ARGB_BLUE_IDX];
		g = c[ARGB_GREEN_IDX];
		r = c[ARGB_RED_IDX];
		a = c[ARGB_ALPHA_IDX];

		colors_c = (unsigned char *) (colors_data + i);
		colors_c[ARGB_BLUE_IDX] = b;
		colors_c[ARGB_GREEN_IDX] = g;
		colors_c[ARGB_RED_IDX] = r;
		colors_c[ARGB_ALPHA_IDX] = 0xff;

		alpha_c = (unsigned char *) (alpha_data + i);
		alpha_c[ARGB_BLUE_IDX] = 0;
		alpha_c[ARGB_GREEN_IDX] = 0;
		alpha_c[ARGB_RED_IDX] = 0;
		alpha_c[ARGB_ALPHA_IDX] = a;

		if (window.xshape)
			mask_data[i] = (a == 0) ? 0 : 1;
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
		g_free(mask_data);
		g_free(mask_image);
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

	root_picture = XRenderCreatePicture(window.display, window.id,
			XRenderFindVisualFormat(window.display, window.visual),
			CPSubwindowMode, &root_pict_attrs);

	colors_picture = XRenderCreatePicture(window.display, colors,
			pict_format, 0, 0);

	alpha_picture = XRenderCreatePicture(window.display, alpha,
			pict_format, 0, 0);

	XRenderComposite(window.display, PictOpSrc, colors_picture,
			alpha_picture, root_picture, 0, 0, 0, 0,
			xoffset, yoffset, width, height);

	XRenderFreePicture(window.display, colors_picture);
	XRenderFreePicture(window.display, alpha_picture);

	XFreePixmap(window.display, colors);
	XFreePixmap(window.display, alpha);
}

#endif /* ENABLE_XRENDER */

/******************************************************************************
 *
 * render window buffer
 *
 ******************************************************************************/

static bool widget_rect(widget_t *widget, int x, int y, unsigned int w, unsigned int h) {
	int x1_1, x2_1, y1_1, y2_1, x1_2, x2_2, y1_2, y2_2;
	bool x_overlap, y_overlap;

	x1_1 = widget->xoffset;
	x2_1 = widget->xoffset + widget->width;
	y1_1 = widget->yoffset;
	y2_1 = widget->yoffset + widget->height;

	x1_2 = x;
	x2_2 = x + w;
	y1_2 = y;
	y2_2 = y + h;

	x_overlap = !((x2_2 <= x1_1) || (x2_1 <= x1_2));
	y_overlap = !((y2_2 <= y1_1) || (y2_1 <= y1_2));

	return x_overlap && y_overlap;
}

static void render() {
	int up_x, up_y, up_w, up_h;
	Imlib_Updates update;
	Imlib_Image buffer;
	bool solid_bg;
	xsg_list *l;

	window.updates = imlib_updates_merge_for_rendering(window.updates,
			window.width, window.height);

	for (update = window.updates; update; update = imlib_updates_get_next(update)) {

		imlib_updates_get_coordinates(update, &up_x, &up_y, &up_w, &up_h);

		g_message("Render (x=%d, y=%d, width=%d, height=%d)", up_x, up_y, up_w, up_h);

		buffer = imlib_create_image(up_w, up_h);
		imlib_context_set_image(buffer);
		imlib_image_set_has_alpha(1);
		image_set_color(window.background);
		/* TODO grab_root / parent */

		solid_bg = TRUE;

		for (l = widget_list; l; l = l->next) {
			widget_t *widget = l->data;
			if (widget_rect(widget, up_x, up_y, up_w, up_h)) {
				(widget->render_func)(widget, buffer, up_x, up_y, solid_bg);
				solid_bg = FALSE;
			}
		}

		imlib_context_set_image(buffer);
		imlib_context_set_blend(0);

		if (window.argb_visual)
			xrender(up_x, up_y);
		else
			imlib_render_image_on_drawable(up_x, up_y);

		imlib_context_set_blend(1);
		imlib_free_image();
	}

	if (window.updates)
		imlib_updates_free(window.updates);

	window.updates = 0;
}

/******************************************************************************/

static void scroll_and_update(uint64_t count) {
	widget_t *widget;
	xsg_list *l;

	for (l = widget_list; l; l = l->next) {

		widget = l->data;

		if (widget->update && (count % widget->update) == 0) {

			(widget->scroll_func)(widget);
			(widget->update_func)(widget, ALL_VARS);

			window.updates = imlib_update_append_rect(window.updates,
					widget->xoffset, widget->yoffset,
					widget->width, widget->height);
		}
	}

	render();

	if (window.xshape)
		XShapeCombineMask(window.display, window.id, ShapeBounding,
				0, 0, window.mask, ShapeSet);
}

void xsg_widgets_update(uint16_t widget_id, uint16_t var_id) {
	widget_t *widget;

	widget = get_widget(widget_id);

	(widget->update_func)(widget, var_id);

	window.updates = imlib_update_append_rect(window.updates,
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);

	render();

	if (window.xshape)
		XShapeCombineMask(window.display, window.id, ShapeBounding,
				0, 0, window.mask, ShapeSet);
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

	g_message("Setting Xatom %s = %s", type, property);

	xev.type = ClientMessage;
	xev.xclient.type = ClientMessage;
	xev.xclient.window = window.id;
	xev.xclient.message_type = type_atom;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = property_atom;
	xev.xclient.data.l[2] = 0;

	XSendEvent(window.display, XRootWindow(window.display, window.screen),
			FALSE, SubstructureNotifyMask, &xev);
}

/******************************************************************************
 *
 * hide window decorations
 *
 ******************************************************************************/

#define MWM_HINTS_DECORATIONS (1L << 1)

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
	gulong nitems;
	gulong bytes_after;
	MotifWmHints *hints;
	MotifWmHints **hints_pointer = &hints;
	MotifWmHints new_hints = { 0 };

	hints_atom = XInternAtom(window.display, "_MOTIF_WM_HINTS", FALSE);

	XGetWindowProperty(window.display, window.id, hints_atom, 0,
			sizeof(MotifWmHints)/sizeof(long), False, AnyPropertyType,
			&type, &format, &nitems, &bytes_after, (unsigned char **) hints_pointer);

	if (type == None)
		hints = &new_hints;

	hints->flags |= MWM_HINTS_DECORATIONS;
	hints->decorations = 0;

	XChangeProperty(window.display, window.id, hints_atom, hints_atom, 32,
			PropModeReplace, (guchar *) hints, sizeof(MotifWmHints)/sizeof(long));

	if (hints != &new_hints)
		XFree(hints);
}

/******************************************************************************
 *
 * hints
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
 * X11 GSource
 *
 ******************************************************************************/

static void handle_xevents() {
	XEvent event;

	while (XPending(window.display)) {
		XNextEvent(window.display, &event);
		if (event.type == Expose) {
			g_message("XExpose (x=%d, y=%d, width=%d, height=%d)",
					event.xexpose.x, event.xexpose.y,
					event.xexpose.width, event.xexpose.height);
			window.updates = imlib_update_append_rect(window.updates,
					event.xexpose.x, event.xexpose.y,
					event.xexpose.width, event.xexpose.height);
		} else {
			g_message("XEvent (type=%d)", event.type);
		}
	}
}

static bool prepare_source(GSource *source, int *timeout) {

	handle_xevents();
	return (window.updates != NULL);
}

static bool check_source(GSource *source) {

	handle_xevents();
	return (window.updates != NULL);
}

static bool dispatch_source(GSource *source, GSourceFunc callback, gpointer user_data) {

	handle_xevents();
	if (window.updates)
		render();
	return TRUE;
}

static void attach_source(int fd) {
	GSourceFuncs *source_funcs;
	GSource *source;
	GPollFD* pollfd;

	pollfd = g_new0(GPollFD, 1);
	pollfd->fd = fd;
	pollfd->events = G_IO_IN | G_IO_HUP | G_IO_ERR;
	pollfd->revents = 0;

	source_funcs = g_new0(GSourceFuncs, 1);

	source_funcs->prepare = prepare_source;
	source_funcs->check = check_source;
	source_funcs->dispatch = dispatch_source;
	source_funcs->finalize = NULL;
	source_funcs->closure_callback = 0;
	source_funcs->closure_marshal = 0;

	source = g_source_new(source_funcs, sizeof(GSource));

	g_source_add_poll(source, pollfd);
	g_source_set_priority(source, G_PRIORITY_HIGH_IDLE + 20);

	g_source_attach(source, NULL);
}

/******************************************************************************
 *
 * initialize widgets and window
 *
 ******************************************************************************/

void xsg_widgets_init() {
	Colormap colormap;
	XSetWindowAttributes attrs;

	if ((window.display = XOpenDisplay(NULL)) == NULL)
		g_error("Cannot open display");

	window.screen = XDefaultScreen(window.display);

	if (window.flags & XNegative) {
		window.xoffset += DisplayWidth(window.display, window.screen) - window.width;
		window.win_gravity = NorthEastGravity;
	}

	if (window.flags & YNegative) {
		window.yoffset += DisplayHeight(window.display, window.screen) - window.height;
		window.win_gravity = (window.win_gravity == NorthEastGravity ?
				SouthEastGravity : SouthWestGravity);
	}

	if (window.argb_visual) {
		xrender_check();
		window.visual = find_argb_visual();
		window.depth = 32;
	} else {
		window.visual = imlib_get_best_visual(window.display,
				window.screen, &window.depth);
	}

	if (window.xshape) {
		int event_base, error_base;

		if (!XShapeQueryExtension(window.display, &event_base, &error_base))
			g_error("No xshape extension found");
	}

	colormap = XCreateColormap(window.display,
			RootWindow(window.display, window.screen),
			window.visual, AllocNone);

	attrs.background_pixel = 0;
	attrs.colormap = colormap;

	window.id = XCreateWindow(window.display, XRootWindow(window.display, window.screen),
			window.xoffset, window.yoffset, window.width, window.height, 0,
			window.depth, InputOutput, window.visual,
			CWBackPixel | CWColormap, &attrs);

	set_size_hints();
	set_class_hints();

	XStoreName(window.display, window.id, window.name);

	XSelectInput(window.display, window.id, ExposureMask);

	if (window.argb_visual)
		xrender_init();

	if (!window.decorations)
		hide_decorations();

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

	imlib_context_set_blend(1);
	imlib_context_set_dither(0);
	imlib_context_set_dither_mask(0);
	imlib_context_set_anti_alias(1);
	imlib_context_set_display(window.display);
	imlib_context_set_visual(window.visual);
	imlib_context_set_colormap(colormap);
	imlib_context_set_drawable(window.id);
	imlib_set_cache_size(window.cache_size);

	if (window.xshape) {
		window.mask = XCreatePixmap(window.display, window.id,
				window.width, window.height, 1);
		imlib_context_set_mask(window.mask);
	}

	handle_xevents();
	window.updates = imlib_update_append_rect(window.updates, 0, 0, window.width, window.height);
	scroll_and_update(0);

	xsg_main_add_update_func(scroll_and_update);
	attach_source(ConnectionNumber(window.display));
}

/******************************************************************************
 *
 * Line <x1> <y1> <x2> <y2> <color>
 *
 ******************************************************************************/

typedef struct {
	int x1;
	int y1;
	int x2;
	int y2;
	Imlib_Color color;
} line_t;

static void render_line(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	line_t *line;

	g_message("Render Line");

	line = (line_t *) widget->data;

	imlib_context_set_image(buffer);

	imlib_context_set_color(line->color.red, line->color.green,
			line->color.blue, line->color.alpha);

	imlib_image_draw_line(line->x1 - up_x, line->y1 - up_y,
			line->x2 - up_x, line->y2 - up_y, 0);
}

static void update_line(widget_t *widget, uint16_t var_id) {
	return;
}

static void scroll_line(widget_t *widget) {
	return;
}

void xsg_widgets_parse_line() {
	widget_t *widget;
	line_t *line;

	widget = g_new0(widget_t, 1);
	line = g_new(line_t, 1);

	line->x1 = xsg_conf_read_int();
	line->y1 = xsg_conf_read_int();
	line->x2 = xsg_conf_read_int();
	line->y2 = xsg_conf_read_int();
	line->color = uint2color(xsg_conf_read_color());
	xsg_conf_read_newline();


	widget->update = 0;
	widget->xoffset = MIN(line->x1, line->x2);
	widget->yoffset = MIN(line->y1, line->y2);
	widget->width = MAX(line->x1, line->x2) - widget->xoffset + 1;
	widget->height = MAX(line->y1, line->y2) - widget->yoffset + 1;
	widget->render_func = render_line;
	widget->update_func = update_line;
	widget->scroll_func = scroll_line;
	widget->data = (void *) line;

	widget_list = xsg_list_append(widget_list, widget);
}

/******************************************************************************
 *
 * Rectangle <x> <y> <width> <height> <color> [ColorRange <angle> <count> <distance> <color> ...] [Direction <angle>] [Filled]
 *
 ******************************************************************************/

typedef struct {
	angle_t *angle;
	Imlib_Color color;
	Imlib_Color_Range range;
	double range_angle;
	bool filled;
} rectangle_t;

static void render_rectangle(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	rectangle_t *rectangle;
	Imlib_Image tmp = NULL;
	int xoffset, yoffset;
	unsigned int width, height;

	g_message("Render Rectangle");

	rectangle = (rectangle_t *) widget->data;

	if (rectangle->angle) {
		xoffset = 0;
		yoffset = 0;
		width = rectangle->angle->width;
		height = rectangle->angle->height;
		tmp = imlib_create_image(width, height);
		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		image_set_color(window.background);
	} else {
		xoffset = widget->xoffset - up_x;
		yoffset = widget->yoffset - up_y;
		width = widget->width;
		height = widget->height;
		imlib_context_set_image(buffer);
	}

	if (rectangle->range) {
		imlib_context_set_color_range(rectangle->range);
		imlib_image_fill_color_range_rectangle(xoffset, yoffset,
				width, height, rectangle->range_angle);
	} else if (rectangle->filled) {
		imlib_context_set_color(rectangle->color.red, rectangle->color.green,
				rectangle->color.blue, rectangle->color.alpha);
		imlib_image_fill_rectangle(xoffset, yoffset, width, height);
	} else {
		imlib_context_set_color(rectangle->color.red, rectangle->color.green,
				rectangle->color.blue, rectangle->color.alpha);
		imlib_image_draw_rectangle(xoffset, yoffset, width, height);
	}

	if (rectangle->angle) {
		if (!blend_at_angle(buffer, tmp, rectangle->angle, up_x, up_y)) {
			imlib_context_set_image(buffer);
			imlib_blend_image_onto_image(tmp, 1, 0, 0,
					widget->width, widget->height,
					widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);
		}
		imlib_context_set_image(tmp);
		imlib_free_image();
	}
}

static void update_rectangle(widget_t *widget, uint16_t var_id) {
	return;
}

static void scroll_rectangle(widget_t *widget) {
	return;
}

void xsg_widgets_parse_rectangle() {
	widget_t *widget;
	rectangle_t *rectangle;

	widget = g_new0(widget_t, 1);
	rectangle = g_new(rectangle_t, 1);

	widget->update = 0;
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_rectangle;
	widget->update_func = update_rectangle;
	widget->scroll_func = scroll_rectangle;
	widget->data = (void *) rectangle;

	rectangle->angle = NULL;
	rectangle->color = uint2color(xsg_conf_read_color());
	rectangle->range = NULL;
	rectangle->range_angle = 0.0;
	rectangle->filled = FALSE;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;
			rectangle->range = imlib_create_color_range();
			imlib_context_set_color_range(rectangle->range);
			imlib_context_set_color(rectangle->color.red, rectangle->color.green,
					rectangle->color.blue, rectangle->color.alpha);
			imlib_add_color_to_color_range(0);
			rectangle->range_angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = uint2color(xsg_conf_read_color());
				imlib_context_set_color(color.red, color.green,
						color.blue, color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			rectangle->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Filled")) {
			rectangle->filled = TRUE;
		} else {
			xsg_conf_error("Skew, ColorRange or Filled");
		}
	}

	widget_list = xsg_list_append(widget_list, widget);
}

/******************************************************************************
 *
 * Ellipse <xc> <yc> <a> <b> <color> [Filled]
 *
 ******************************************************************************/

typedef struct {
	int xc;
	int yc;
	int a;
	int b;
	Imlib_Color color;
	bool filled;
} ellipse_t;

static void render_ellipse(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	ellipse_t *ellipse;

	g_message("Render Ellipse");

	ellipse = (ellipse_t *) widget->data;

	imlib_context_set_image(buffer);

	imlib_context_set_color(ellipse->color.red, ellipse->color.green,
			ellipse->color.blue, ellipse->color.alpha);

	if (ellipse->filled)
		imlib_image_fill_ellipse(ellipse->xc - up_x, ellipse->yc - up_y,
				ellipse->a, ellipse->b);
	else
		imlib_image_draw_ellipse(ellipse->xc - up_x, ellipse->yc - up_y,
				ellipse->a, ellipse->b);
}

static void update_ellipse(widget_t *widget, uint16_t var_id) {
	return;
}

static void scroll_ellipse(widget_t *widget) {
	return;
}

void xsg_widgets_parse_ellipse() {
	widget_t *widget;
	ellipse_t *ellipse;

	widget = g_new0(widget_t, 1);
	ellipse = g_new0(ellipse_t, 1);

	ellipse->xc = xsg_conf_read_int();
	ellipse->yc = xsg_conf_read_int();
	ellipse->a = xsg_conf_read_uint();
	ellipse->b = xsg_conf_read_uint();
	ellipse->color = uint2color(xsg_conf_read_color());
	ellipse->filled = FALSE;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Filled"))
			ellipse->filled = TRUE;
		else
			xsg_conf_error("Filled");
	}

	widget->update = 0;
	widget->xoffset = ellipse->xc - ellipse->a;
	widget->yoffset = ellipse->yc - ellipse->b;
	widget->width = ellipse->a * 2;
	widget->height = ellipse->b * 2;
	widget->render_func = render_ellipse;
	widget->update_func = update_ellipse;
	widget->scroll_func = scroll_ellipse;
	widget->data = (void *) ellipse;

	widget_list = xsg_list_append(widget_list, widget);
}

/******************************************************************************
 *
 * Polygon <color> <count> <x> <y> <x> <y> ... [Filled|Closed]
 *
 ******************************************************************************/

typedef struct {
	Imlib_Color color;
	ImlibPolygon polygon;
	bool filled;
	bool closed;
} polygon_t;

static void render_polygon(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	polygon_t *polygon;
	ImlibPolygon poly;

	g_message("Render Polygon");

	polygon = (polygon_t *) widget->data;

	imlib_context_set_image(buffer);

	imlib_context_set_color(polygon->color.red, polygon->color.green,
			polygon->color.blue, polygon->color.alpha);

	poly = poly_copy(polygon->polygon, -up_x, -up_y);

	if (polygon->filled)
		imlib_image_fill_polygon(poly);
	else
		imlib_image_draw_polygon(poly, polygon->closed);

	imlib_polygon_free(poly);
}

static void update_polygon(widget_t *widget, uint16_t var_id) {
	return;
}

static void scroll_polygon(widget_t *widget) {
	return;
}

void xsg_widgets_parse_polygon() {
	widget_t *widget;
	polygon_t *polygon;
	unsigned int count, i;
	int x1, y1, x2, y2;

	widget = g_new0(widget_t, 1);
	polygon = g_new0(polygon_t, 1);

	polygon->color = uint2color(xsg_conf_read_color());
	polygon->polygon = imlib_polygon_new();
	polygon->filled = FALSE;
	polygon->closed = FALSE;

	count = xsg_conf_read_uint();
	for (i = 0; i < count; i++) {
		int x, y;
		x = xsg_conf_read_int();
		y = xsg_conf_read_int();
		imlib_polygon_add_point(polygon->polygon, x, y);
	}

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Filled"))
			polygon->filled = TRUE;
		else if (xsg_conf_find_command("Closed"))
			polygon->closed = TRUE;
		else
			xsg_conf_error("Filled or Closed");
	}

	imlib_polygon_get_bounds(polygon->polygon, &x1, &y1, &x2, &y2);

	widget->update = 0;
	widget->xoffset = x1;
	widget->yoffset = y1;
	widget->width = x2 - x1;
	widget->height = y2 - y1;
	widget->render_func = render_polygon;
	widget->update_func = update_polygon;
	widget->scroll_func = scroll_polygon;
	widget->data = (void *) polygon;

	widget_list = xsg_list_append(widget_list, widget);;
}

/******************************************************************************
 *
 * Image <x> <y> <image> [Angle <angle>] [Scale <width> <height>]
 *
 ******************************************************************************/

typedef struct {
	angle_t *angle;
	char *filename;
	Imlib_Image image;
	bool scale;
} image_t;

static void render_image(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	image_t *image;

	g_message("Render Image");

	image = (image_t *) widget->data;

	imlib_context_set_image(buffer);

	if (image->angle)
		imlib_blend_image_onto_image_at_angle(image->image, 1, 0, 0,
				image->angle->width, image->angle->height,
				image->angle->xoffset - up_x, image->angle->yoffset - up_y,
				image->angle->angle_x, image->angle->angle_y);
	else
		imlib_blend_image_onto_image(image->image, 1, 0, 0,
				widget->width, widget->height, widget->xoffset - up_x,
				widget->yoffset - up_y, widget->width, widget->height);
}

static void update_image(widget_t *widget, uint16_t var_id) {
	return;
}

static void scroll_image(widget_t *widget) {
	return;
}

void xsg_widgets_parse_image() {
	widget_t *widget;
	image_t *image;
	double angle = 0.0;

	widget = g_new0(widget_t, 1);
	image = g_new0(image_t, 1);

	widget->update = 0;
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = 0;
	widget->height = 0;
	widget->render_func = render_image;
	widget->update_func = update_image;
	widget->scroll_func = scroll_image;
	widget->data = (void *) image;

	image->angle = NULL;
	image->filename = xsg_conf_read_string();
	image->image = NULL;
	image->scale = FALSE;

	image->image = load_image(image->filename, TRUE);
	imlib_context_set_image(image->image);

	widget->width = imlib_image_get_width();
	widget->height = imlib_image_get_height();

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Scale")) {
			image->scale = TRUE;
			widget->width = xsg_conf_read_uint();
			widget->height = xsg_conf_read_uint();
		} else {
			xsg_conf_error("Angle or Scale");
		}
	}

	if (image->scale) {
		image->image = imlib_create_cropped_scaled_image(0, 0, imlib_image_get_width(),
				imlib_image_get_height(), widget->width, widget->height);
		imlib_free_image();
	}

	if (angle != 0.0) {
		image->angle = parse_angle(angle, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);

		if (image->angle->angle == 90.0) {
			imlib_image_orientate(1);
			image->angle = NULL;
		} else if (image->angle->angle == 180.0) {
			imlib_image_orientate(2);
			image->angle = NULL;
		} else if (image->angle->angle == 270.0) {
			imlib_image_orientate(3);
			image->angle = NULL;
		}
	}

	widget_list = xsg_list_append(widget_list, widget);
}

/******************************************************************************
 *
 * BarChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Mask <image>]
 * + <variable> <color> [ColorRange <angle> <count> <distance> <color> ...] [Mult <mult>] [Add <add>] [AddPrev]
 *
 ******************************************************************************/

typedef struct {
	uint16_t var_id;
	Imlib_Color color;
	Imlib_Color_Range range;
	double angle;
	double mult;
	double add;
	bool add_prev;
	double value;
} barchart_var_t;

typedef struct {
	angle_t *angle;
	double min;
	double max;
	bool const_min;
	bool const_max;
	Imlib_Image mask;
	xsg_list *var_list;
} barchart_t;

static void render_barchart(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	barchart_t *barchart;
	barchart_var_t *barchart_var;
	double min, max;
	xsg_list *l;
	int clip_x, clip_y, clip_w, clip_h;
	int last_h = 0;
	double value;
	double pixel_h;
	Imlib_Image tmp;
	unsigned int width;
	unsigned int height;

	g_message("Render BarChart");

	barchart = (barchart_t *) widget->data;

	if (barchart->const_min) {
		min = barchart->min;
	} else {
		min = G_MAXDOUBLE;
		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;
			min = MIN(min, barchart_var->value);
		}
		if (min == G_MAXDOUBLE)
			return;
	}

	if (barchart->const_max) {
		max = barchart->max;
	} else {
		max = G_MINDOUBLE;
		for (l = barchart->var_list; l; l = l->next) {
			barchart_var = l->data;
			max = MAX(max, barchart_var->value);
		}
		if (max == G_MINDOUBLE)
			return;
	}

	if (barchart->angle) {
		width = barchart->angle->width;
		height = barchart->angle->height;
	} else {
		width = widget->width;
		height = widget->height;
	}

	tmp = imlib_create_image(width, height);
	imlib_context_set_image(tmp);
	imlib_image_set_has_alpha(1);
	image_set_color(0);

	pixel_h = (max - min) / (double) height;

	for (l = barchart->var_list; l; l = l->next) {
		barchart_var = l->data;
		value = barchart_var->value;
		if (isnan(value))
			continue;

		clip_x = 0;
		clip_y = last_h;
		clip_w = width;

		if (barchart_var->add_prev) {
			clip_h = last_h + llround(value / pixel_h);
			last_h = clip_h + clip_y;
		} else {
			clip_h = llround(value / pixel_h);
			last_h = clip_h;
		}

		clip_y = height - clip_h - clip_y;

		if ((clip_w <= 0) || (clip_h <= 0))
			continue;
		imlib_context_set_cliprect(clip_x, clip_y, clip_w, clip_h);
		if (barchart_var->range) {
			imlib_context_set_color_range(barchart_var->range);
			imlib_image_fill_color_range_rectangle(0, 0,
					width, height, barchart_var->angle);
		} else {
			imlib_context_set_color(barchart_var->color.red, barchart_var->color.green,
					barchart_var->color.blue, barchart_var->color.alpha);
			imlib_image_fill_rectangle(0, 0, width, height);
		}
	}

	imlib_context_set_cliprect(0, 0, 0, 0);
	imlib_context_set_image(tmp);

	if (barchart->mask)
		blend_mask(buffer, barchart->mask);

	if (barchart->angle) {
		if (!blend_at_angle(buffer, tmp, barchart->angle, up_x, up_y)) {
			imlib_context_set_image(buffer);
			imlib_blend_image_onto_image(tmp, 1, 0, 0,
					widget->width, widget->height,
					widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);
		}
	} else {
		imlib_context_set_image(buffer);
		imlib_blend_image_onto_image(tmp, 1, 0, 0, widget->width, widget->height,
				widget->xoffset - up_x, widget->yoffset - up_y,
				widget->width, widget->height);
	}

	imlib_context_set_image(tmp);
	imlib_free_image();
}

static void update_barchart(widget_t *widget, uint16_t var_id) {
	barchart_t *barchart;
	barchart_var_t *barchart_var;
	xsg_list *l;
	double prev = 0.0;

	barchart = (barchart_t *) widget->data;
	for (l = barchart->var_list; l; l = l->next) {
		barchart_var = l->data;

		if ((var_id == ALL_VARS) || (barchart_var->var_id == var_id)) {
			barchart_var->value = xsg_var_as_double(barchart_var->var_id);
			barchart_var->value *= barchart_var->mult;
			barchart_var->value += barchart_var->add;
			if (barchart_var->add_prev)
				barchart_var->value += prev;
		}
		prev = barchart_var->value;
	}
}

static void scroll_barchart(widget_t *widget) {
	return;
}

void xsg_widgets_parse_barchart(uint64_t *update, uint16_t *widget_id) {
	widget_t *widget;
	barchart_t *barchart;

	widget = g_new0(widget_t, 1);
	barchart = g_new0(barchart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_barchart;
	widget->update_func = update_barchart;
	widget->scroll_func = scroll_barchart;
	widget->data = (void *) barchart;

	barchart->angle = NULL;
	barchart->min = 0.0;
	barchart->max = 0.0;
	barchart->const_min = FALSE;
	barchart->const_max = FALSE;
	barchart->mask = NULL;
	barchart->var_list = NULL;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			barchart->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Min")) {
			barchart->min = xsg_conf_read_double();
			barchart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			barchart->max = xsg_conf_read_double();
			barchart->const_max = TRUE;
		} else if (xsg_conf_find_command("Mask")) {
			char *filename = xsg_conf_read_string();
			barchart->mask = load_image(filename, TRUE);
			g_free(filename);
		} else {
			xsg_conf_error("Angle, Min, Max or Mask");
		}
	}

	*update = widget->update;
	*widget_id = xsg_list_length(widget_list);

	widget_list = xsg_list_append(widget_list, widget);;
}

void xsg_widgets_parse_barchart_var(uint16_t var_id) {
	widget_t *widget;
	barchart_t *barchart;
	barchart_var_t *barchart_var;

	barchart_var = g_new0(barchart_var_t, 1);

	barchart_var->var_id = var_id;
	barchart_var->color = uint2color(xsg_conf_read_color());
	barchart_var->range = NULL;
	barchart_var->angle = 0.0;
	barchart_var->mult = 1.0;
	barchart_var->add = 0.0;
	barchart_var->add_prev = FALSE;
	barchart_var->value = nan("");

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;
			barchart_var->range = imlib_create_color_range();
			imlib_context_set_color_range(barchart_var->range);
			imlib_context_set_color(
					barchart_var->color.red,
					barchart_var->color.green,
					barchart_var->color.blue,
					barchart_var->color.alpha);
			imlib_add_color_to_color_range(0);
			barchart_var->angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = uint2color(xsg_conf_read_color());
				imlib_context_set_color(
						color.red,
						color.green,
						color.blue,
						color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("AddPrev")) {
			barchart_var->add_prev = TRUE;
		} else if (xsg_conf_find_command("Add")) {
			barchart_var->add = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Mult")) {
			barchart_var->mult = xsg_conf_read_double();
		} else {
			xsg_conf_error("ColorRange, AddPrev, Add or Mult");
		}
	}

	widget = xsg_list_last(widget_list)->data;
	barchart = widget->data;
	barchart->var_list = xsg_list_append(barchart->var_list, barchart_var);
}

/******************************************************************************
 *
 * LineChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Background <image>]
 * + <variable> <color> [Mult <mult>] [Add <add>] [AddPrev]
 *
 ******************************************************************************/

typedef struct {
	uint16_t var_id;
	Imlib_Color color;
	double mult;
	double add;
	bool add_prev;
	double *values;
} linechart_var_t;

typedef struct {
	angle_t *angle;
	double min;
	double max;
	bool const_min;
	bool const_max;
	Imlib_Image background;
	xsg_list *var_list;
	unsigned int value_index;
} linechart_t;

static void render_linechart(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	linechart_t *linechart;
	linechart_var_t *linechart_var;
	double min, max;
	xsg_list *l;
	Imlib_Image tmp;
	ImlibPolygon poly;
	double pixel_h;
	unsigned int width;
	unsigned int height;
	unsigned int i;

	g_message("Render LineChart");

	linechart = (linechart_t *) widget->data;

	if (linechart->angle) {
		width = linechart->angle->width;
		height = linechart->angle->height;
	} else {
		width = widget->width;
		height = widget->height;
	}

	if (linechart->const_min) {
		min = linechart->min;
	} else {
		min = G_MAXDOUBLE;
		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;
			for (i = 0; i < width; i++)
				min = MIN(min, linechart_var->values[i]);
		}
		if (min == G_MAXDOUBLE)
			return;
	}

	if (linechart->const_max) {
		max = linechart->max;
	} else {
		max = G_MINDOUBLE;
		for (l = linechart->var_list; l; l = l->next) {
			linechart_var = l->data;
			for (i = 0; i < width; i++)
				max = MAX(max, linechart_var->values[i]);
		}
		if (max == G_MINDOUBLE)
			return;
	}

	tmp = imlib_create_image(width, height);
	imlib_context_set_image(tmp);
	imlib_image_set_has_alpha(1);
	image_set_color(0);

	/* TODO background image */

	pixel_h = (max - min) / (double) height;

	for (l= linechart->var_list; l; l = l->next) {
		linechart_var = l->data;

		poly = imlib_polygon_new();

		for (i = 0; i < width; i++) {
			unsigned int j;
			int x, y;

			j = (i + linechart->value_index) % width;
			x = width - i;
			y = height - linechart_var->values[j] * pixel_h;

			imlib_polygon_add_point(poly, x, y); /* FIXME */
		}

		imlib_image_draw_polygon(poly, 0);

		imlib_polygon_free(poly);
	}

	if (linechart->angle) {
		if (!blend_at_angle(buffer, tmp, linechart->angle, up_x, up_y)) {
			imlib_context_set_image(buffer);
			imlib_blend_image_onto_image(tmp, 1, 0, 0,
					widget->width, widget->height,
					widget->xoffset - up_x, widget->yoffset - up_y,
					widget->width, widget->height);
		}
	} else {
		imlib_context_set_image(buffer);
		imlib_blend_image_onto_image(tmp, 1, 0, 0, widget->width, widget->height,
				widget->xoffset - up_x, widget->yoffset - up_y,
				widget->width, widget->height);
	}

	imlib_context_set_image(tmp);
	imlib_free_image();
}

static void update_linechart(widget_t *widget, uint16_t var_id) {
	linechart_t *linechart;
	linechart_var_t *linechart_var;
	xsg_list *l;
	double prev = 0.0;
	unsigned int i;

	linechart = (linechart_t *) widget->data;
	i = linechart->value_index;
	for (l = linechart->var_list; l; l = l->next) {
		linechart_var = l->data;

		if ((var_id == ALL_VARS) || (linechart_var->var_id == var_id)) {
			linechart_var->values[i] = xsg_var_as_double(linechart_var->var_id);
			linechart_var->values[i] *= linechart_var->mult;
			linechart_var->values[i] += linechart_var->add;
			if (linechart_var->add_prev)
				linechart_var->values[i] += prev;
		}
		prev = linechart_var->values[i];
	}
}

static void scroll_linechart(widget_t *widget) {
	linechart_t *linechart;
	unsigned int width;

	linechart = (linechart_t *) widget->data;

	if (linechart->angle)
		width = linechart->angle->width;
	else
		width = widget->width;

	linechart->value_index = (linechart->value_index + 1) % width;
}

void xsg_widgets_parse_linechart(uint64_t *update, uint16_t *widget_id) {
	widget_t *widget;
	linechart_t *linechart;

	widget = g_new0(widget_t, 1);
	linechart = g_new0(linechart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_linechart;
	widget->update_func = update_linechart;
	widget->scroll_func = scroll_linechart;
	widget->data = (void *) linechart;

	linechart->angle = NULL;
	linechart->min = 0.0;
	linechart->max = 0.0;
	linechart->const_min = FALSE;
	linechart->const_max = FALSE;
	linechart->background = NULL;
	linechart->var_list = NULL;
	linechart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			linechart->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Min")) {
			linechart->min = xsg_conf_read_double();
			linechart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			linechart->max = xsg_conf_read_double();
			linechart->const_max = TRUE;
		} else if (xsg_conf_find_command("Background")) {
			char *filename = xsg_conf_read_string();
			linechart->background = load_image(filename, TRUE);
			g_free(filename);
		} else {
			xsg_conf_error("Angle, Min, Max or Background");
		}
	}

	*update = widget->update;
	*widget_id = xsg_list_length(widget_list);

	widget_list = xsg_list_append(widget_list, widget);
}

void xsg_widgets_parse_linechart_var(uint16_t var_id) {
	widget_t *widget;
	linechart_t *linechart;
	linechart_var_t * linechart_var;
	unsigned int width, i;

	widget = xsg_list_last(widget_list)->data;
	linechart = widget->data;
	linechart_var = g_new0(linechart_var_t, 1);
	linechart->var_list = xsg_list_append(linechart->var_list, linechart_var);

	if (linechart->angle)
		width = linechart->angle->width;
	else
		width = widget->width;

	linechart_var->var_id = var_id;
	linechart_var->color = uint2color(xsg_conf_read_color());
	linechart_var->mult = 1.0;
	linechart_var->add = 0.0;
	linechart_var->add_prev = FALSE;
	linechart_var->values = g_new0(double, width);

	for (i = 0; i < width; i++)
		linechart_var->values[i] = nan("");

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("AddPrev")) {
			linechart_var->add_prev = TRUE;
		} else if (xsg_conf_find_command("Add")) {
			linechart_var->add = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Mult")) {
			linechart_var->mult = xsg_conf_read_double();
		} else {
			xsg_conf_error("AddPrev, Add or Mult");
		}
	}
}

/******************************************************************************
 *
 * AreaChart <update> <x> <y> <width> <height> [Angle <angle>] [Min <min>] [Max <max>] [Background <image>]
 * + <variable> <color> [ColorRange <angle> <count> <distance> <color> ...] [Top <height> <color>] [Mult <mult>] [Add <add>] [AddPrev]
 *
 ******************************************************************************/

typedef struct {
	uint16_t var_id;
	Imlib_Color color;
	Imlib_Color_Range range;
	double angle;
	unsigned int top_height;
	Imlib_Color top_color;
	double mult;
	double add;
	bool add_prev;
	double *values;
} areachart_var_t;

typedef struct {
	angle_t *angle;
	double min;
	double max;
	bool const_min;
	bool const_max;
	Imlib_Image background;
	xsg_list *var_list;
	unsigned int value_index;
} areachart_t;

static void render_areachart(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	g_message("Render AreaChart");
	/* TODO */
}

static void update_areachart(widget_t *widget, uint16_t var_id) {
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	xsg_list *l;
	double prev = 0.0;
	unsigned int i;

	areachart = (areachart_t *) widget->data;
	i = areachart->value_index;
	for (l = areachart->var_list; l ; l = l->next) {
		areachart_var = l->data;

		if ((var_id == ALL_VARS) || (areachart_var->var_id == var_id)) {
			areachart_var->values[i] = xsg_var_as_double(areachart_var->var_id);
			areachart_var->values[i] *= areachart_var->mult;
			areachart_var->values[i] += areachart_var->add;
			if (areachart_var->add_prev)
				areachart_var->values[i] += prev;
		}
		prev = areachart_var->values[i];
	}
}

static void scroll_areachart(widget_t *widget) {
	areachart_t *areachart;
	unsigned int width;

	areachart = (areachart_t *) widget->data;

	if (areachart->angle)
		width = areachart->angle->width;
	else
		width = widget->width;

	areachart->value_index = (areachart->value_index + 1) % width;
}

void xsg_widgets_parse_areachart(uint64_t *update, uint16_t *widget_id) {
	widget_t *widget;
	areachart_t *areachart;

	widget = g_new0(widget_t, 1);
	areachart = g_new(areachart_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_areachart;
	widget->update_func = update_areachart;
	widget->scroll_func = scroll_areachart;
	widget->data = (void *) areachart;

	areachart->angle = NULL;
	areachart->min = 0.0;
	areachart->max = 0.0;
	areachart->const_min = FALSE;
	areachart->const_max = FALSE;
	areachart->background = NULL;
	areachart->var_list = NULL;
	areachart->value_index = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			areachart->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Min")) {
			areachart->min = xsg_conf_read_double();
			areachart->const_min = TRUE;
		} else if (xsg_conf_find_command("Max")) {
			areachart->max = xsg_conf_read_double();
			areachart->const_max = TRUE;
		} else if (xsg_conf_find_command("Background")) {
			char *filename = xsg_conf_read_string();
			areachart->background = load_image(filename, TRUE);
			g_free(filename);
		} else {
			xsg_conf_error("Angle, Min, Max or Background");
		}
	}

	*update = widget->update;
	*widget_id = xsg_list_length(widget_list);

	widget_list = xsg_list_append(widget_list, widget);
}

void xsg_widgets_parse_areachart_var(uint16_t var_id) {
	widget_t *widget;
	areachart_t *areachart;
	areachart_var_t *areachart_var;
	unsigned int width, i;

	widget = xsg_list_last(widget_list)->data;
	areachart = widget->data;
	areachart_var = g_new0(areachart_var_t, 1);
	areachart->var_list = xsg_list_append(areachart->var_list, areachart_var);

	if (areachart->angle)
		width = areachart->angle->width;
	else
		width = widget->width;

	areachart_var->var_id = var_id;
	areachart_var->color = uint2color(xsg_conf_read_color());
	areachart_var->range = NULL;
	areachart_var->angle = 0.0;
	areachart_var->top_height = 0;
	areachart_var->mult = 1.0;
	areachart_var->add = 0.0;
	areachart_var->add_prev = FALSE;
	areachart_var->values = g_new0(double, width);

	for (i = 0; i < width; i++)
		areachart_var->values[i] = nan("");

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("ColorRange")) {
			unsigned int count, i;
			areachart_var->range = imlib_create_color_range();
			imlib_context_set_color_range(areachart_var->range);
			imlib_context_set_color(
					areachart_var->color.red,
					areachart_var->color.green,
					areachart_var->color.blue,
					areachart_var->color.alpha);
			imlib_add_color_to_color_range(0);
			areachart_var->angle = xsg_conf_read_double();
			count = xsg_conf_read_uint();
			for (i = 0; i < count; i++) {
				int distance;
				Imlib_Color color;
				distance = xsg_conf_read_uint();
				color = uint2color(xsg_conf_read_color());
				imlib_context_set_color(
						color.red,
						color.green,
						color.blue,
						color.alpha);
				imlib_add_color_to_color_range(distance);
			}
		} else if (xsg_conf_find_command("Top")) {
			areachart_var->top_height = xsg_conf_read_uint();
			areachart_var->top_color = uint2color(xsg_conf_read_color());
		} else if (xsg_conf_find_command("AddPrev")) {
			areachart_var->add_prev = TRUE;
		} else if (xsg_conf_find_command("Add")) {
			areachart_var->add = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Mult")) {
			areachart_var->mult = xsg_conf_read_double();
		} else {
			xsg_conf_error("ColorRange, Top, AddPrev, Add or Mult");
		}
	}
}

/******************************************************************************
 *
 * Text <update> <x> <y> <width> <height> <color> <font> <format> [Angle <angle>] [Alignment <alignment>] [TabWidth <width>]
 * + <variable> [Mult <mult>] [Add <add>]
 *
 ******************************************************************************/

typedef struct {
	Imlib_Color color;
	char *font;
	char *format;
	angle_t *angle;
	alignment_t alignment;
	unsigned int tab_width;
	xsg_list *var_list;
	GString *buffer;
} text_t;

static void render_text(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	g_message("Render Text");
	/* TODO */
}

static void update_text(widget_t *widget, uint16_t var_id) {
	text_t *text;
	text_var_t *text_var;
	xsg_list *l;

	text = (text_t *)widget->data;
	for (l = text->var_list; l; l = l->next) {
		text_var = l->data;

		if (text_var->type == 0)
			parse_format(text->format, text->var_list);

		if ((var_id == ALL_VARS) || (var_id == text_var->var_id)) {
			switch (text_var->type) {
				case XSG_INT:
					text_var->value.i = xsg_var_as_int(text_var->var_id);
					break;
				case XSG_DOUBLE:
					text_var->value.d = xsg_var_as_double(text_var->var_id);
					break;
				case XSG_STRING:
					text_var->value.s = xsg_var_as_string(text_var->var_id);
					break;
				default:
					g_error("Unexpected type");
			}
		}
	}

	string_format(text->buffer, text->format, text->var_list);
}

static void scroll_text(widget_t *widget) {
	return;
}

void xsg_widgets_parse_text(uint64_t *update, uint16_t *widget_id) {
	widget_t *widget;
	text_t *text;

	widget = g_new0(widget_t, 1);
	text = g_new0(text_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_text;
	widget->update_func = update_text;
	widget->scroll_func = scroll_text;
	widget->data = (void *) text;

	text->color = uint2color(xsg_conf_read_color());
	text->font = xsg_conf_read_string();
	text->format = xsg_conf_read_string();
	text->angle = NULL;
	text->alignment = CENTER; /* FIXME */
	text->tab_width = 0;
	text->var_list = NULL;
	text->buffer = g_string_new("");

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			text->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Alignment")) {
			text->alignment = parse_alignment();
		} else if (xsg_conf_find_command("TabWidth")) {
			text->tab_width = xsg_conf_read_uint();
		} else {
			xsg_conf_error("Angle, Alignment or TabWidth");
		}
	}

	*update = widget->update;
	*widget_id = xsg_list_length(widget_list);

	widget_list = xsg_list_append(widget_list, widget);
}

void xsg_widgets_parse_text_var(uint16_t var_id) {
	widget_t *widget;
	text_t *text;
	text_var_t *text_var;

	widget = xsg_list_last(widget_list)->data;
	text = widget->data;
	text_var = g_new0(text_var_t, 1);
	text->var_list = xsg_list_append(text->var_list, text_var);

	text_var->var_id = var_id;
	text_var->mult = 1.0;
	text_var->add = 0.0;
	text_var->type = 0;
	text_var->value.u = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Add")) {
			text_var->add = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Mult")) {
			text_var->mult = xsg_conf_read_double();
		} else {
			xsg_conf_error("Add or Mult");
		}
	}
}

/******************************************************************************
 *
 * ImageText <update> <x> <y> <width> <height> <filemask> <format> [Angle <angle>] [Alignment <alignment>] [TabWidth <width>]
 * + <variable> [Mult <mult>] [Add <add>]
 *
 ******************************************************************************/

typedef struct {
	char *file_mask;
	char *format;
	angle_t *angle;
	alignment_t alignment;
	unsigned int tab_width;
	xsg_list *var_list;
	GString *buffer;
} imagetext_t;

static void render_imagetext(widget_t *widget, Imlib_Image buffer, int up_x, int up_y, bool solid_bg) {
	g_message("Render ImageText");
	/* TODO */
}

static void update_imagetext(widget_t *widget, uint16_t var_id) {
	imagetext_t *imagetext;
	text_var_t *text_var;
	xsg_list *l;

	imagetext = (imagetext_t *)widget->data;
	for (l = imagetext->var_list; l; l = l->next) {
		text_var = l->data;

		if (text_var->type == 0)
			parse_format(imagetext->format, imagetext->var_list);

		if ((var_id == ALL_VARS) || (var_id == text_var->var_id)) {
			switch (text_var->type) {
				case XSG_INT:
					text_var->value.i = xsg_var_as_int(text_var->var_id);
					break;
				case XSG_DOUBLE:
					text_var->value.d = xsg_var_as_double(text_var->var_id);
					break;
				case XSG_STRING:
					text_var->value.s = xsg_var_as_string(text_var->var_id);
					break;
				default:
					g_error("Unexpected type");
			}
		}
	}

	string_format(imagetext->buffer, imagetext->format, imagetext->var_list);
}

static void scroll_imagetext(widget_t *widget) {
	return;
}

void xsg_widgets_parse_imagetext(uint64_t *update, uint16_t *widget_id) {
	widget_t *widget;
	imagetext_t *imagetext;

	widget = g_new0(widget_t, 1);
	imagetext = g_new0(imagetext_t, 1);

	widget->update = xsg_conf_read_uint();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_imagetext;
	widget->update_func = update_imagetext;
	widget->scroll_func = scroll_imagetext;
	widget->data = (void *) imagetext;

	imagetext->file_mask = xsg_conf_read_string();
	imagetext->format = xsg_conf_read_string();
	imagetext->angle = NULL;
	imagetext->alignment = CENTER; /* FIXME */
	imagetext->tab_width = 0;
	imagetext->var_list = NULL;
	imagetext->buffer = g_string_new("");

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Angle")) {
			double a = xsg_conf_read_double();
			imagetext->angle = parse_angle(a, widget->xoffset, widget->yoffset,
					&widget->width, &widget->height);
		} else if (xsg_conf_find_command("Alignment")) {
			imagetext->alignment = parse_alignment();
		} else if (xsg_conf_find_command("TabWidth")) {
			imagetext->tab_width = xsg_conf_read_uint();
		} else {
			xsg_conf_error("Angle, Alignment or TabWidth");
		}
	}

	*update = widget->update;
	*widget_id = xsg_list_length(widget_list);

	widget_list = xsg_list_append(widget_list, widget);
}

void xsg_widgets_parse_imagetext_var(uint16_t var_id) {
	widget_t *widget;
	imagetext_t *imagetext;
	text_var_t *text_var;

	widget = xsg_list_last(widget_list)->data;
	imagetext = widget->data;
	text_var = g_new0(text_var_t, 1);
	imagetext->var_list = xsg_list_append(imagetext->var_list, text_var);

	text_var->var_id = var_id;
	text_var->mult = 1.0;
	text_var->add = 0.0;
	text_var->type = 0;
	text_var->value.u = 0;

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Add")) {
			text_var->add = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Mult")) {
			text_var->mult = xsg_conf_read_double();
		} else {
			xsg_conf_error("Add or Mult");
		}
	}
}

