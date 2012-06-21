/* font path */
static char *font_path = "~/.fonts/shnm.yaft";

/* framubuffer device */
static char *fb_path = "/dev/fb0";

/* shell */
static char *shell_cmd = "/bin/bash";

/* TERM value */
static char *term_name = "TERM=yaft-256color";

/* color: you can specify colors defined in color.h
	and 24bit rgb color like 0xFFFFFF */
enum {
	DEFAULT_FG = GRAY,
	DEFAULT_BG = BLACK,
	CURSOR_COLOR = GREEN,
};

/* terminal setting */
enum {
	DUMP = false,
	DEBUG = false,
	LAZYDRAW = true,
	WALLPAPER = true,
	OFFSET_X = 32,
	OFFSET_Y = 32,
	TERM_WIDTH = 0,
	TERM_HEIGHT = 0,
	TABSTOP = 8,
	INTERVAL = 1000000,			/* polling interval(usec) */
};
