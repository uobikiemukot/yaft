/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <math.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct xwindow xwindow;
typedef struct framebuffer framebuffer;
typedef struct terminal terminal;
typedef struct escape escape;
typedef struct cell cell;
typedef struct glyph_t glyph_t;
typedef struct pair pair;
typedef struct margin margin;
typedef struct color_pair color_pair;
typedef struct uchar uchar;
typedef struct state state;
typedef struct parm_t parm_t;

#include "./color.h"				/* 256color definition */
#include "./conf.h"				/* user configuration */

enum char_code {
	BEL = 0x07, ESC = 0x1B, SPACE = 0x20, BACKSLASH = 0x5C, DEL = 0x7F,
};

enum {
	/* misc */
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024,				/* read, esc, various buffer size */
	ESC_PARAMS = 16,			/* max parameters of csi/osc sequence */
	COLORS = 256,				/* num of color */
	UCS2_CHARS = 0x10000,		/* number of UCS2 glyph */
	CTRL_CHARS = 0x20,			/* number of control character */
	ESC_CHARS = 0x80,			/* number of final character of escape sequence */
	DEFAULT_CHAR = SPACE,		/* erase char, and used for cell_size: SPACE */
	RESET = 0x00,				/* reset for char_attr, term_mode, esc_state */
	BRIGHT_INC = 8,				/* incrment value: from dark color to light color */
};

enum width_flag {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

enum char_attr {
	BOLD = 1,					/* bright foreground */
	UNDERLINE = 4,
	BLINK = 5,					/* bright background */
	REVERSE = 7,
};

static const u8 attr_mask[] = {
	0x00, 0x01, 0x00, 0x00,		/* 0:none      1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08,		/* 4:underline 5:blink 6:none 7:reverse */
};

enum term_mode {
	ORIGIN = 0x01,				/* origin mode: DECOM */
	CURSOR = 0x02,				/* cursor visible: DECTECM */
	AMRIGHT = 0x04,				/* auto wrap: DECAWM */
};

enum esc_state {
	STATE_ESC = 1,				/* 0x1B, \033, ESC */
	STATE_CSI,					/* ESC [ */
	STATE_OSC,					/* ESC ] */
};

struct pair {
	int x, y;
};

struct margin {
	int top, bottom;
};

struct color_pair {
	int fg, bg;
};

struct xwindow {
	Display *dsp;
	Window win;
	Pixmap buf;
	GC gc;
	pair res;
	int sc;
};

struct cell {
	u16 code;					/* UCS2 */
	color_pair color;			/* color (fg, bg) */
	u8 attribute;				/* bold, underscore, etc... */
	int wide;					/* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct escape {
	u8 buf[BUFSIZE];
	u8 *bp;
	int state;					/* esc state */
};

struct uchar {
	u32 code;					/* UCS4: but only print UCS2 (< 0xFFFF) */
	int length, count;
};

struct glyph_t {
	pair size;
	u32 *bitmap;
};

struct state {					/* for save/restore state */
	int mode;
	pair cursor;
	u8 attribute;
};

struct parm_t {
	int argc;
	char *argv[ESC_PARAMS];
};

struct terminal {
	int fd;						/* master fd */

	glyph_t *fonts[UCS2_CHARS];
	u32 *wall;					/* buffer for wallpaper */

	pair offset;				/* window offset (x, y) */
	int width, height;			/* terminal size (pixel) */

	int cols, lines;			/* terminal size (cell) */
	cell *cells;				/* pointer of each cell: cells[cols + lines * num_of_cols] */
	pair cell_size;				/* default glyph size */

	margin scroll;				/* scroll margin */
	pair cursor;				/* cursor pos (x, y) */

	bool *line_dirty;			/* dirty flag */
	bool *tabstop;				/* tabstop flag */

	int mode;					/* for set/reset mode */
	bool wrap;					/* whether auto wrap occured or not */
	state save_state;			/* for restore */

	color_pair color;			/* color (fg, bg) */
	u8 attribute;				/* bold, underscore, etc... */

	escape esc;					/* store escape sequence */
	uchar ucs;					/* store UTF-8 sequence */
};

static void (*ctrl_func[CTRL_CHARS])(terminal * term, void *arg);
static void (*esc_func[ESC_CHARS])(terminal * term, void *arg);
static void (*csi_func[ESC_CHARS])(terminal * term, void *arg);
