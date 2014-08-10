/* See LICENSE for licence details. */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xlocale.h>

#define XK_NO_MOD UINT_MAX

static const char *im_font = "-*-*-medium-r-*-*-16-*-*-*-*-*-*-*";

enum { /* default terminal size */
    TERM_WIDTH  = 640,
    TERM_HEIGHT = 384,
};

struct keydef {
	KeySym k;
	unsigned int mask;
	char s[BUFSIZE];
};

const struct keydef key[] = {
    { XK_BackSpace, XK_NO_MOD, "\177"    },
    { XK_Up,        Mod1Mask,  "\033\033[A"},
    { XK_Up,        XK_NO_MOD, "\033[A"  },
    { XK_Down,      Mod1Mask,  "\033\033[B"},
    { XK_Down,      XK_NO_MOD, "\033[B"  },
    { XK_Right,     Mod1Mask,  "\033\033[C"},
    { XK_Right,     XK_NO_MOD, "\033[C"  },
    { XK_Left,      Mod1Mask,  "\033\033[D"},
    { XK_Left,      XK_NO_MOD, "\033[D"  },
    { XK_Begin,     XK_NO_MOD, "\033[G"  },
    { XK_Home,      XK_NO_MOD, "\033[1~" },
    { XK_Insert,    XK_NO_MOD, "\033[2~" },
    { XK_Delete,    XK_NO_MOD, "\033[3~" },
    { XK_End,       XK_NO_MOD, "\033[4~" },
    { XK_Prior,     XK_NO_MOD, "\033[5~" },
    { XK_Next,      XK_NO_MOD, "\033[6~" },
    { XK_F1,        XK_NO_MOD, "\033[[A" },
    { XK_F2,        XK_NO_MOD, "\033[[B" },
    { XK_F3,        XK_NO_MOD, "\033[[C" },
    { XK_F4,        XK_NO_MOD, "\033[[D" },
    { XK_F5,        XK_NO_MOD, "\033[[E" },
    { XK_F6,        XK_NO_MOD, "\033[17~"},
    { XK_F7,        XK_NO_MOD, "\033[18~"},
    { XK_F8,        XK_NO_MOD, "\033[19~"},
    { XK_F9,        XK_NO_MOD, "\033[20~"},
    { XK_F10,       XK_NO_MOD, "\033[21~"},
    { XK_F11,       XK_NO_MOD, "\033[23~"},
    { XK_F12,       XK_NO_MOD, "\033[24~"},
    { XK_F13,       XK_NO_MOD, "\033[25~"},
    { XK_F14,       XK_NO_MOD, "\033[26~"},
    { XK_F15,       XK_NO_MOD, "\033[28~"},
    { XK_F16,       XK_NO_MOD, "\033[29~"},
    { XK_F17,       XK_NO_MOD, "\033[31~"},
    { XK_F18,       XK_NO_MOD, "\033[32~"},
    { XK_F19,       XK_NO_MOD, "\033[33~"},
    { XK_F20,       XK_NO_MOD, "\033[34~"},
};

/* not assigned:
    kcbt=\E[Z,kmous=\E[M,kspd=^Z,
*/

struct paste_t {
	Atom target;
	struct point_t begin, end, current;
	int prev;
	bool copy_cursor_visible;
	char *str;
};

struct xwindow {
    Display *display;
    Window window;
    Pixmap pixbuf;
    GC gc;
	int width, height;
    int screen;
	XIM im;
	XIC ic;
	XIMStyle input_style;
	struct paste_t paste;
};

void print_style(XIMStyle style)
{
    if (style & XIMPreeditCallbacks)
        fprintf(stderr, "XIMPreeditCallbacks\n");
    else if (style & XIMPreeditPosition)
        fprintf(stderr, "XIMPreeditPosition\n");
    else if (style & XIMPreeditArea)
        fprintf(stderr, "XIMPreeditArea\n");
    else if (style & XIMPreeditNothing)
        fprintf(stderr, "XIMPreeditNothing\n");
    else
        fprintf(stderr, "XIMPreeditNone\n");

    if (style & XIMStatusCallbacks)
        fprintf(stderr, "XIMStatusCallbacks\n");
    else if (style & XIMStatusArea)
        fprintf(stderr, "XIMStatusArea\n");
    else if (style & XIMStatusNothing)
        fprintf(stderr, "XIMStatusNothing\n");
    else
        fprintf(stderr, "XIMStatusNone\n");
	
	fprintf(stderr, "\n");
}

