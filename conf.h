/* See LICENSE for licence details. */
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
	LAZY_DRAW        = true,   /* don't draw when input data size is larger than BUFSIZE */
	BACKGROUND_DRAW  = false,  /* always draw even if vt is not active */
	//WALLPAPER        = true,  /* copy framebuffer before startup, and use it as wallpaper */
	SUBSTITUTE_HALF  = 0x0020, /* used for missing glyph (single width): U+0020 (SPACE) */
	SUBSTITUTE_WIDE  = 0x3000, /* used for missing glyph (double width): U+3000 (IDEOGRAPHIC SPACE) */
	REPLACEMENT_CHAR = 0x003F, /* used for malformed UTF-8 sequence    : U+003F (QUESTION MARK)  */
};

/* TERM value */
const char *term_name = "yaft-256color";

/* framubuffer device */
#if defined(__linux__)
	const char *fb_path = "/dev/fb0";
#elif defined(__FreeBSD__)
	const char *fb_path = "/dev/tty";
#elif defined(__NetBSD__)
	const char *fb_path = "/dev/ttyE0";
#elif defined(__OpenBSD__)
	const char *fb_path = "/dev/ttyC0";
#endif
//const char *fb_path = "/dev/graphics/fb0"; /* for Android */

/* shell: refer SHELL environment at first */
#if defined(__linux__) || defined(__MACH__)
	const char *shell_cmd = "/bin/bash";
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	const char *shell_cmd = "/bin/csh";
#endif
//const char *shell_cmd = "/system/bin/sh"; /* for Android */
