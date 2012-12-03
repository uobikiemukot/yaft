/* See LICENSE for licence details. */
/* framubuffer device */
const char *fb_path = "/dev/fb0";

/* shell */
const char *shell_cmd = "/bin/bash";

/* TERM value */
const char *term_name = "yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	/*
	*/
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	CURSOR_COLOR = 2,
	/* solarized
	DEFAULT_FG = 12,
	DEFAULT_BG = 8,
	CURSOR_COLOR = 2,
	*/
};

/* misc */
enum {
	DEBUG = false,
	TABSTOP = 8,
	SUBSTITUTE_HALF = 0x20,   /* used for missing glyph: SPACE (0x20) */
	SUBSTITUTE_WIDE = 0x3000, /* used for missing glyph: IDEOGRAPHIC SPACE(0x3000) */
	REPLACEMENT_CHAR = 0xFFFD,  /* used for malformed UTF-8 sequence: REPLACEMENT CHARACTER */
};
