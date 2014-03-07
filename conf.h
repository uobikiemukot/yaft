/* See LICENSE for licence details. */
/* framubuffer device */
const char *fb_path = "/dev/fb0";

/* shell */
const char *shell_cmd = "/bin/bash";

/* TERM value */
const char *term_name = "yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	CURSOR_COLOR = 2,
};

/* misc */
enum {
	DEBUG = false,
	TABSTOP = 8,
	SUBSTITUTE_HALF = 0x20,   /* used for missing glyph: SPACE (0x20) */
	SUBSTITUTE_WIDE = 0x3000, /* used for missing glyph: IDEOGRAPHIC SPACE(0x3000) */
	REPLACEMENT_CHAR = 0xFFFD,  /* used for malformed UTF-8 sequence: REPLACEMENT CHARACTER */
	LAZY_DRAW = false,
	BACKGROUND_DRAW = false,
	NORMAL = 0, CLOCKWISE = 90, UPSIDE_DOWN = 180, COUNTER_CLOCKWISE = 270,
	ROTATE = CLOCKWISE, /* 0 or 90 or 180 or 270 (see above) */
};
