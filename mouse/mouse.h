#include <linux/input.h>

enum {
	VALUE_PRESSED  = 1,
	VALUE_RELEASED = 0,
	MOUSE_COLOR   = 6,
};

struct cursor_t {
	int x, y;
};

struct mouse_t {
	int fd;
	struct cursor_t current;
	struct cursor_t pressed, released;
	bool is_pressed, do_paste;
};

const char *mouse_path = "/dev/input/by-path/pci-0000:00:1a.0-usb-0:1.2:1.1-event-mouse";

const char *ev_type[EV_CNT] = {
	[EV_SYN]       = "EV_SYN",
	[EV_KEY]       = "EV_KEY",
	[EV_REL]       = "EV_REL",
	[EV_ABS]       = "EV_ABS",
	[EV_MSC]       = "EV_MSC",
	[EV_SW]        = "EV_SW",
	[EV_LED]       = "EV_LED",
	[EV_SND]       = "EV_SND",
	[EV_REP]       = "EV_REP",
	[EV_FF]        = "EV_FF",
	[EV_PWR]       = "EV_PWR",
	[EV_FF_STATUS] = "EV_FF_STATUS",
	[EV_MAX]       = "EV_MAX",
};

const char *ev_code_syn[SYN_CNT] = {
	[SYN_REPORT]    = "SYN_REPORT",
	[SYN_CONFIG]    = "SYN_CONFIG",
	[SYN_MT_REPORT] = "SYN_MT_REPORT",
	[SYN_DROPPED]   = "SYN_DROPPED",
	[SYN_MAX]       = "SYN_MAX",
};

const char *ev_code_rel[REL_CNT] = {
	[REL_X]      = "REL_X",
	[REL_Y]      = "REL_Y",
	[REL_Z]      = "REL_Z",
	[REL_RX]     = "REL_RX",
	[REL_RY]     = "REL_RY",
	[REL_RZ]     = "REL_RZ",
	[REL_HWHEEL] = "REL_WHEEL",
	[REL_DIAL]   = "REL_DIAL",
	[REL_WHEEL]  = "REL_WHEEL",
	[REL_MISC]   = "REL_MISC",
	[REL_MAX]    = "REL_MAX",
};

const char *ev_code_key[KEY_CNT] = {
	[BTN_LEFT]    = "BTN_LEFT",
	[BTN_RIGHT]   = "BTN_RIGHT",
	[BTN_MIDDLE]  = "BTN_MIDDLE",
	[BTN_SIDE]    = "BTN_SIDE",
	[BTN_EXTRA]   = "BTN_EXTRA",
	[BTN_FORWARD] = "BTN_FORWARD",
	[BTN_BACK]    = "BTN_BACK",
	[BTN_TASK]    = "BTN_TASK",
	[KEY_MAX]     = "KEY_MAX",
};

/* mouse functions */
bool mouse_init(struct mouse_t *mouse)
{
	if ((mouse->fd = eopen(mouse_path, O_RDONLY)) < 0)
		return false;

	mouse->current.x  = mouse->current.y  = 0;
	mouse->pressed.x  = mouse->pressed.y  = 0;
	mouse->released.x = mouse->released.y = 0;
	mouse->is_pressed = false;
	mouse->do_paste   = false;

	return true;
}

void mouse_die(struct mouse_t *mouse)
{
	close(mouse->fd);
}

/*
void print_event(struct input_event *ie)
{
	switch (ie->type) {
	case EV_SYN:
		logging(DEBUG, "time:%ld.%06ld\ttype:%s\tcode:%s\tvalue:%d\n",
			ie->time.tv_sec, ie->time.tv_usec, ev_type[ie->type],
			ev_code_syn[ie->code], ie->value);
		break;
	case EV_REL:
		logging(DEBUG, "time:%ld.%06ld\ttype:%s\tcode:%s\tvalue:%d\n",
			ie->time.tv_sec, ie->time.tv_usec, ev_type[ie->type],
			ev_code_rel[ie->code], ie->value);
		break;
	case EV_KEY:
		logging(DEBUG, "time:%ld.%06ld\ttype:%s\tcode:%s\tvalue:%d\n",
			ie->time.tv_sec, ie->time.tv_usec, ev_type[ie->type],
			ev_code_key[ie->code], ie->value);
		break;
	default:
		break;
	}
}

void print_mouse_state(struct mouse_t *mouse)
{
	logging(DEBUG, "\033[1;1H\033[2Kcurrent(%d, %d) pressed(%d, %d) released(%d, %d)",
		mouse->current.x, mouse->current.y,
		mouse->pressed.x, mouse->pressed.y,
		mouse->released.x, mouse->released.y);
}
*/

void cursor(struct framebuffer_t *fb, struct mouse_t *mouse, struct input_event *ie)
{
	if (ie->code == REL_X)
		mouse->current.x += ie->value;

	if (mouse->current.x < 0)
		mouse->current.x = 0;
	else if (mouse->current.x >= fb->info.width)
		mouse->current.x = fb->info.width - 1;

	if (ie->code == REL_Y)
		mouse->current.y += ie->value;

	if (mouse->current.y < 0)
		mouse->current.y = 0;
	else if (mouse->current.y >= fb->info.height)
		mouse->current.y = fb->info.height - 1;
}

void button(struct framebuffer_t *fb, struct mouse_t *mouse, struct input_event *ie)
{
	(void) fb;

	if (ie->code == BTN_LEFT) {
		if (ie->value == VALUE_PRESSED) {
			mouse->pressed = mouse->current;
			mouse->is_pressed = true;
		}

		if (ie->value == VALUE_RELEASED) {
			mouse->released = mouse->current;
			mouse->is_pressed = false;
		}

	} else if (ie->code == BTN_RIGHT) {
		if (ie->value == VALUE_PRESSED)
			mouse->do_paste = true;
	}
}

