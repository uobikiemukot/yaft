/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pty.h>
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

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef struct termios termios;
typedef struct timeval timeval;
typedef struct winsize winsize;
typedef struct fb_fix_screeninfo finfo_t;
typedef struct fb_var_screeninfo vinfo_t;
typedef struct fb_cmap cmap_t;

typedef struct framebuffer framebuffer;
typedef struct terminal terminal;
typedef struct escape escape;
typedef struct cell cell;
typedef struct glyph_t glyph_t;
typedef struct pair pair;
typedef struct margin margin;
typedef struct color_t color_t;
typedef struct color_pair color_pair;
typedef struct uchar uchar;
typedef struct state state;
typedef struct parm_t parm_t;

enum char_code {
	BEL = 0x07, BS = 0x08, HT = 0x09,
	LF = 0x0A, VT = 0x0B, FF = 0x0C,
	CR = 0x0D, ESC = 0x1B, SPACE = 0x20,
	BACKSLASH = 0x5C, DEL = 0x7F,
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

enum char_attr {
	BOLD = 1,					/* bright foreground */
	UNDERLINE = 4,
	BLINK = 5,					/* bright background */
	REVERSE = 7,
};

const u8 attr_mask[] = {
	0x00, 0x01, 0x00, 0x00,		/* 0:none	  1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08,		/* 4:underline 5:blink 6:none 7:reverse */
};

u32 bit_mask[] = {
	0x00,
	0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
	0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
	0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

enum term_mode {
	MODE_ORIGIN = 0x01,				/* origin mode: DECOM */
	MODE_CURSOR = 0x02,				/* cursor visible: DECTECM */
	MODE_AMRIGHT = 0x04,			/* auto wrap: DECAWM */
};

enum esc_state {
	STATE_ESC = 1,				/* 0x1B, \033, ESC */
	STATE_CSI,					/* ESC [ */
	STATE_OSC,					/* ESC ] */
};

enum width_flag {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

struct pair { int x, y; };

struct margin { int top, bottom; };

struct color_t { u32 r, g, b; };

struct color_pair { int fg, bg; };

struct framebuffer {
	u8 *fp;						/* pointer of framebuffer(read only), assume bits per pixel == 32 */
	u8 *wall;					/* buffer for wallpaper */
	int fd;						/* file descriptor of framebuffer */
	pair res;					/* resolution (x, y) */
	long screen_size;			/* screen data size (byte) */
	int line_len;				/* line length (byte) */
	int bpp;					/* "bytes" per pixel */
	u32 color_palette[COLORS];
	cmap_t *cmap, *cmap_org;
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
	glyph_t *fonts[UCS2_CHARS];
};

#include "conf.h"				/* user configuration */
#include "color.h"				/* 256color definition */
