/* See LICENSE for licence details. */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define XK_NO_MOD UINT_MAX

enum { /* default terminal size 80x24 (8x16 dot font) */
	TERM_WIDTH  = 640,
	TERM_HEIGHT = 384,
};

struct keymap_t {
	KeySym keysym;
	unsigned int mask;
	char str[BUFSIZE];
};

const struct keymap_t keymap[] = {
	{XK_BackSpace, XK_NO_MOD, "\177"      },
	{XK_Up,        Mod1Mask,  "\033\033[A"},
	{XK_Up,        XK_NO_MOD, "\033[A"    },
	{XK_Down,      Mod1Mask,  "\033\033[B"},
	{XK_Down,      XK_NO_MOD, "\033[B"    },
	{XK_Right,     Mod1Mask,  "\033\033[C"},
	{XK_Right,     XK_NO_MOD, "\033[C"    },
	{XK_Left,      Mod1Mask,  "\033\033[D"},
	{XK_Left,      XK_NO_MOD, "\033[D"    },
	{XK_Begin,     XK_NO_MOD, "\033[G"    },
	{XK_Home,      XK_NO_MOD, "\033[1~"   },
	{XK_Insert,    XK_NO_MOD, "\033[2~"   },
	{XK_Delete,    XK_NO_MOD, "\033[3~"   },
	{XK_End,       XK_NO_MOD, "\033[4~"   },
	{XK_Prior,     XK_NO_MOD, "\033[5~"   },
	{XK_Next,      XK_NO_MOD, "\033[6~"   },
	{XK_F1,        XK_NO_MOD, "\033[[A"   },
	{XK_F2,        XK_NO_MOD, "\033[[B"   },
	{XK_F3,        XK_NO_MOD, "\033[[C"   },
	{XK_F4,        XK_NO_MOD, "\033[[D"   },
	{XK_F5,        XK_NO_MOD, "\033[[E"   },
	{XK_F6,        XK_NO_MOD, "\033[17~"  },
	{XK_F7,        XK_NO_MOD, "\033[18~"  },
	{XK_F8,        XK_NO_MOD, "\033[19~"  },
	{XK_F9,        XK_NO_MOD, "\033[20~"  },
	{XK_F10,       XK_NO_MOD, "\033[21~"  },
	{XK_F11,       XK_NO_MOD, "\033[23~"  },
	{XK_F12,       XK_NO_MOD, "\033[24~"  },
	{XK_F13,       XK_NO_MOD, "\033[25~"  },
	{XK_F14,       XK_NO_MOD, "\033[26~"  },
	{XK_F15,       XK_NO_MOD, "\033[28~"  },
	{XK_F16,       XK_NO_MOD, "\033[29~"  },
	{XK_F17,       XK_NO_MOD, "\033[31~"  },
	{XK_F18,       XK_NO_MOD, "\033[32~"  },
	{XK_F19,       XK_NO_MOD, "\033[33~"  },
	{XK_F20,       XK_NO_MOD, "\033[34~"  },
};

/* not assigned:
    kcbt=\E[Z,kmous=\E[M,kspd=^Z,
*/

struct xwindow_t {
	Display *display;
	Window window;
	Pixmap pixbuf;
	Colormap cmap;
	GC gc;
	int width, height;
	int screen;
	unsigned long color_palette[COLORS];
};

#define PXCACHE_SIZE (1024)
struct pxcache_entry {
	uint32_t color;
	unsigned long pixel;
};
static struct pxcache_entry pxcache[PXCACHE_SIZE];
static int pxcache_top = 0;

bool search_pxcache(uint32_t color, unsigned long *pixel)
{
	int i;
	/* usually faster to search recently allocated colors first */
	for (i = pxcache_top - 1; i >= 0; i--) {
		if (pxcache[i].color == color) {
			*pixel = pxcache[i].pixel;
			return true;
		}
	}
	return false;
}

unsigned long color2pixel(struct xwindow_t *xw, uint32_t color)
{
	unsigned long pixel;
	if (search_pxcache(color, &pixel)) {
		return pixel;
	} else {
		XColor xc;

		xc.red   = ((color >> 16) & bit_mask[8]) << 8;
		xc.green = ((color >>  8) & bit_mask[8]) << 8;
		xc.blue  = ((color >>  0) & bit_mask[8]) << 8;

		if (!XAllocColor(xw->display, xw->cmap, &xc)) {
			logging(WARN, "could not allocate color\n");
			return BlackPixel(xw->display, DefaultScreen(xw->display));
		} else {
			if (pxcache_top == PXCACHE_SIZE) {
				logging(WARN, "pixel cache is full. starting over\n");
				pxcache_top = 0;
			}
			pxcache[pxcache_top].color = color;
			pxcache[pxcache_top].pixel = xc.pixel;
			pxcache_top++;
			return xc.pixel;
		}
	}
}