/*
void sync(struct framebuffer_t *fb, struct mouse_t *mouse, struct input_event *ie)
{
	if (ie->code != SYN_REPORT)
		return;
}
*/

void (*mouse_handler[EV_CNT])(struct framebuffer_t *fb, struct mouse_t *mouse, struct input_event *ie) = {
	//[EV_SYN] = sync,
	[EV_REL] = cursor,
	[EV_KEY] = button, 
	[EV_MAX] = NULL, 
};

/*
void draw_cursor(struct framebuffer_t *fb, struct cursor_t *cursor, uint32_t color)
{
	memcpy(fb->fp + cursor->y * fb->info.line_length
		+ cursor->x * fb->info.bytes_per_pixel, &color, fb->info.bytes_per_pixel);
}
*/

static inline void draw_mouse(struct framebuffer_t *fb, struct terminal_t *term, struct mouse_t *mouse, int line)
{
	int pos, size, bdf_padding, glyph_width, margin_right;
	int mouse_col, mouse_line, pressed_col, pressed_line;
	int col, w, h;
	uint32_t pixel;
	struct color_pair_t color_pair;
	struct cell_t *cellp;

	mouse_col  = mouse->current.x / CELL_WIDTH;
	mouse_line = mouse->current.y / CELL_HEIGHT;
	//logging(DEBUG, "mouse (%d, %d) (col, line) = (%d, %d)\n",
		//mouse->current.x, mouse->current.y, mouse_col, mouse_line);

	pressed_col  = mouse->pressed.x / CELL_WIDTH;
	pressed_line = mouse->pressed.y / CELL_HEIGHT;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* target cell */
		cellp = &term->cells[line * term->cols + col];

		/* draw sixel pixmap */
		if (cellp->has_pixmap) {
			draw_sixel(fb, line, col, cellp->pixmap);
			continue;
		}

		/* copy current color_pair (maybe changed) */
		color_pair = cellp->color_pair;

		/* check wide character or not */
		glyph_width = (cellp->width == HALF) ? CELL_WIDTH: CELL_WIDTH * 2;
		bdf_padding = my_ceil(glyph_width, BITS_PER_BYTE) * BITS_PER_BYTE - glyph_width;
		if (cellp->width == WIDE)
			bdf_padding += CELL_WIDTH;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cellp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cellp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color_pair.fg = DEFAULT_BG;
			color_pair.bg = (!vt_active && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		/* check mouse position */
		if ((mouse_col == col && mouse_line == line)
			|| (mouse->is_pressed && pressed_col == col && pressed_line == line)) {
			color_pair.fg = DEFAULT_BG;
			color_pair.bg = MOUSE_COLOR;
		}

		for (h = 0; h < CELL_HEIGHT; h++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((h == (CELL_HEIGHT - 1)) && (cellp->attribute & attr_mask[ATTR_UNDERLINE]))
				color_pair.bg = color_pair.fg;

			for (w = 0; w < CELL_WIDTH; w++) {
				pos = (term->width - 1 - margin_right - w) * fb->info.bytes_per_pixel
					+ (line * CELL_HEIGHT + h) * fb->info.line_length;

				/* set color palette */
				if (cellp->glyphp->bitmap[h] & (0x01 << (bdf_padding + w)))
					pixel = fb->real_palette[color_pair.fg];
				else if (fb->wall && color_pair.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->info.bytes_per_pixel);
				else
					pixel = fb->real_palette[color_pair.bg];

				/* update copy buffer only */
				memcpy(fb->buf + pos, &pixel, fb->info.bytes_per_pixel);
			}
		}
	}

	/* actual display update (bit blit) */
	pos = (line * CELL_HEIGHT) * fb->info.line_length;
	size = CELL_HEIGHT * fb->info.line_length;
	memcpy(fb->fp + pos, fb->buf + pos, size);

	/* TODO: page flip
		if fb_fix_screeninfo.ypanstep > 0, we can use hardware panning.
		set fb_fix_screeninfo.{yres_virtual,yoffset} and call ioctl(FBIOPAN_DISPLAY)
		but drivers  of recent hardware (inteldrmfb, nouveaufb, radeonfb) don't support...
		(maybe we can use this by using libdrm) */
	/* TODO: vertical synchronizing */

	term->line_dirty[line] = (mouse_line == line) ? true: false;

	if (mouse->is_pressed)
		term->line_dirty[pressed_line] = true;
}

static inline void refresh_mouse(struct framebuffer_t *fb, struct terminal_t *term, struct mouse_t *mouse)
{
	term->line_dirty[mouse->current.y / CELL_HEIGHT] = true;

	for (int line = 0; line < term->lines; line++) {
		if (term->line_dirty[line]) {
			draw_mouse(fb, term, mouse, line);
		}
	}
}

void mouse_paste(struct terminal_t *term, struct mouse_t *mouse)
{
	int start, end, size;
	char utf8_buf[UTF8_LEN_MAX + 1];
	struct cell_t *cellp;

	start = (mouse->pressed.y / CELL_HEIGHT) * term->cols + (mouse->pressed.x / CELL_WIDTH);
	end   = (mouse->released.y / CELL_HEIGHT) * term->cols + (mouse->released.x / CELL_WIDTH);

	for (int cell = start; cell <= end; cell++) {
		cellp = &term->cells[cell];

		if (cellp->width == NEXT_TO_WIDE)
			continue;

		size = utf8_encode(cellp->glyphp->code, utf8_buf);
		ewrite(term->fd, utf8_buf, size);
	}
}
