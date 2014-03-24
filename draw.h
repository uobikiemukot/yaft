/* See LICENSE for licence details. */
int get_rotated_pos(struct framebuffer *fb, struct terminal *term, int x, int y)
{
	int p, q;
	long pos;

	if (fb->rotate == CLOCKWISE) {
		p = y;
		q = (term->width - 1) - x;
	}
	else if (fb->rotate == UPSIDE_DOWN) {
		p = (term->width - 1) - x;
		q = (term->height - 1) - y;
	}
	else if (fb->rotate == COUNTER_CLOCKWISE) {
		p = (term->height - 1) - y;
		q = x;
	}
	else { /* rotate: NORMAL */
		p = x;
		q = y;
	}

	pos = p * fb->bpp + q * fb->line_length;
	if (pos < 0 || pos >= fb->screen_size) {
		fprintf(stderr, "(%d, %d) -> (%d, %d) term:(%d, %d) res:(%d, %d) pos:%ld\n",
			x, y, p, q, term->width, term->height, fb->res.x, fb->res.y, pos);
		exit(EXIT_FAILURE);
	}

	return pos;
}

void draw_line(struct framebuffer *fb, struct terminal *term, int line)
{
	int copy_size, pos, bit_shift, margin_right;

	int col, glyph_width_offset, glyph_height_offset;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

	copy_size = (fb->rotate == CLOCKWISE || fb->rotate == COUNTER_CLOCKWISE) ?
		CELL_HEIGHT * fb->bpp: CELL_HEIGHT * fb->line_length;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* get cell color and glyph */
		cp = &term->cells[col + line * term->cols];
		color = cp->color;
		gp = cp->gp;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color.fg = DEFAULT_BG;
			color.bg = (!tty.visible && tty.background_draw) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		for (glyph_height_offset = 0; glyph_height_offset < CELL_HEIGHT; glyph_height_offset++) {
			if ((glyph_height_offset == (CELL_HEIGHT - 1)) && (cp->attribute & attr_mask[UNDERLINE]))
				color.bg = color.fg;

			for (glyph_width_offset = 0; glyph_width_offset < CELL_WIDTH; glyph_width_offset++) {
				pos = get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset,
					line * CELL_HEIGHT + glyph_height_offset);

				if (cp->width == WIDE)
					bit_shift = glyph_width_offset + CELL_WIDTH;
				else
					bit_shift = glyph_width_offset;

				/* set color palette */
				if (gp->bitmap[glyph_height_offset] & (0x01 << bit_shift))
					pixel = term->color_palette[color.fg];
				else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->bpp);
				else
					pixel = term->color_palette[color.bg];

				memcpy(fb->buf + pos, &pixel, fb->bpp);
			}
		}

		if (fb->rotate == CLOCKWISE) {
			for (glyph_width_offset = 0; glyph_width_offset < CELL_WIDTH; glyph_width_offset++) {
				pos = get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset, line * CELL_HEIGHT);
				memcpy(fb->fp + pos, fb->buf + pos, copy_size);
			}
		} else if (fb->rotate == COUNTER_CLOCKWISE) {
			for (glyph_width_offset = 0; glyph_width_offset < CELL_WIDTH; glyph_width_offset++) {
				pos = get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset, (line + 1) * CELL_HEIGHT - 1);
				memcpy(fb->fp + pos, fb->buf + pos, copy_size);
			}
		}
	}

	if (fb->rotate == NORMAL) {
		pos = get_rotated_pos(fb, term, 0, line * CELL_HEIGHT);
		memcpy(fb->fp + pos, fb->buf + pos, copy_size);
	} else if (fb->rotate == UPSIDE_DOWN) {
		pos = get_rotated_pos(fb, term, term->width - 1, (line + 1) * CELL_HEIGHT - 1);
		memcpy(fb->fp + pos, fb->buf + pos, copy_size);
	}

	term->line_dirty[line] = ((term->mode & MODE_CURSOR) && term->cursor.y == line) ? true: false;
}

void refresh(struct framebuffer *fb, struct terminal *term)
{
	int line;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (line = 0; line < term->lines; line++) {
		if (term->line_dirty[line])
			draw_line(fb, term, line);
	}
}
