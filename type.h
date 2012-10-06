/* See LICENSE for licence details. */
typedef struct framebuffer framebuffer;
typedef struct terminal terminal;
typedef struct escape escape;
typedef struct cell cell;
typedef struct glyph_t glyph_t;
typedef struct pair pair;
typedef struct margin margin;
typedef struct color_t color_t;
typedef struct color_pair color_pair;
typedef struct uchar uchar;
typedef struct state state;
typedef struct parm_t parm_t;

typedef struct termios termios;
typedef struct timeval timeval;
typedef struct winsize winsize;
typedef struct fb_fix_screeninfo finfo_t;
typedef struct fb_var_screeninfo vinfo_t;
typedef struct fb_cmap cmap_t;

struct pair {
	int x, y;
};

struct margin {
	int top, bottom;
};

struct color_t {
	u32 r, g, b;
};

struct color_pair {
	int fg, bg;
};

struct framebuffer {
	u8 *fp;						/* pointer of framebuffer(read only), assume bits per pixel == 32 */
	u8 *wall;					/* buffer for wallpaper */
	int fd;						/* file descriptor of framebuffer */
	pair res;					/* resolution (x, y) */
	long sc_size;				/* screen data size (bytes) */
	int line_length;			/* (pixel) */
	int bytes_per_pixel;
	cmap_t *cmap, *cmap_org;
};

struct cell {
	u16 code;					/* UCS2 */
	color_pair color;			/* color (fg, bg) */
	u8 attribute;				/* bold, underscore, etc... */
	int wide;					/* wide char flag: WIDE, NEXT_TO_WIDE, HALF */
};

struct escape {
	u8 buf[BUFSIZE];
	u8 *bp;
	int state;					/* esc state */
};

struct uchar {
	u32 code;					/* UCS4: but only print UCS2 (< 0xFFFF) */
	int length, count;
};

struct glyph_t {
	pair size;
	u32 *bitmap;
};

struct state {					/* for save/restore state */
	int mode;
	pair cursor;
	u8 attribute;
};

struct parm_t {
	int argc;
	char *argv[ESC_PARAMS];
};

struct terminal {
	int fd;						/* master fd */
	glyph_t *fonts[UCS2_CHARS];

	pair offset;				/* window offset (x, y) */
	int width, height;			/* terminal size (pixel) */

	int cols, lines;			/* terminal size (cell) */
	cell *cells;				/* pointer of each cell: cells[cols + lines * num_of_cols] */
	pair cell_size;				/* default glyph size */

	margin scroll;				/* scroll margin */
	pair cursor;				/* cursor pos (x, y) */

	bool *line_dirty;			/* dirty flag */
	bool *tabstop;				/* tabstop flag */

	int mode;					/* for set/reset mode */
	bool wrap;					/* whether auto wrap occured or not */
	state save_state;			/* for restore */

	color_pair color;			/* color (fg, bg) */
	u8 attribute;				/* bold, underscore, etc... */

	escape esc;					/* store escape sequence */
	uchar ucs;					/* store UTF-8 sequence */
};