XIMStyle select_better_style(XIMStyle im_style, XIMStyle best_style)
{
	XIMStyle i, b;
	XIMStyle preedit, status;

	preedit = XIMPreeditArea | XIMPreeditCallbacks |
		XIMPreeditPosition | XIMPreeditNothing | XIMPreeditNone;
	status = XIMStatusArea | XIMStatusCallbacks |
		XIMStatusNothing | XIMStatusNone;

	/* if one style == 0, select another */
	if (im_style == 0)
		return best_style;

	if (best_style == 0)
		return im_style;

	/* im_style and best_style are the same */
	if ((im_style & (preedit | status)) == (best_style & (preedit | status)))
		return im_style;

	/* ok, now we select better style */
	i = im_style & preedit;
	b = best_style & preedit;

	if (i != b) { /* different preedit style */
		if ((i | b) & XIMPreeditCallbacks)
			return (i == XIMPreeditCallbacks) ? im_style: best_style;
		else if ((i | b) & XIMPreeditPosition)
			return (i == XIMPreeditPosition) ? im_style: best_style;
		else if ((i | b) & XIMPreeditArea)
			return (i == XIMPreeditArea) ? im_style: best_style;
		else if ((i | b) & XIMPreeditNothing)
			return (i == XIMPreeditNothing) ? im_style: best_style;
		else
			return 0;
	} else { /* if preedit flags are the same, compare status flags */
		i = im_style & status;
		b = best_style & status;

		if ((i | b) & XIMStatusCallbacks)
			return (i == XIMStatusCallbacks) ? im_style: best_style;
		else if ((i | b) & XIMStatusArea)
			return (i == XIMStatusArea) ? im_style: best_style;
		else if ((i | b) & XIMStatusNothing)
			return (i == XIMStatusNothing) ? im_style: best_style;
		else
			return 0;
	}
}

void im_init(struct xwindow *xw)
{
	int i;
	long im_event_mask;
	int num_missing_charsets = 0;
	char **missing_charsets;
	char *default_string;
	XFontSet fontset;
	XVaNestedList list;
	XIMStyles *im_styles;
	XIMStyle app_styles, style;
	XPoint xpoint = {.x = 0, .y = 0};

	if ((xw->im = XOpenIM(xw->display, NULL, NULL, NULL)) == NULL)
		fatal("couldn't open input method");

	/* check available input style */
	app_styles = XIMPreeditNone | XIMPreeditNothing | XIMPreeditPosition
		| XIMStatusNone | XIMStatusNothing;

	XGetIMValues(xw->im, XNQueryInputStyle, &im_styles, NULL);
	xw->input_style = 0;

	if (DEBUG)
		fprintf(stderr, "im supported styles:\n");

	for (i = 0; i < im_styles->count_styles; i++) {
		style = im_styles->supported_styles[i];
		if (DEBUG)
			print_style(style);

		if ((style & app_styles) == style)
			xw->input_style = select_better_style(style, xw->input_style);
	}
	if (xw->input_style == 0)
		fatal("couldn't get valid input method style");

	XFree(im_styles);

	if (DEBUG) {
		fprintf(stderr, "input style:\n");
		print_style(xw->input_style);
	}

	fontset = XCreateFontSet(xw->display, im_font,
		&missing_charsets, &num_missing_charsets, &default_string);
	list = XVaCreateNestedList(0, XNSpotLocation, &xpoint,
		XNForeground, color_list[DEFAULT_FG],
		XNBackground, color_list[DEFAULT_BG],
		XNFontSet, fontset, NULL);

	if ((xw->ic = XCreateIC(xw->im, XNInputStyle, xw->input_style,
		XNPreeditAttributes, list, XNClientWindow, xw->window, NULL)) == NULL)
		fatal("couldn't create ic");
	XFree(list);

	XGetICValues(xw->ic, XNFilterEvents, &im_event_mask, NULL);
	XSelectInput(xw->display, xw->window, im_event_mask
		| FocusChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask
		| PointerMotionMask | ButtonPressMask | ButtonReleaseMask);
	XSetICFocus(xw->ic);
}

