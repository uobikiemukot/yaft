/* See LICENSE for licence details. */
/* font path */
static char *font_path = "./fonts/default.yaft";

/* framubuffer device */
static char *fb_path = "/dev/fb0";

/* shell */
static char *shell_cmd = "/bin/bash";

/* TERM value */
static char *term_name = "TERM=yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	CURSOR_COLOR = 2,
};

/* terminal setting */
enum {
	DUMP = false,
	DEBUG = false,
	LAZYDRAW = true,
	WALLPAPER = false,
	OFFSET_X = 32,
	OFFSET_Y = 32,
	TERM_WIDTH = 0,
	TERM_HEIGHT = 0,
	TABSTOP = 8,
	INTERVAL = 1000000,			/* polling interval(usec) */
};
