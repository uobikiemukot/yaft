/* See LICENSE for licence details. */
/* framubuffer device */
const char *fb_path = "/dev/fb0";

/* shell */
const char *shell_cmd = "/bin/bash";

/* TERM value */
const char *term_name = "yaft-256color";

/* color: index number of color_palette[] in color.h */
enum {
	DEFAULT_FG = 7,
	DEFAULT_BG = 0,
	CURSOR_COLOR = 2,
	CURSOR_INACTIVE_COLOR = 8,
};

/* misc */
enum {
	DEBUG = false,
	TABSTOP = 8,
	SUBSTITUTE_HALF = 0x20,   /* used for missing glyph: SPACE (0x20) */
	SUBSTITUTE_WIDE = 0x3000, /* used for missing glyph: IDEOGRAPHIC SPACE(0x3000) */
	REPLACEMENT_CHAR = 0xFFFD,/* used for malformed UTF-8 sequence */
	WIDTH = 640,
	HEIGHT = 384,
};

const struct keydef key[] = {
	{ XK_BackSpace, XK_NO_MOD, "\177"    },
	{ XK_Up,        XK_NO_MOD, "\033[A"  },
	{ XK_Down,      XK_NO_MOD, "\033[B"  },
	{ XK_Right,     XK_NO_MOD, "\033[C"  },
	{ XK_Left,      XK_NO_MOD, "\033[D"  },
	{ XK_Begin,     XK_NO_MOD, "\033[G"  },
	{ XK_Home,      XK_NO_MOD, "\033[1~" },
	{ XK_Insert,    XK_NO_MOD, "\033[2~" },
	{ XK_Delete,    XK_NO_MOD, "\033[3~" },
	{ XK_End,       XK_NO_MOD, "\033[4~" },
	{ XK_Prior,     XK_NO_MOD, "\033[5~" },
	{ XK_Next,      XK_NO_MOD, "\033[6~" },
	{ XK_F1,        XK_NO_MOD, "\033[[A" },
	{ XK_F2,        XK_NO_MOD, "\033[[B" },
	{ XK_F3,        XK_NO_MOD, "\033[[C" },
	{ XK_F4,        XK_NO_MOD, "\033[[D" },
	{ XK_F5,        XK_NO_MOD, "\033[[E" },
	{ XK_F6,        XK_NO_MOD, "\033[17~"},
	{ XK_F7,        XK_NO_MOD, "\033[18~"},
	{ XK_F8,        XK_NO_MOD, "\033[19~"},
	{ XK_F9,        XK_NO_MOD, "\033[20~"},
	{ XK_F10,       XK_NO_MOD, "\033[21~"},
	{ XK_F11,       XK_NO_MOD, "\033[23~"},
	{ XK_F12,       XK_NO_MOD, "\033[24~"},
	{ XK_F13,       XK_NO_MOD, "\033[25~"},
	{ XK_F14,       XK_NO_MOD, "\033[26~"},
	{ XK_F15,       XK_NO_MOD, "\033[28~"},
	{ XK_F16,       XK_NO_MOD, "\033[29~"},
	{ XK_F17,       XK_NO_MOD, "\033[31~"},
	{ XK_F18,       XK_NO_MOD, "\033[32~"},
	{ XK_F19,       XK_NO_MOD, "\033[33~"},
	{ XK_F20,       XK_NO_MOD, "\033[34~"},
};

/* not assigned:
	kcbt=\E[Z,kmous=\E[M,kspd=^Z,
*/