void init_paste(struct xwindow *xw)
{
	struct paste_t *paste = &xw->paste;

	paste->begin.x   = paste->begin.y   = 0;
	paste->end.x     = paste->end.y     = 0;
	paste->current.x = paste->current.y = 0;
	paste->prev = -1;

	paste->copy_cursor_visible = false;
	paste->str = NULL;
}

void xw_init(struct xwindow *xw)
{
	XTextProperty xtext = {.value = (unsigned char *) "yaftx",
		.encoding = XA_STRING, .format = 8, .nitems = 5};

	if ((xw->display = XOpenDisplay(NULL)) == NULL)
		fatal("XOpenDisplay failed");

	if (!XSupportsLocale())
		fatal("X does not support locale\n");

	if (XSetLocaleModifiers("") == NULL)
		fatal("cannot set locale modifiers");

	xw->screen = DefaultScreen(xw->display);
	xw->window = XCreateSimpleWindow(xw->display, DefaultRootWindow(xw->display),
		0, 0, TERM_WIDTH, TERM_HEIGHT, 0, color_list[DEFAULT_FG], color_list[DEFAULT_BG]);
	XSetWMProperties(xw->display, xw->window, &xtext, NULL, NULL, 0, NULL, NULL, NULL);

	xw->gc = XCreateGC(xw->display, xw->window, 0, NULL);
	XSetGraphicsExposures(xw->display, xw->gc, False);

	xw->width = DisplayWidth(xw->display, xw->screen);
	xw->height = DisplayHeight(xw->display, xw->screen);
	xw->pixbuf = XCreatePixmap(xw->display, xw->window,
		xw->width, xw->height, XDefaultDepth(xw->display, xw->screen));

	xw->paste.target = XInternAtom(xw->display, "UTF8_STRING", 0);
	if(xw->paste.target == None)
		xw->paste.target = XA_STRING;

	init_paste(xw);
	im_init(xw);

	XMapWindow(xw->display, xw->window);
}

void xw_die(struct xwindow *xw)
{
	XFreeGC(xw->display, xw->gc);
	XFreePixmap(xw->display, xw->pixbuf);
	XDestroyIC(xw->ic);
	XCloseIM(xw->im);
	XDestroyWindow(xw->display, xw->window);
	XCloseDisplay(xw->display);
}

static inline void draw_sixel(struct xwindow *xw, int line, int col, struct cell_t *cellp)
{
	int w, h;
	uint32_t color = 0;

	for (h = 0; h < CELL_HEIGHT; h++) {
		for (w = 0; w < CELL_WIDTH; w++) {
			memcpy(&color, cellp->pixmap + BYTES_PER_PIXEL * (h * CELL_WIDTH + w), BYTES_PER_PIXEL);

			if (color_list[DEFAULT_BG] != color) {
				XSetForeground(xw->display, xw->gc, color);
				XDrawPoint(xw->display, xw->pixbuf, xw->gc,
					col * CELL_WIDTH + w, line * CELL_HEIGHT + h);
			}
		}
	}
}

