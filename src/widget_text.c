/* widget_text.c
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
#include <string.h>
#include <alloca.h>

#include "widgets.h"
#include "widget.h"
#include "window.h"
#include "angle.h"
#include "imlib.h"
#include "printf.h"
#include "conf.h"
#include "var.h"

/******************************************************************************/

typedef enum {
	TOP_LEFT      = 1 << 0,
	TOP_CENTER    = 1 << 1,
	TOP_RIGHT     = 1 << 2,
	CENTER_LEFT   = 1 << 3,
	CENTER        = 1 << 4,
	CENTER_RIGHT  = 1 << 5,
	BOTTOM_LEFT   = 1 << 6,
	BOTTOM_CENTER = 1 << 7,
	BOTTOM_RIGHT  = 1 << 8,
	TOP           = TOP_LEFT    | TOP_CENTER    | TOP_RIGHT,
	Y_CENTER      = CENTER_LEFT | CENTER        | CENTER_RIGHT,
	BOTTOM        = BOTTOM_LEFT | BOTTOM_CENTER | BOTTOM_RIGHT,
	LEFT          = TOP_LEFT    | CENTER_LEFT   | BOTTOM_LEFT,
	X_CENTER      = TOP_CENTER  | CENTER        | BOTTOM_CENTER,
	RIGHT         = TOP_RIGHT   | CENTER_RIGHT  | BOTTOM_RIGHT
} alignment_t;

/******************************************************************************/

typedef struct {
	Imlib_Color color;
	Imlib_Font font;
	char *font_name;
	xsg_printf_t *print;
	xsg_angle_t *angle;
	alignment_t alignment;
	unsigned int tab_width;
	xsg_string_t *string;
	int space_advance;
	int line_advance;
} text_t;

/******************************************************************************/

static char *
copy_substr(char *s, char *buf)
{
	while (*s != '\t' && *s != '\n' && *s != '\0') {
		*buf = *s;
		buf++;
		s++;
	}

	*buf = '\0';

	return s;
}

/******************************************************************************/

