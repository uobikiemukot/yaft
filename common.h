/* See LICENSE for licence details. */
#define _XOPEN_SOURCE 600
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

enum char_code {
	BEL = 0x07, ESC = 0x1B, SPACE = 0x20,
	BACKSLASH = 0x5C, DEL = 0x7F,
};

enum {
	/* misc */
	BITS_PER_BYTE = 8,
	BUFSIZE = 1024,				/* read, esc, various buffer size */
	ESC_PARAMS = 16,			/* max parameters of csi/osc sequence */
	COLORS = 256,				/* num of color */
	UCS2_CHARS = 0x10000,		/* number of UCS2 glyph */
	CTRL_CHARS = 0x20,			/* number of control character */
	ESC_CHARS = 0x80,			/* number of final character of escape sequence */
	DEFAULT_CHAR = SPACE,		/* erase char, and used for cell_size: SPACE */
	RESET = 0x00,				/* reset for char_attr, term_mode, esc_state */
	BRIGHT_INC = 8,				/* incrment value: from dark color to light color */
};

enum width_flag {
	NEXT_TO_WIDE = 0,
	HALF,
	WIDE,
};

enum char_attr {
	BOLD = 1,					/* bright foreground */
	UNDERLINE = 4,
	BLINK = 5,					/* bright background */
	REVERSE = 7,
};

const u8 attr_mask[] = {
	0x00, 0x01, 0x00, 0x00,		/* 0:none	  1:bold  2:none 3:none */
	0x02, 0x04, 0x00, 0x08,		/* 4:underline 5:blink 6:none 7:reverse */
};

u32 bit_mask[] = {
	0x00,
	0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF,
	0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF,
	0x1FFFF, 0x3FFFF, 0x7FFFF, 0xFFFFF, 0x1FFFFF, 0x3FFFFF, 0x7FFFFF, 0xFFFFFF,
	0x1FFFFFF, 0x3FFFFFF, 0x7FFFFFF, 0xFFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

enum term_mode {
	ORIGIN = 0x01,				/* origin mode: DECOM */
	CURSOR = 0x02,				/* cursor visible: DECTECM */
	AMRIGHT = 0x04,				/* auto wrap: DECAWM */
};

enum esc_state {
	STATE_ESC = 1,				/* 0x1B, \033, ESC */
	STATE_CSI,					/* ESC [ */
	STATE_OSC,					/* ESC ] */
};

#include "conf.h"				/* user configuration */
#include "type.h"				/* struct and typedef */
#include "color.h"				/* 256color definition */

void (*ctrl_func[CTRL_CHARS])(terminal * term, void *arg);
void (*esc_func[ESC_CHARS])(terminal * term, void *arg);
void (*csi_func[ESC_CHARS])(terminal * term, void *arg);
