/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef uint64_t bitmap_width_t;

enum char_code {
	/* 7 bit control char */
	BEL = 0x07, BS  = 0x08, HT  = 0x09,
	LF  = 0x0A, VT  = 0x0B, FF  = 0x0C,
	CR  = 0x0D, ESC = 0x1B, DEL = 0x7F,
	/* others */
	SPACE     = 0x20,
	BACKSLASH = 0x5C,
};

enum misc {
	BITS_PER_BYTE     = 8,
	BUFSIZE           = 1024,    /* read, esc, various buffer size */
	UCS2_CHARS        = 0x10000, /* number of UCS2 glyph */
	DEFAULT_CHAR      = SPACE,   /* used for erase char, cell_size */
	MAX_HEIGHT        = 64,
	BDF_HEADER        = 0,
	BDF_CHAR          = 1,
	BDF_BITMAP        = 2,
};

enum encode_t {
	X68000,
	JISX0208,
	JISX0201,
	/*
	JISX0212,
	JISX0213,
	*/
	ISO8859,
	ISO10646
};

enum glyph_width_t {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

struct glyph_t {
	uint8_t width, height;
	bitmap_width_t *bitmap;
};

struct glyph_list_t {
	uint32_t code;
	struct glyph_t *glyph;
	struct glyph_list_t *next;
};

struct bdf_header_t {
	int bbw, bbh, bbx, bby;
	int ascent, descent;
	int default_char;
	int chars;
	int pixel_size;
	char charset[BUFSIZE];
};

struct bdf_char_t {
	int bbw, bbh, bbx, bby;
	int dwidth;
	int encoding;
	bitmap_width_t bitmap[MAX_HEIGHT];
};

int convert_table[UCS2_CHARS];
