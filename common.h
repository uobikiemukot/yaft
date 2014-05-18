/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
	/* 7 bit */
	BEL = 0x07, BS  = 0x08, HT  = 0x09,
	LF  = 0x0A, VT  = 0x0B, FF  = 0x0C,
	CR  = 0x0D, ESC = 0x1B, DEL = 0x7F,
	/* 8 bit */
	IND   = 0x84, NEL = 0x85, HTS = 0x88, RI  = 0x8D,
	SS2   = 0x8E, SS3 = 0x8F, DCS = 0x90, SOS = 0x98,
	DECID = 0x9A, CSI = 0x9B, ST  = 0x9C, OSC = 0x9D,
	PM    = 0x9E, APC = 0x9F,
	/* others */
	SPACE = 0x20,
	BACKSLASH = 0x5C,
	IDEOGRAPHIC_SPACE = 0x3000,
	REPLACEMENT_CHARACTER = 0xFFFD,
};

enum misc {
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024,         /* read, esc, various buffer size */
	MAX_ESC_LENGTH = 65535, /* limit of terminal escape sequence*/
	SELECT_TIMEOUT = 15000, /* used by select() */
	ESC_PARAMS = 16,        /* max parameters of csi/osc sequence */
	COLORS = 256,           /* num of color */
	UCS2_CHARS = 0x10000,   /* number of UCS2 glyph */
	CTRL_CHARS = 0x20,      /* number of ctrl_func */
	ESC_CHARS = 0x80,       /* number of esc_func */
	DRCS_CHARSETS = 63,     /* number of charset of DRCS (according to DRCSMMv1) */
	GLYPH_PER_CHARSET = 96, /* number of glyph of each DRCS charset */
	BITS_PER_SIXEL = 6,     /* number of bits of a sixel */
	DEFAULT_CHAR = SPACE,   /* used for erase char, cell_size */
	RESET = 0x00,           /* reset for char_attr, term_mode, esc_state */
	BRIGHT_INC = 8,         /* value used for brightening color */
	OSC_GWREPT = 8900,      /* OSC Ps: mode number of yaft GWREPT */
};

enum char_attr {
	BOLD = 1,      /* brighten foreground */
	UNDERLINE = 4,
	BLINK = 5,     /* brighten background */
	REVERSE = 7,
};

const uint8_t attr_mask[] = {
	0x00, 0x01, 0x00, 0x00, /* 0:none      1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08, /* 4:underline 5:blink 6:none 7:reverse */
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
	MODE_ORIGIN = 0x01,  /* origin mode: DECOM */
	MODE_CURSOR = 0x02,  /* cursor visible: DECTCEM */
	MODE_AMRIGHT = 0x04, /* auto wrap: DECAWM */
};

enum esc_state {
	STATE_RESET = 0,
	STATE_ESC,       /* 0x1B, \033, ESC */
	STATE_CSI,       /* ESC [ */
	STATE_OSC,       /* ESC ] */
	STATE_DCS,       /* ESC P */
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
	bool background_draw;
};

struct pair { int x, y; };
struct margin { int top, bottom; };
struct color_t { uint32_t r, g, b; };
struct color_pair { uint8_t fg, bg; };

struct cell {
	const struct static_glyph_t *gp; /* pointer to glyph */
	struct color_pair color;         /* color (fg, bg) */
	uint8_t attribute;               /* bold, underscore, etc... */
	enum width_flag width;           /* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct parm_t { /* for parse_arg() */
	int argc;
	char *argv[ESC_PARAMS];
};

struct esc_t {
	char buf[MAX_ESC_LENGTH];
	char *bp;
	enum esc_state state; /* esc state */
};

struct charset_t {
	uint32_t code; /* UCS4 code point but only print UCS2 (<= U+FFFF) */
	int following_byte, count;
	bool is_valid;
};

struct state_t { /* for save, restore state */
	enum term_mode mode;
	struct pair cursor;
	uint8_t attribute;
};

struct drcs_t { /* for drcs */
	uint8_t start_char;
	int erase_mode;
	//int charset_num;             /* 94 or 96 */
	struct static_glyph_t *chars;
};

//struct sixel {
	//uint8_t width;             /* always 1 */
	//char *pixmap[CELL_HEIGHT]; /* size: 24bpp * cell width */
//};

struct terminal {
	int fd;                         /* master fd */
	int width, height;              /* terminal size (pixel) */
	int cols, lines;                /* terminal size (cell) */
	struct cell *cells;             /* pointer to each cell: cells[cols + lines * num_of_cols] */
	struct margin scroll;           /* scroll margin */
	struct pair cursor;             /* cursor pos (x, y) */
	bool *line_dirty;               /* dirty flag */
	bool *tabstop;                  /* tabstop flag */
	enum term_mode mode;            /* for set/reset mode */
	bool wrap;                      /* whether auto wrap occured or not */
	struct state_t state;           /* for restore */
	struct color_pair color;        /* color (fg, bg) */
	uint8_t attribute;              /* bold, underscore, etc... */
	struct esc_t esc;               /* store escape sequence */
	struct charset_t charset;       /* store UTF-8 byte stream */
	uint32_t color_palette[COLORS]; /* 256 color palette */
	const struct static_glyph_t
		*glyph_map[UCS2_CHARS];

	struct drcs_t drcs[DRCS_CHARSETS];
	int drcs_index;
};

struct tty_state tty = {
	.save_tm = NULL,
	.visible = true,
	.redraw_flag = false,
	.loop_flag = true,
	.background_draw = false,
};
