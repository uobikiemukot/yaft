/* See LICENSE for licence details. */
/* framubuffer device */
char *fb_path = "/dev/fb0";

/* shell */
char *shell_cmd = "/bin/bash";

/* TERM value */
char *term_name = "TERM=yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	CURSOR_COLOR = 2,
};

/* terminal setting */
enum {
	DEBUG = false,
	TABSTOP = 8,
	SUBSTITUTE_HALF = 0x20, /* SPACE */
	//SUBSTITUTE_HALF = 0xFFFD, /* REPLACEMENT CHARACTER: ambiguous width */
	SUBSTITUTE_WIDE = 0x3000, /* IDEOGRAPHIC SPACE */
};
