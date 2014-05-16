/* See LICENSE for licence details. */
#include <limits.h> /* UINT_MAX */
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#define XK_NO_MOD UINT_MAX

enum x_misc {
    WIDTH = 640,
    HEIGHT = 384,
};

struct keydef {
	KeySym k;
	unsigned int mask;
	char s[BUFSIZE];
};

const struct keydef key[] = {
    { XK_BackSpace, XK_NO_MOD, "\177"    },
    { XK_Up,        XK_NO_MOD, "\033[A"  },
    { XK_Down,      XK_NO_MOD, "\033[B"  },
    { XK_Right,     XK_NO_MOD, "\033[C"  },
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

struct xwindow {
    Display *display;
    Window window;
    Pixmap pixbuf;
    GC gc;
	int width, height;
    int screen;
};

void xw_init(struct xwindow *xw)
{
	XTextProperty title;

	if ((xw->display = XOpenDisplay(NULL)) == NULL)
		fatal("XOpenDisplay failed");

	xw->screen = DefaultScreen(xw->display);

	xw->window = XCreateSimpleWindow(xw->display, DefaultRootWindow(xw->display),
		0, 0, WIDTH, HEIGHT, 0, color_list[DEFAULT_FG], color_list[DEFAULT_BG]);

	title.value = (unsigned char *) "yaftx";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 5;
	XSetWMProperties(xw->display, xw->window, &title, NULL, NULL, 0, NULL, NULL, NULL);

	xw->gc = XCreateGC(xw->display, xw->window, 0, NULL);

	xw->width = WIDTH;
	xw->height = HEIGHT;

	xw->pixbuf = XCreatePixmap(xw->display, xw->window, WIDTH, HEIGHT, XDefaultDepth(xw->display, xw->screen));
	XSelectInput(xw->display, xw->window, FocusChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);

	XMapWindow(xw->display, xw->window);
}

void xw_die(struct xwindow *xw)
{
	XFreeGC(xw->display, xw->gc);
	XFreePixmap(xw->display, xw->pixbuf);
	XDestroyWindow(xw->display, xw->window);
	XCloseDisplay(xw->display);
}

void draw_bitmap(struct xwindow *xw, struct terminal *term, int y, int x, int offset)
{
	int i, shift, glyph_width;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;
	extern struct tty_state tty;

	cp = &term->cells[x + y * term->cols];
	if (cp->width == NEXT_TO_WIDE)
		return;

	gp = cp->gp;
	glyph_width = gp->width * CELL_WIDTH;
	shift = ((glyph_width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	if ((term->mode & MODE_CURSOR && y == term->cursor.y) /* cursor */
		&& (x == term->cursor.x || (cp->width == WIDE && (x + 1) == term->cursor.x))) {
		color.fg = DEFAULT_BG;
		color.bg = (tty.visible) ? ACTIVE_CURSOR_COLOR: PASSIVE_CURSOR_COLOR;
	}

	if ((offset == (CELL_HEIGHT - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;

	for (i = 0; i < glyph_width; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1))) {
			XSetForeground(xw->display, xw->gc, color_list[color.fg]);
			XDrawPoint(xw->display, xw->pixbuf, xw->gc, CELL_WIDTH * x + i, CELL_HEIGHT * y + offset);
		}
		else if (color.bg != DEFAULT_BG) {
			XSetForeground(xw->display, xw->gc, color_list[color.bg]);
			XDrawPoint(xw->display, xw->pixbuf, xw->gc, CELL_WIDTH * x + i, CELL_HEIGHT * y + offset);
		}
	}
}

void draw_line(struct xwindow *xw, struct terminal *term, int y)
{
	int offset, x;

	XSetForeground(xw->display, xw->gc, color_list[DEFAULT_BG]);
	XFillRectangle(xw->display, xw->pixbuf, xw->gc, 0, y * CELL_HEIGHT, term->width, CELL_HEIGHT);

	for (offset = 0; offset < CELL_HEIGHT; offset++) {
		for (x = 0; x < term->cols; x++)
			draw_bitmap(xw, term, y, x, offset);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;

	XCopyArea(xw->display, xw->pixbuf, xw->window, xw->gc, 0, y * CELL_HEIGHT,
		term->width, CELL_HEIGHT, 0, y * CELL_HEIGHT);
}

void refresh(struct xwindow *xw, struct terminal *term)
{
	int y;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (y = 0; y < term->lines; y++) {
		if (term->line_dirty[y])
			draw_line(xw, term, y);
	}
}
