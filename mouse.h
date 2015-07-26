#include <linux/input.h>

enum {
	VALUE_PRESSED  = 1,
	VALUE_RELEASED = 0,
	UTF8_LEN_MAX   = 3,
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

void (*mouse_handler[EV_CNT])(struct framebuffer_t *fb, struct mouse_t *mouse, struct input_event *ie) = {
	[EV_REL] = cursor,
	[EV_KEY] = button,
	[EV_MAX] = NULL,
};

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

	pressed_col  = mouse->pressed.x / CELL_WIDTH;
	pressed_line = mouse->pressed.y / CELL_HEIGHT;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* target cell */
		cellp = &term->cells[line][col];

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
			color_pair.bg = MOUSE_CURSOR_COLOR;
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

int utf8_encode(uint32_t ucs, char utf8_buf[UTF8_LEN_MAX + 1])
{
	if ((0xD800 <= ucs && ucs <= 0xDFFF)    /* used as surrogate pair in UTF-16 */
		|| (0xFDD0 <= ucs && ucs <= 0xFDEF) /* Noncharacter */
		|| ucs == 0xFFFE                    /* conflict byte order mark (U+FEFF) */
		|| ucs == 0xFFFF                    /* Noncharacter U+FFFF ("useful for internal purposes as sentinels") */
		|| ucs > 0xFFFF) {                  /* UCS2 (Unicode BMP): 0x0000 - 0xFFFF */
		/* invalid codepoint */
		return -1;
	}

	if (ucs <= 0x7F) {
		/* ASCII Character */
		utf8_buf[0] = (ucs & 0x7F);
		utf8_buf[1] = '\0';
		return 1;
	} else if (0x80 <= ucs && ucs <= 0x7FF) {
		/* 2 byte sequence */
		utf8_buf[0] = 0xC0 | ((ucs >> 6) & 0x1F);
		utf8_buf[1] = 0x80 | (ucs & 0x3F);
		utf8_buf[2] = '\0';
		return 2;
	} else if (0x800 <= ucs && ucs <= 0xFFFF) {
		/* 3 byte sequence */
		utf8_buf[0] = 0xE0 | ((ucs >> 12) & 0x0F);
		utf8_buf[1] = 0x80 | ((ucs >>  6) & 0x3F);
		utf8_buf[2] = 0x80 | (ucs & 0x3F);
		utf8_buf[3] = '\0';
		return 3;
	}
	/* illegal codepoint */
	return -1;
}

void mouse_paste(struct terminal_t *term, struct mouse_t *mouse)
{
	int x, y, start, end, tmp, size;
	char utf8_buf[UTF8_LEN_MAX + 1];
	struct cell_t *cellp;

	start = (mouse->pressed.y / CELL_HEIGHT) * term->cols + (mouse->pressed.x / CELL_WIDTH);
	end   = (mouse->released.y / CELL_HEIGHT) * term->cols + (mouse->released.x / CELL_WIDTH);

	if (start > end) {
		tmp   = start;
		start = end;
		end   = tmp;
	}

	for (int pos = start; pos <= end; pos++) {
		x = pos % term->cols;
		y = pos / term->cols;
		cellp = &term->cells[y][x];

		if (cellp->width == NEXT_TO_WIDE)
			continue;

		size = utf8_encode(cellp->glyphp->code, utf8_buf);
		ewrite(term->fd, utf8_buf, size);
	}
}
