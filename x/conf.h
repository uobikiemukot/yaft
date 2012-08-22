/* See LICENSE for licence details. */
/* font path */
static char *font_path[] = {
	"/home/nak/haru/.fonts/milk.yaft",
	"/home/nak/haru/.fonts/ambhalf.fix",
	NULL,
};

/* glyph alias file */
static char *glyph_alias = "/home/nak/haru/.fonts/ambiguous-half.alias";

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
	OFFSET_X = 600,
	OFFSET_Y = 600,
	TERM_WIDTH = 640,
	TERM_HEIGHT = 384,
	TABSTOP = 8,
	SELECT_TIMEOUT = 20000,
};
