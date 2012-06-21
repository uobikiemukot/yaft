#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <string.h>
#include <signal.h>
#include <pty.h>
#include <ctype.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct fb_fix_screeninfo fix_info;
typedef struct fb_var_screeninfo var_info;
typedef struct fb_bitfield bitfield;
typedef struct termios termios;
typedef struct timeval timeval;
typedef struct winsize winsize;

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

#include "color.h"				/* 256color definition */
#include "conf.h"				/* user configuration */

enum {
	/* misc */
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024,				/* read, esc, various buffer size */
	ESC_PARAMS = 64,			/* max parameters of csi/osc sequence */
	COLORS = 256,				/* num of color */
	UCS_CHARS = 0x10000,		/* number of UCS2 glyph */
	CTRL_CHARS = 0x20,			/* number of control character */
	ESC_CHARS = 0x80,			/* number of final character of escape sequence */
	SPACE = 0x20,
	DEL = 0x7F,
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
	BOLD = 1,					/* only affect for ascii */
	UNDERLINE = 4,
	BLINK = 5,
	REVERSE = 7,
};

static u8 attr_mask[] = {
	0x00,						/* 0: not used */
	0x01,						/* 1: bold */
	0x00,						/* 2: not used */
	0x00,						/* 3: not used */
	0x02,						/* 4: underline */
	0x04,						/* 5: blink */
	0x00,						/* 6: not used */
	0x08,						/* 7: reverse */
};

enum term_mode {
	ORIGIN = 0x01,				/* origin mode: DECOM */
	CURSOR = 0x02,				/* cursor visible: DECTECM */
	AMRIGHT = 0x04,
};

enum esc_state {
	ESC = 1,					/* 0x1B, \033, ESC */
	CSI,						/* ESC [ */
	OSC,						/* ESC ] */
};

struct pair {
	int x, y;
};
struct margin {
	int top, bottom;
};
struct color_pair {
	u32 fg, bg;
};

struct framebuffer {
	u32 *fp;					/* pointer of framebuffer(read only), assume bits per pixel == 32 */
	int fd;						/* file descriptor of framebuffer */
	pair res;					/* resolution (x, y) */
	long sc_size;				/* screen data size (bytes) */
	int line_length;			/* (pixel) */
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
	u8 *bitmap;
};

struct state {
	/* saves cursor position, character attribute
	   and origin mode selection */
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

	glyph_t *fonts[UCS_CHARS];
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

static void (*ctrl_func[CTRL_CHARS]) (terminal * term, void *arg);
static void (*esc_func[ESC_CHARS]) (terminal * term, void *arg);
static void (*csi_func[ESC_CHARS]) (terminal * term, void *arg);
