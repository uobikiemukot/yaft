/* See LICENSE for licence details. */
/* framubuffer device */
char *fb_path = "/dev/tty";

/* shell */
char *shell_cmd = "/bin/bash";

/* TERM value */
char *term_name = "TERM=yaft-256color";

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
	SUBSTITUTE_HALF = 0x20, /* SPACE (0x20) or REPLACEMENT CHARACTER (0xFFFD) */
	SUBSTITUTE_WIDE = 0x3000, /* IDEOGRAPHIC SPACE */
	/* must be the same setting as current video mode */
	MODE = 279,
};
