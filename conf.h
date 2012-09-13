/* See LICENSE for licence details. */
/* font path */
char *font_path[] = {
	"/usr/share/yaft/fonts/milkjf-jis0208.yaft",
	"/usr/share/yaft/fonts/milkjf-jis0201.yaft",
	"/usr/share/yaft/fonts/milkjf-iso8859.yaft",
	"/usr/share/yaft/fonts/ambiguous-half.yaft",
	NULL,
};

/* glyph alias file */
char *glyph_alias = "/usr/share/yaft/alias/ambiguous-half.alias";

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
	SELECT_TIMEOUT = 20000,
};