bool xw_init(struct xwindow_t *xw)
{
	XTextProperty xtext = {.value = (unsigned char *) "yaftx",
		.encoding = XA_STRING, .format = 8, .nitems = 5};

	if ((xw->display = XOpenDisplay(NULL)) == NULL) {
		logging(ERROR, "XOpenDisplay() failed\n");
		return false;
	}

	if (!XSupportsLocale())
		logging(WARN, "X does not support locale\n");

	if (XSetLocaleModifiers("") == NULL)
		logging(WARN, "cannot set locale modifiers\n");

	xw->cmap = DefaultColormap(xw->display, xw->screen);
	for (int i = 0; i < COLORS; i++)
		xw->color_palette[i] = color2pixel(xw, color_list[i]);

	xw->screen = DefaultScreen(xw->display);
	xw->window = XCreateSimpleWindow(xw->display, DefaultRootWindow(xw->display),
		0, 0, TERM_WIDTH, TERM_HEIGHT, 0, xw->color_palette[DEFAULT_FG], xw->color_palette[DEFAULT_BG]);
	XSetWMProperties(xw->display, xw->window, &xtext, NULL, NULL, 0, NULL, NULL, NULL); /* return void */

	xw->gc = XCreateGC(xw->display, xw->window, 0, NULL);
	XSetGraphicsExposures(xw->display, xw->gc, False);

	xw->width  = DisplayWidth(xw->display, xw->screen);
	xw->height = DisplayHeight(xw->display, xw->screen);
	xw->pixbuf = XCreatePixmap(xw->display, xw->window,
		xw->width, xw->height, XDefaultDepth(xw->display, xw->screen));

	XSelectInput(xw->display, xw->window,
		ExposureMask | KeyPressMask | StructureNotifyMask);

	XMapWindow(xw->display, xw->window);

	return true;
}

void xw_die(struct xwindow_t *xw)
{
	XFreeGC(xw->display, xw->gc);
	XFreePixmap(xw->display, xw->pixbuf);
	XDestroyWindow(xw->display, xw->window);
	XCloseDisplay(xw->display);
}

static inline void draw_sixel(struct xwindow_t *xw, int line, int col, struct cell_t *cellp)
{
	int w, h;
	uint32_t color = 0;

	for (h = 0; h < CELL_HEIGHT; h++) {
		for (w = 0; w < CELL_WIDTH; w++) {
			memcpy(&color, cellp->pixmap + BYTES_PER_PIXEL * (h * CELL_WIDTH + w), BYTES_PER_PIXEL);

			if (color_list[DEFAULT_BG] != color) {
				XSetForeground(xw->display, xw->gc, color2pixel(xw, color));
				XDrawPoint(xw->display, xw->pixbuf, xw->gc,
					col * CELL_WIDTH + w, line * CELL_HEIGHT + h);
			}
		}
	}
}

static inline void draw_line(struct xwindow_t *xw, struct terminal_t *term, int line)
{
	int bdf_padding, glyph_width, margin_right;
	int col, w, h;
	struct color_pair_t color_pair;
	struct cell_t *cellp;
	const struct glyph_t *glyphp;

	/* at first, fill all pixels of line in background color */
	XSetForeground(xw->display, xw->gc, xw->color_palette[DEFAULT_BG]);
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

		/* check cursor position */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cellp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cellp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color_pair.fg = DEFAULT_BG;
			color_pair.bg = (!vt_active && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		for (h = 0; h < CELL_HEIGHT; h++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((h == (CELL_HEIGHT - 1)) && (cellp->attribute & attr_mask[ATTR_UNDERLINE]))
				color_pair.bg = color_pair.fg;

			for (w = 0; w < CELL_WIDTH; w++) {
				/* set color palette */
				if (glyphp->bitmap[h] & (0x01 << (bdf_padding + w)))
					XSetForeground(xw->display, xw->gc, xw->color_palette[color_pair.fg]);
				else if (color_pair.bg != DEFAULT_BG)
					XSetForeground(xw->display, xw->gc, xw->color_palette[color_pair.bg]);
				else /* already draw */
					continue;

				/* update copy buffer */
				XDrawPoint(xw->display, xw->pixbuf, xw->gc,
					term->width - 1 - margin_right - w, line * CELL_HEIGHT + h);
			}
		}
	}

	term->line_dirty[line] = ((term->mode & MODE_CURSOR) && term->cursor.y == line) ? true: false;
}

void refresh(struct xwindow_t *xw, struct terminal_t *term)
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
}
