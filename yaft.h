/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "glyph.h"
#include "color.h"

enum char_code {
	/* 7 bit */
	BEL = 0x07, BS  = 0x08, HT  = 0x09,
	LF  = 0x0A, VT  = 0x0B, FF  = 0x0C,
	CR  = 0x0D, ESC = 0x1B, DEL = 0x7F,
	/* others */
	SPACE     = 0x20,
	BACKSLASH = 0x5C,
};

enum misc {
	BUFSIZE            = 1024,             /* read, esc, various buffer size */
	BITS_PER_BYTE      = 8,                /* bits per byte */
	ESCSEQ_SIZE        = 1024,             /* limit size of terminal escape sequence */
	SELECT_TIMEOUT     = 15000,            /* used by select() */
	SLEEP_TIME         = 30000,            /* sleep time at EAGAIN, EWOULDBLOCK (usec) */
	MAX_ARGS           = 16,               /* max parameters of csi/osc sequence */
	UCS2_CHARS         = 0x10000,          /* number of UCS2 glyphs */
	CTRL_CHARS         = 0x20,             /* number of ctrl_func */
	ESC_CHARS          = 0x80,             /* number of esc_func */
	DEFAULT_CHAR       = SPACE,            /* used for erase char */
	BRIGHT_INC         = 8,                /* value used for brightening color */
};

enum char_attr {
	ATTR_RESET     = 0,
	ATTR_BOLD      = 1, /* brighten foreground */
	ATTR_UNDERLINE = 4,
	ATTR_BLINK     = 5, /* brighten background */
	ATTR_REVERSE   = 7,
};

const uint8_t attr_mask[] = {
	0x00, 0x01, 0x00, 0x00, /* 0:none      1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08, /* 4:underline 5:blink 6:none 7:reverse */
};

const uint32_t bit_mask[] = {
	0x00,
	0x01,       0x03,       0x07,       0x0F,
	0x1F,       0x3F,       0x7F,       0xFF,
	0x1FF,      0x3FF,      0x7FF,      0xFFF,
	0x1FFF,     0x3FFF,     0x7FFF,     0xFFFF,
	0x1FFFF,    0x3FFFF,    0x7FFFF,    0xFFFFF,
	0x1FFFFF,   0x3FFFFF,   0x7FFFFF,   0xFFFFFF,
	0x1FFFFFF,  0x3FFFFFF,  0x7FFFFFF,  0xFFFFFFF,
	0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

enum osc {
	OSC_GWREPT = 8900, /* OSC Ps: mode number of yaft GWREPT */
};

enum term_mode {
	MODE_RESET   = 0x00,
	MODE_ORIGIN  = 0x01, /* origin mode: DECOM */
	MODE_CURSOR  = 0x02, /* cursor visible: DECTCEM */
	MODE_AMRIGHT = 0x04, /* auto wrap: DECAWM */
	MODE_VWBS    = 0x08, /* variable-width backspace */
};

enum esc_state {
	STATE_RESET = 0x00,
	STATE_ESC   = 0x01, /* 0x1B, \033, ESC */
	STATE_CSI   = 0x02, /* ESC [ */
	STATE_OSC   = 0x04, /* ESC ] */
	STATE_DCS   = 0x08, /* ESC P */
};

enum glyph_width {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

struct margin_t { uint16_t top, bottom; };
struct point_t { uint16_t x, y; };
struct color_pair_t { uint8_t fg, bg; };

struct cell_t {
	const struct glyph_t *glyphp;   /* pointer to glyph */
	struct color_pair_t color_pair; /* color (fg, bg) */
	enum char_attr attribute;       /* bold, underscore, etc... */
	enum glyph_width width;         /* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct esc_t {
	char *buf;
	char *bp;
	int size;
	enum esc_state state;
};

struct charset_t {
	uint32_t code;     /* UCS2 code point: yaft only supports BMP */
	int following_byte;
	int count;
	bool is_valid;
};

struct state_t {   /* for save, restore state */
	struct point_t cursor;
	enum term_mode mode;
	enum char_attr attribute;
};

struct terminal_t {
	int fd;                                  /* master of pseudo terminal */
	int width, height;                       /* terminal size (pixel) */
	int cols, lines;                         /* terminal size (cell) */
	struct cell_t **cells;                   /* pointer to each cell: cells[y * lines + x] */
	struct margin_t scroll;                  /* scroll margin */
	struct point_t cursor;                   /* cursor pos (x, y) */
	bool *line_dirty;                        /* dirty flag */
	bool *tabstop;                           /* tabstop flag */
	enum term_mode mode;                     /* for set/reset mode */
	bool wrap_occured;                       /* whether auto wrap occured or not */
	struct state_t state;                    /* for restore */
	struct color_pair_t color_pair;          /* color (fg, bg) */
	enum char_attr attribute;                /* bold, underscore, etc... */
	struct charset_t charset;                /* store UTF-8 byte stream */
	struct esc_t esc;                        /* store escape sequence */
	const struct glyph_t *glyph[UCS2_CHARS]; /* array of pointer to glyphs[] */
};

struct parm_t { /* for parse_arg() */
	int argc;
	char *argv[MAX_ARGS];
};

volatile sig_atomic_t vt_active   = true;  /* SIGUSR1: vt is active or not */
volatile sig_atomic_t need_redraw = false; /* SIGUSR1: vt activated */
volatile sig_atomic_t child_alive = false; /* SIGCHLD: child process (shell) is alive or not */
struct termios termios_orig;