static inline void draw_line(struct xwindow *xw, struct terminal *term, int line)
{
	int bdf_padding, glyph_width, margin_right;
	int col, w, h, begin, current, pos;
	uint8_t color_tmp;
	struct color_pair_t color_pair;
	struct cell_t *cellp;
	const struct glyph_t *glyphp;

	struct paste_t *paste = &xw->paste;

	/* at first, fill all pixels of line in backgournd color */
	XSetForeground(xw->display, xw->gc, color_list[DEFAULT_BG]);
	XFillRectangle(xw->display, xw->pixbuf, xw->gc, 0, line * CELL_HEIGHT, term->width, CELL_HEIGHT);

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* draw sixel pixmap */
		cellp = &term->cells[line][col];
		if (cellp->has_pixmap) {
			draw_sixel(xw, line, col, cellp);
			continue;
		}

		/* get color and glyph */
		color_pair = cellp->color_pair;
		glyphp     = cellp->glyphp;

		/* check wide character or not */
		glyph_width = (cellp->width == HALF) ? CELL_WIDTH: CELL_WIDTH * 2;
		bdf_padding = my_ceil(glyph_width, BITS_PER_BYTE) * BITS_PER_BYTE - glyph_width;
		if (cellp->width == WIDE)
			bdf_padding += CELL_WIDTH;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cellp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cellp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color_pair.fg = DEFAULT_BG;
			color_pair.bg = (!tty.visible && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		/* show copy area */
		if (paste->copy_cursor_visible) {
			pos     = col + line * term->cols;
			begin   = paste->begin.x + paste->begin.y * term->cols;
			current = paste->current.x + paste->current.y * term->cols;

			if (pos == begin || pos == current) {
				color_tmp     = color_pair.fg;
				color_pair.fg = color_pair.bg;
				color_pair.bg = color_tmp;
			}
		}

		for (h = 0; h < CELL_HEIGHT; h++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((h == (CELL_HEIGHT - 1)) && (cellp->attribute & attr_mask[ATTR_UNDERLINE]))
				color_pair.bg = color_pair.fg;

			for (w = 0; w < CELL_WIDTH; w++) {
				/* set color palette */
				if (glyphp->bitmap[h] & (0x01 << (bdf_padding + w)))
					XSetForeground(xw->display, xw->gc, color_list[color_pair.fg]);
				else if (color_pair.bg != DEFAULT_BG)
					XSetForeground(xw->display, xw->gc, color_list[color_pair.bg]);
				else /* already draw */
					continue;

				/* update copy buffer */
				XDrawPoint(xw->display, xw->pixbuf, xw->gc,
					term->width - 1 - margin_right - w, line * CELL_HEIGHT + h);
			}
		}
	}
	/* actual display update: move to refresh() */
	XCopyArea(xw->display, xw->pixbuf, xw->window, xw->gc, 0, line * CELL_HEIGHT,
		term->width, CELL_HEIGHT, 0, line * CELL_HEIGHT);
	*/

	term->line_dirty[line] = ((term->mode & MODE_CURSOR) && term->cursor.y == line) ? true: false;
}

void set_preedit_pos(struct xwindow *xw, struct terminal *term)
{
	XVaNestedList list;
	XPoint pos;

	/* FIXME: hard coding! this offset for xim, not suitable for scim */
	/* TODO: implement xim {Preedit,Status}Callbacks */
	pos.x = term->cursor.x * CELL_WIDTH;
	pos.y = (term->cursor.y + 1) * CELL_HEIGHT - 2;

	list = XVaCreateNestedList(0, XNSpotLocation, &pos, NULL);

	XSetICValues(xw->ic, XNPreeditAttributes, list, NULL);

	XFree(list);
}

void refresh(struct xwindow *xw, struct terminal *term)
{
	int line, update_from, update_to;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	update_from = update_to = -1;
	for (line = 0; line < term->lines; line++) {
		if (term->line_dirty[line]) {
			draw_line(xw, term, line);

			if (update_from == -1)
				update_from = update_to = line;
			else
				update_to = line;
		}
	}

	/* actual display update: vertical synchronizing */
	if (update_from != -1)
		XCopyArea(xw->display, xw->pixbuf, xw->window, xw->gc, 0, update_from * CELL_HEIGHT,
			term->width, (update_to - update_from + 1) * CELL_HEIGHT, 0, update_from * CELL_HEIGHT);

	if (xw->input_style & XIMPreeditPosition)
		set_preedit_pos(xw, term);
}
