/* wallpaper */
static char *wall_path = "~/.yaft/karmic-gray.ppm";
//static char *wall_path = NULL;

/* font path */
static char *font_path = "~/.yaft/milk.yaft";

/* framubuffer device */
static char *fb_path = "/dev/fb0";

/* shell */
static char *shell_cmd = "/bin/bash";

/* TERM value */
static char *term_name = "yaft";

/* color: you can specify colors defined in color.h
	and 24bit rgb color like 0xFFFFFF */
enum {
	DEFAULT_FG = GRAY,
	DEFAULT_BG = BLACK,
	CURSOR_COLOR = GREEN,
};

/* terminal setting */
enum {
	DUMP = true,
	DEBUG = true,
	LAZYDRAW = true,
	/*
	OFFSET_X = 0,
	OFFSET_Y = 0,
	TERM_WIDTH = 1280,
	TERM_HEIGHT = 1024,
	*/
	OFFSET_X = 32,
	OFFSET_Y = 32,
	TERM_WIDTH = 640, // 8 * 80
	TERM_HEIGHT = 384, // 16 * 24
	/*
	*/
	TABSTOP = 8,
	INTERVAL = 1000000, /* polling interval(usec) */
	//SCROLLBACK = 1024, /* not implemented */
};
