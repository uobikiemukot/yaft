/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
/* #include <execinfo.h> for DEBUG */
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

enum char_code {
	BEL = 0x07, BS = 0x08, HT = 0x09,
	LF = 0x0A, VT = 0x0B, FF = 0x0C,
	CR = 0x0D, ESC = 0x1B, SPACE = 0x20,
	BACKSLASH = 0x5C, DEL = 0x7F,
};

enum {
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024,			/* read, esc, various buffer size */
	SELECT_TIMEOUT = 20000,	/* used by select() */
	ESC_PARAMS = 16,		/* max parameters of csi/osc sequence */
	COLORS = 256,			/* num of color */
	UCS2_CHARS = 0x10000,	/* number of UCS2 glyph */
	CTRL_CHARS = 0x20,		/* number of ctrl_func */
	ESC_CHARS = 0x80,		/* number of esc_func */
	DEFAULT_CHAR = SPACE,	/* used for erase char, cell_size */
	RESET = 0x00,			/* reset for char_attr, term_mode, esc_state */
	BRIGHT_INC = 8,			/* value used for brightening color */
};

enum char_attr {
	BOLD = 1,				/* brighten foreground */
	UNDERLINE = 4,
	BLINK = 5,				/* brighten background */
	REVERSE = 7,
};

const uint8_t attr_mask[] = {
	0x00, 0x01, 0x00, 0x00,	/* 0:none      1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08,	/* 4:underline 5:blink 6:none 7:reverse */
};

const uint32_t bit_mask[] = {
	0x00,
	0x01, 0x03, 0x07, 0x0F,
	0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF,
	0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
	0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF,
	0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
	0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF,
	0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

enum term_mode {
	MODE_ORIGIN = 0x01,		/* origin mode: DECOM */
	MODE_CURSOR = 0x02,		/* cursor visible: DECTECM */
	MODE_AMRIGHT = 0x04,	/* auto wrap: DECAWM */
};

enum esc_state {
	STATE_ESC = 1,			/* 0x1B, \033, ESC */
	STATE_CSI,				/* ESC [ */
	STATE_OSC,				/* ESC ] */
};

enum width_flag {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

struct tty_state {
	struct termios *save_tm;
	bool visible;
	bool redraw_flag;
	bool loop_flag;
	bool setmode;
};

struct pair { int x, y; };
struct margin { int top, bottom; };
struct color_t { uint32_t r, g, b; };
struct color_pair { uint8_t fg, bg; };

struct framebuffer {
	uint8_t *fp;			/* pointer of framebuffer(read only) */
	uint8_t *wall;			/* buffer for wallpaper */
	uint8_t *buf;			/* copy of framebuffer */
	int fd;					/* file descriptor of framebuffer */
	struct pair res;		/* resolution (x, y) */
	long screen_size;		/* screen data size (byte) */
	int line_length;		/* line length (byte) */
	int bpp;				/* BYTES per pixel */
	uint32_t color_palette[COLORS];
	struct fb_cmap *cmap, *cmap_org;
};

struct cell {
	uint16_t code;			/* UCS2 */
	struct color_pair color;/* color (fg, bg) */
	uint8_t attribute;		/* bold, underscore, etc... */
	int wide;				/* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct parm_t {				/* for parse_arg() */
	int argc;
	char *argv[ESC_PARAMS];
};

struct esc_t {
	uint8_t buf[BUFSIZE];
	uint8_t *bp;
	int state;				/* esc state */
};

struct ucs_t {
	uint32_t code;			/* UCS4: but only print UCS2 (<= U+FFFF) */
	int length, count;
};

struct state_t {			/* for save, restore state */
	int mode;
	struct pair cursor;
	uint8_t attribute;
};

struct terminal {
	int fd;							/* master fd */
	struct pair offset;				/* window offset (x, y) */
	int width, height;				/* terminal size (pixel) */
	int cols, lines;				/* terminal size (cell) */
	struct cell *cells;				/* pointer to each cell: cells[cols + lines * num_of_cols] */
	struct margin scroll;			/* scroll margin */
	struct pair cursor;				/* cursor pos (x, y) */
	bool *line_dirty;				/* dirty flag */
	bool *tabstop;					/* tabstop flag */
	int mode;						/* for set/reset mode */
	bool wrap;						/* whether auto wrap occured or not */
	struct state_t state;			/* for restore */
	struct color_pair color;		/* color (fg, bg) */
	uint8_t attribute;				/* bold, underscore, etc... */
	struct esc_t esc;				/* store escape sequence */
	struct ucs_t ucs;				/* store UTF-8 sequence */
};

#include "conf.h"		/* user configuration */
#include "color.h"		/* 256color definition */
