#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stropts.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <ctype.h>
#include <pty.h>
#include <assert.h>

#include "color.h" /* 256color definition */
#include "conf.h" /* user configuration */

enum {
	/* misc */
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024, /* read, esc, various buffer size */
	ESC_PARAMS = 64, /* max parameters of csi/osc sequence */
	COLORS = 256, /* num of color */
	UCS_CHARS = 0xFFFF, /* glyph num: UCS2 */
	DEFAULT_CHAR = 0x7F, /* erase char, and used for cell_size: DEL */
	RESET = 0x00, /* reset for char_attr, term_mode, esc_state */
};

enum width_flag {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

enum char_attr {
	BOLD = 0x01, /* only affect for ascii */
	UNDERLINE = 0x02,
	REVERSE = 0x04,
};

enum term_mode {
	ORIGIN = 0x01, /* origin mode: DECOM */
	CURSOR = 0x02, /* cursor visible: DECTECM */
	COLUMN = 0x04, /* 132/80 columns mode switch: DECCOLM */
	AMRIGHT = 0x08, /* auto right margin: DECAWM */
	AMLEFT = 0x10, /* auto left margin */
};

enum esc_state {
	ESC = 1, /* 0x1B, \033, ESC */
	CSI, /* ESC [ */
	OSC, /* ESC ] */
};

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

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

struct pair { int x, y; };
struct margin { int top, bottom; };
struct color_pair { u32 fg, bg; };

struct framebuffer {
	u32 *fp; /* pointer of framebuffer(read only), assume bits per pixel == 32 */
	int fd; /* file descriptor of framebuffer */
	pair res; /* resolution (x, y) */
	long sc_size; /* screen data size (bytes) */
};

struct cell {
	u16 code; /* UCS2 */
	color_pair color; /* color (fg, bg) */
	u8 attribute; /* bold, underscore, etc... */
	int wide; /* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct escape {
	u8 buf[BUFSIZE];
	u8 *bp;
	int state; /* esc state */
};

struct uchar {
	u32 code; /* UCS4: but only print UCS2 (< 0xFFFF) */
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
	int fd; /* master fd */

	glyph_t *fonts[UCS_CHARS];
	u32 *wall; /* buffer for wallpaper */

	pair offset; /* window offset (x, y) */
	int width, height; /* terminal size (pixel) */

	int cols, lines; /* terminal size (cell) */
	cell *cells; /* pointer of each cell: cells[lines][cols] */
	pair cell_size; /* default glyph size */

	margin scroll; /* scroll margin */
	pair cursor; /* cursor pos (x, y) */

	bool *line_dirty; /* dirty flag */
	bool *tabstop; /* tabstop flag */

	int mode; /* for set/reset mode */
	state save_state; /* for restore */

	color_pair color; /* color (fg, bg) */
	u8 attribute; /* bold, underscore, etc... */
	u32 color_palette[COLORS];

	escape esc; /* store escape sequence */
	uchar ucs;  /* store UTF-8 sequence */
};

static void (*ctrl_func[0x1F])(terminal *term, void *arg);
static void (*esc_func[0x7F])(terminal *term, void *arg);
static void (*csi_func[0x7F])(terminal *term, void *arg);
static void (*osc_func[0x7F])(terminal *term, void *arg);