static void
render_text(xsg_widget_t *widget, Imlib_Image buffer, int up_x, int up_y)
{
	unsigned line_count = 1;
	text_t *text;
	char *buf;
	char *s;

	xsg_debug("%s: render Text: x=%d, y=%d, w=%u, h=%u",
			xsg_window_get_config_name(widget->window),
			widget->xoffset, widget->yoffset,
			widget->width, widget->height);

	text = widget->data;

	buf = (char *) alloca(text->string->len + 1);

	s = text->string->str - 1;

	/* count lines */
	if (text->alignment & Y_CENTER || text->alignment & BOTTOM) {
		char *str = text->string->str;

		while (*str != '\0') {
			if (*str == '\n') {
				line_count++;
			}
			str++;
		}
	}

	imlib_context_set_font(text->font);

	imlib_context_set_color(text->color.red, text->color.green,
			text->color.blue, text->color.alpha);

	if ((text->angle == NULL) || (text->angle->angle == 0.0)) {
		int line_y = 0;

		imlib_context_set_image(buffer);

		imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);

		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		if (text->alignment & TOP) {
			line_y = widget->yoffset - up_y;
		} else if (text->alignment & Y_CENTER) {
			line_y = widget->yoffset - up_y
				+ ((int) ((int) widget->height
					- (text->line_advance * line_count)) / 2);
		} else if (text->alignment & BOTTOM) {
			line_y = widget->yoffset - up_y
				+ ((int) widget->height
					- (text->line_advance * line_count));
		} else {
			xsg_error("unknown alignment: %x", text->alignment);
		}

		do {
			int xoffset = 0;
			int width = 0;

			if (text->alignment & X_CENTER || text->alignment & RIGHT) {
				char *begin_of_line = s;

				do {
					int column_advance;

					s++;

					s = copy_substr(s, buf);

					if (*buf != '\0') {
						imlib_get_text_advance(buf,
							&column_advance, NULL);
					}

					width += column_advance;

					if (*s == '\t') {
						width += text->space_advance;

						if (text->tab_width > 0) {
							width += text->tab_width
								- (width % text->tab_width);
						}
					}
				} while (*s == '\t');

				s = begin_of_line;
			}

			if (text->alignment & LEFT) {
				xoffset = widget->xoffset - up_x;
			} else if (text->alignment & X_CENTER) {
				xoffset = widget->xoffset - up_x
					+ (((int) widget->width - width) / 2);
			} else if (text->alignment & RIGHT) {
				xoffset = widget->xoffset - up_x
					+ ((int) widget->width - width);
			} else {
				xsg_error("unknown alignment: %x",
						text->alignment);
			}

			width = 0;

			do {
				int column_advance;

				s++;

				s = copy_substr(s, buf);

				if (*buf != '\0') {
					xsg_imlib_text_draw_with_return_metrics(
						xoffset + width,
						line_y, buf,
						NULL, NULL,
						&column_advance, NULL);
				}

				width += column_advance;

				if (*s == '\t') {
					width += text->space_advance;

					if (text->tab_width > 0) {
						width += text->tab_width
							- (width % text->tab_width);
					}
				}
			} while (*s == '\t');

			line_y += text->line_advance;

		} while (*s == '\n');

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (text->angle->angle == 90.0) {
		int line_x = 0;

		imlib_context_set_image(buffer);

		imlib_context_set_direction(IMLIB_TEXT_TO_DOWN);

		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		if (text->alignment & TOP) {
			line_x = widget->xoffset - up_x + widget->width
				- text->line_advance;
		} else if (text->alignment & Y_CENTER) {
			line_x = widget->xoffset - up_x + widget->width
				- text->line_advance - ((int) ((int) widget->width
					- (text->line_advance * line_count)) / 2);
		} else if (text->alignment & BOTTOM) {
			line_x = widget->xoffset - up_x + widget->width
				- text->line_advance - ((int) widget->width
					- (text->line_advance * line_count));
		} else {
			xsg_error("unknown alignment: %x", text->alignment);
		}

		do {
			int yoffset = 0;
			int height = 0;

			if (text->alignment & X_CENTER || text->alignment & RIGHT) {
				char *begin_of_line = s;

				do {
					int column_advance;

					if (*buf != '\0') {
						imlib_get_text_advance(buf,
							&column_advance, NULL);
					}

					height += column_advance;

					if (*s == '\t') {
						height += text->space_advance;

						if (text->tab_width > 0) {
							height += text->tab_width
								- (height % text->tab_width);
						}
					}
				} while (*s == '\t');

				s = begin_of_line;
			}

			if (text->alignment & LEFT) {
				yoffset = widget->yoffset - up_y;
			} else if (text->alignment & X_CENTER) {
				yoffset = widget->yoffset - up_y
					+ (((int) widget->height - height) / 2);
			} else if (text->alignment & RIGHT) {
				yoffset = widget->yoffset - up_y
					+ ((int) widget->height - height);
			} else {
				xsg_error("unknown alignment: %x",
						text->alignment);
			}

			height = 0;

			do {
				int column_advance;

				s++;

				s = copy_substr(s, buf);

				if (*buf != '\0') {
					xsg_imlib_text_draw_with_return_metrics(line_x,
						yoffset + height, buf,
						NULL, NULL, NULL,
						&column_advance);
				}

				height += column_advance;

				if (*s == '\t') {
					height += text->space_advance;

					if (text->tab_width > 0) {
						height += text->tab_width
							- (height % text->tab_width);
					}
				}
			} while (*s == '\t');

			line_x -= text->line_advance;
		} while (*s == '\n');

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (text->angle->angle == 180.0) {
		int line_y = 0;

		imlib_context_set_image(buffer);

		imlib_context_set_direction(IMLIB_TEXT_TO_LEFT);

		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		if (text->alignment & TOP) {
			line_y = widget->yoffset - up_y + widget->height
				- text->line_advance;
		} else if (text->alignment & Y_CENTER) {
			line_y = widget->yoffset - up_y + widget->height
				- text->line_advance - ((int) ((int) widget->height
					- (text->line_advance * line_count)) / 2);
		} else if (text->alignment & BOTTOM) {
			line_y = widget->yoffset - up_y + widget->height
				- text->line_advance - ((int) widget->height
					- (text->line_advance * line_count));
		} else {
			xsg_error("unknown alignment: %x", text->alignment);
		}

		do {
			int xoffset = 0;
			int width = 0;

			if (text->alignment & X_CENTER || text->alignment & LEFT) {
				char *begin_of_line = s;

				do {
					int column_advance;

					s++;

					s = copy_substr(s, buf);

					if (*buf != '\0') {
						imlib_get_text_advance(buf,
							&column_advance, NULL);
					}

					width += column_advance;

					if (*s == '\t') {
						width += text->space_advance;

						if (text->tab_width > 0) {
							width += text->tab_width
								- (width % text->tab_width);
						}
					}
				} while (*s == '\t');

				s = begin_of_line;
			}

			if (text->alignment & LEFT) {
				xoffset = widget->xoffset - up_x
					+ ((int) widget->width - width);
			} else if (text->alignment & X_CENTER) {
				xoffset = widget->xoffset - up_x
					+ (((int) widget->width - width) / 2);
			} else if (text->alignment & RIGHT) {
				xoffset = widget->xoffset - up_x;
			} else {
				xsg_error("unknown alignment: %x",
						text->alignment);
			}

			width = 0;

			do {
				int column_advance;

				s++;

				s = copy_substr(s, buf);

				if (*buf != '\0') {
					xsg_imlib_text_draw_with_return_metrics(xoffset
						+ width, line_y, buf,
						NULL, NULL, &column_advance,
						NULL);
				}

				width += column_advance;

				if (*s == '\t') {
					width += text->space_advance;

					if (text->tab_width > 0) {
						width += text->tab_width
							- (width % text->tab_width);
					}
				}
			} while (*s == '\t');

			line_y -= text->line_advance;

		} while (*s == '\n');

		imlib_context_set_cliprect(0, 0, 0, 0);

	} else if (text->angle->angle == 270.0) {
		int line_x = 0;

		imlib_context_set_image(buffer);

		imlib_context_set_direction(IMLIB_TEXT_TO_UP);

		imlib_context_set_cliprect(widget->xoffset - up_x,
				widget->yoffset - up_y,
				widget->width, widget->height);

		if (text->alignment & TOP) {
			line_x = widget->xoffset - up_x;
		} else if (text->alignment & Y_CENTER) {
			line_x = widget->xoffset - up_x
				+ ((int) ((int) widget->width
					- (text->line_advance * line_count)) / 2);
		} else if (text->alignment & BOTTOM) {
			line_x = widget->xoffset - up_x
				+ ((int) widget->width
					- (text->line_advance * line_count));
		} else {
			xsg_error("unknown alignment: %x", text->alignment);
		}

		do {
			int yoffset = 0;
			int height = 0;

			if (text->alignment & X_CENTER || text->alignment & LEFT) {
				char *begin_of_line = s;

				do {
					int column_advance;

					s++;

					s = copy_substr(s, buf);

					if (*buf != '\0') {
						imlib_get_text_advance(buf,
							&column_advance, NULL);
					}

					height += column_advance;

					if (*s == '\t') {
						height += text->space_advance;

						if (text->tab_width > 0) {
							height += text->tab_width
								- (height % text->tab_width);
						}
					}
				} while (*s == '\t');

				s = begin_of_line;
			}

			if (text->alignment & LEFT) {
				yoffset = widget->yoffset - up_y
					+ ((int) widget->height - height);
			} else if (text->alignment & X_CENTER) {
				yoffset = widget->yoffset - up_y
					+ (((int) widget->height - height) / 2);
			} else if (text->alignment & RIGHT) {
				yoffset = widget->yoffset - up_y;
			} else {
				xsg_error("unknown alignment: %x",
						text->alignment);
			}

			height = 0;

			do {
				int column_advance;

				s++;

				s = copy_substr(s, buf);

				if (*buf != '\0') {
					xsg_imlib_text_draw_with_return_metrics(line_x,
						yoffset + height, buf,
						NULL, NULL, NULL,
						&column_advance);
				}

				height += column_advance;

				if (*s == '\t') {
					height += text->space_advance;

					if (text->tab_width > 0) {
						height += text->tab_width
							- (height % text->tab_width);
					}
				}
			} while (*s == '\t');

			line_x += text->line_advance;

		} while (*s == '\n');

		imlib_context_set_cliprect(0, 0, 0, 0);
	} else {
		Imlib_Image tmp;
		int line_y = 0;

		tmp = imlib_create_image(text->angle->width,
				text->angle->height);

		imlib_context_set_image(tmp);
		imlib_image_set_has_alpha(1);
		imlib_image_clear();

		imlib_context_set_direction(IMLIB_TEXT_TO_RIGHT);

		if (text->alignment & TOP) {
			line_y = 0;
		} else if (text->alignment & Y_CENTER) {
			line_y = (int) ((int) text->angle->height
					- (text->line_advance * line_count)) / 2;
		} else if (text->alignment & BOTTOM) {
			line_y = (int) text->angle->height
				- (text->line_advance * line_count);
		} else {
			xsg_error("unknown alignment: %x", text->alignment);
		}

		do {
			int xoffset = 0;
			int width = 0;

			if (text->alignment & X_CENTER || text->alignment & RIGHT) {
				char *begin_of_line = s;

				do {
					int column_advance;

					s++;

					s = copy_substr(s, buf);

					if (*buf != '\0') {
						imlib_get_text_advance(buf,
							&column_advance, NULL);
					}

					width += column_advance;

					if (*s == '\t') {
						width += text->space_advance;

						if (text->tab_width > 0) {
							width += text->tab_width
								- (width % text->tab_width);
						}
					}
				} while (*s == '\t');

				s = begin_of_line;
			}

			if (text->alignment & LEFT) {
				xoffset = 0;
			} else if (text->alignment & X_CENTER) {
				xoffset = ((int) text->angle->width - width)
					/ 2;
			} else if (text->alignment & RIGHT) {
				xoffset = (int) text->angle->width - width;
			} else {
				xsg_error("unknown alignment: %x",
						text->alignment);
			}

			width = 0;

			do {
				int column_advance;

				s++;

				s = copy_substr(s, buf);

				if (*buf != '\0') {
					xsg_imlib_text_draw_with_return_metrics(xoffset
						+ width, line_y, buf,
						NULL, NULL, &column_advance,
						NULL);
				}

				width += column_advance;

				if (*s == '\t') {
					width += text->space_advance;

					if (text->tab_width > 0) {
						width += text->tab_width
							- (width % text->tab_width);
					}
				}
			} while (*s == '\t');

			line_y += text->line_advance;

		} while (*s == '\n');

		imlib_context_set_image(buffer);

		imlib_blend_image_onto_image_at_angle(tmp, 1, 0, 0,
				text->angle->width, text->angle->height,
				text->angle->xoffset - up_x,
				text->angle->yoffset - up_y,
				text->angle->angle_x, text->angle->angle_y);

		imlib_context_set_image(tmp);
		imlib_free_image();
	}
}

static void
update_text(xsg_widget_t *widget, xsg_var_t *var)
{
	text_t *text;
	char *s;

	text = widget->data;

	s = xsg_printf(text->print, var);

	if (!s) {
		return;
	}

	if (!strcmp(s, text->string->str)) {
		return;
	}

	xsg_string_assign(text->string, s);

	xsg_window_update_append_rect(widget->window, widget->xoffset,
			widget->yoffset, widget->width, widget->height);
}

static void
scroll_text(xsg_widget_t *widget)
{
	return;
}

/******************************************************************************/

void
xsg_widget_text_parse(xsg_window_t *window)
{
	xsg_widget_t *widget;
	text_t *text;
	double angle = 0.0;

	widget = xsg_widgets_new(window);

	text = xsg_new(text_t, 1);

	widget->update = xsg_conf_read_update();
	widget->xoffset = xsg_conf_read_int();
	widget->yoffset = xsg_conf_read_int();
	widget->width = xsg_conf_read_uint();
	widget->height = xsg_conf_read_uint();
	widget->render_func = render_text;
	widget->update_func = update_text;
	widget->scroll_func = scroll_text;
	widget->data = (void *) text;

	xsg_imlib_uint2color(xsg_conf_read_color(), &text->color);
	text->font = NULL;
	text->font_name = xsg_conf_read_string();
	text->print = xsg_printf_new(xsg_conf_read_string());
	text->angle = NULL;
	text->alignment = TOP_LEFT;
	text->tab_width = 0;
	text->string = xsg_string_new(NULL);

	text->font = imlib_load_font(text->font_name);

	if (unlikely(text->font == NULL)) {
		xsg_conf_error("cannot load font: \"%s\"", text->font_name);
	}

	imlib_context_set_font(text->font);

	imlib_get_text_advance(" ", &text->space_advance, &text->line_advance);

	if (unlikely(text->line_advance < 1)) {
		xsg_conf_error("font too small: line_advance must be greater than 0");
	}
	if (unlikely(text->space_advance < 1)) {
		xsg_conf_error("font too small: space_advance must be greater than 0");
	}

	while (!xsg_conf_find_newline()) {
		if (xsg_conf_find_command("Visible")) {
			widget->visible_update = xsg_conf_read_update();
			widget->visible_var = xsg_var_parse_num(
					widget->visible_update, window, widget);
		} else if (xsg_conf_find_command("Angle")) {
			angle = xsg_conf_read_double();
		} else if (xsg_conf_find_command("Alignment")) {
			if (xsg_conf_find_command("TopLeft")) {
				text->alignment = TOP_LEFT;
			} else if (xsg_conf_find_command("TopCenter")) {
				text->alignment = TOP_CENTER;
			} else if (xsg_conf_find_command("TopRight")) {
				text->alignment = TOP_RIGHT;
			} else if (xsg_conf_find_command("CenterLeft")) {
				text->alignment = CENTER_LEFT;
			} else if (xsg_conf_find_command("Center")) {
				text->alignment = CENTER;
			} else if (xsg_conf_find_command("CenterRight")) {
				text->alignment = CENTER_RIGHT;
			} else if (xsg_conf_find_command("BottomLeft")) {
				text->alignment = BOTTOM_LEFT;
			} else if (xsg_conf_find_command("BottomCenter")) {
				text->alignment = BOTTOM_CENTER;
			} else if (xsg_conf_find_command("BottomRight")) {
				text->alignment = BOTTOM_RIGHT;
			} else {
				xsg_conf_error("TopLeft, TopCenter, TopRight, "
						"CenterLeft, Center, "
						"CenterRight, BottomLeft, "
						"BottomCenter or BottomRight "
						"expected");
			}
		} else if (xsg_conf_find_command("TabWidth")) {
			text->tab_width = xsg_conf_read_uint();
		} else {
			xsg_conf_error("Visible, Angle, Alignment or TabWidth "
					"expected");
		}
	}

	if (angle != 0.0) {
		text->angle = xsg_angle_parse(angle, widget->xoffset,
				widget->yoffset, widget->width, widget->height);
	}

	while (xsg_conf_find_command("+")) {
		xsg_var_t *var;

		if (xsg_printf_next_var_is_string(text->print)) {
			var = xsg_var_parse_str(widget->update, window, widget);
		} else {
			var = xsg_var_parse_num(widget->update, window, widget);
		}

		xsg_printf_add_var(text->print, var);
		xsg_conf_read_newline();
	}
}

