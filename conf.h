/* See LICENSE for licence details. */
/* framubuffer device */
const char *fb_path = "/dev/fb0";
//const char *fb_path = "/dev/tty"; /* for FreeBSD */

/* shell */
const char *shell_cmd = "/bin/bash";
//const char *shell_cmd = "/bin/csh"; /* for FreeBSD */

/* TERM value */
const char *term_name = "yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	ACTIVE_CURSOR_COLOR = 2,
	PASSIVE_CURSOR_COLOR = 1,
};

/* misc */
enum {
	DEBUG = true,
	TABSTOP = 8,
	SUBSTITUTE_HALF = 0x20,   /* used for missing glyph: SPACE (0x20) */
	SUBSTITUTE_WIDE = 0x3000, /* used for missing glyph: IDEOGRAPHIC SPACE(0x3000) */
	REPLACEMENT_CHAR = 0xFFFD,  /* used for malformed UTF-8 sequence: REPLACEMENT CHARACTER */
	LAZY_DRAW = true,
};
