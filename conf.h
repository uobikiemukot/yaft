/* See LICENSE for licence details. */
/* font path */
static char *font_path[] = {
	".fonts/milkjf-jis0208.yaft",
	".fonts/milkjf-jis0201.yaft",
	".fonts/milkjf-iso8859.yaft",
	".fonts/ambiguous-half.yaft",
	NULL,
};

/* glyph alias file */
static char *glyph_alias = ".fonts/ambiguous-half.alias";

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
	OFFSET_X = 0,
	OFFSET_Y = 0,
	TERM_WIDTH = 0,
	TERM_HEIGHT = 0,
	TABSTOP = 8,
	SELECT_TIMEOUT = 20000, /* usec */
};
