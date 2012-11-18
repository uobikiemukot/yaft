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

/* misc */
enum {
	DEBUG = false,
	TABSTOP = 8,
	//SUBSTITUTE_HALF = 0x20, /* SPACE */
	SUBSTITUTE_HALF = 0xFFFD, /* REPLACEMENT CHARACTER: ambiguous width */
	SUBSTITUTE_WIDE = 0x3000, /* IDEOGRAPHIC SPACE */
	WIDTH = 640,
	HEIGHT = 384,
};

const struct keydef key[] = {
	{ XK_BackSpace, XK_NO_MOD, "\177"    },
	{ XK_Up,        XK_NO_MOD, "\033[A"  },
	{ XK_Down,      XK_NO_MOD, "\033[B"  },
	{ XK_Right,     XK_NO_MOD, "\033[C"  },
	{ XK_Left,      XK_NO_MOD, "\033[D"  },
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

/*
kb2=\E[G, kbs=\177, kcbt=\E[Z, kcub1=\E[D,
kcud1=\E[B, kcuf1=\E[C, kcuu1=\E[A, kdch1=\E[3~,
kend=\E[4~, kf1=\E[[A, kf10=\E[21~, kf11=\E[23~,
kf12=\E[24~, kf13=\E[25~, kf14=\E[26~, kf15=\E[28~,
kf16=\E[29~, kf17=\E[31~, kf18=\E[32~, kf19=\E[33~,
kf2=\E[[B, kf20=\E[34~, kf3=\E[[C, kf4=\E[[D, kf5=\E[[E,
kf6=\E[17~, kf7=\E[18~, kf8=\E[19~, kf9=\E[20~,
khome=\E[1~, kich1=\E[2~, kmous=\E[M, knp=\E[6~, kpp=\E[5~,
kspd=^Z,
*/
