/* See LICENSE for licence details. */

/* framubuffer device, shell command
	are defined in fb/{linux.h,freebsd.h,netbsd.h}
	please check include file in yaft.c */

/* TERM value */
const char *term_name = "yaft-256color";

/* color: index number of color_palette[] (see color.h) */
enum {
	DEFAULT_FG           = 7,
	DEFAULT_BG           = 0,
	ACTIVE_CURSOR_COLOR  = 2,
	PASSIVE_CURSOR_COLOR = 1,
};

/* misc */
enum {
	DEBUG            = false,  /* write dump of input to stdout, debug message to stderr */
	TABSTOP          = 8,      /* hardware tabstop */
	LAZY_DRAW        = false,  /* don't draw when input data size is larger than BUFSIZE */
	BACKGROUND_DRAW  = false,   /* always draw even if vt is not active */
	WALLPAPER        = false,   /* copy framebuffer before startup, and use it as wallpaper */
	SUBSTITUTE_HALF  = 0x0020, /* used for missing glyph(single width): U+FFFD (REPLACEMENT CHARACTER)) */
	SUBSTITUTE_WIDE  = 0x3013, /* used for missing glyph(double width): U+3013 (GETA MARK) */
	REPLACEMENT_CHAR = 0x0020, /* used for malformed UTF-8 sequence   : U+FFFD (REPLACEMENT CHARACTER)  */
};
