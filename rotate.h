/* See LICENSE for licence details. */
int my_ceil(int value, int division)
{
	return ((value + division - 1) / division);
}

void set_bit_normal(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, char *src)
{
	int i, shift, glyph_width;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = cp->gp;
	glyph_width = gp->width * cell_width;
	shift = ((glyph_width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	if ((term->mode & MODE_CURSOR && y == term->cursor.y) /* cursor */
		&& (x == term->cursor.x || (cp->wide == WIDE && (x + 1) == term->cursor.x))) {
		color.fg = DEFAULT_BG;
		color.bg = CURSOR_COLOR;
	}

	if ((offset == (cell_height - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;
	
	for (i = 0; i < glyph_width; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1)))
			pixel = fb->color_palette[color.fg];
		else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
			memcpy(&pixel, fb->wall + (i + x * cell_width) * fb->bpp
				+ (offset + y * cell_height) * fb->line_length, fb->bpp);
		else
			pixel = fb->color_palette[color.bg];
		memcpy(src + i * fb->bpp, &pixel, fb->bpp);
	}
}

void draw_line_normal(struct framebuffer *fb, struct terminal *term, int y)
{
	int offset, x, size, pos;
	char *src, *dst;

	pos = (y * cell_height) * fb->line_length;
	size = fb->res.x * fb->bpp;

	for (offset = 0; offset < cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			fb->set_bit(fb, term, y, x, offset,
				fb->buf + pos + x * cell_width * fb->bpp + offset * fb->line_length);
		src = fb->buf + pos + offset * fb->line_length;
		dst = fb->fp + pos + offset * fb->line_length;
		memcpy(dst, src, size);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;
}

void draw_line_clockwise(struct framebuffer *fb, struct terminal *term, int line)
{
	int y, pos, copy_size;
	char *src, *dst;

	int col, glyph_width_offset, glyph_height_offset, glyph_width; //bits_per_glyph_width;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

	/* calc address of framebuffer */
	/*
	terminal pos
		(fb->res.x, line * cell_height)
	framebuffer pos
		((line * cell_height), 0)
	*/
	pos = (line * cell_height) * fb->bpp;
	copy_size = cell_height * fb->bpp;

	for (y = 0; y < fb->res.x; y++) {
		col =  term->cols - 1 - (y / cell_width);
		for (glyph_height_offset = 0; glyph_height_offset < cell_height; glyph_height_offset++) {
			/* check cell */
			cp = &term->cells[col + line * term->cols];
			color = cp->color;
			if (cp->wide == NEXT_TO_WIDE)
				continue;

			/* check glyph */
			gp = cp->gp;
			glyph_width = gp->width * cell_width; /* gp->width: 1 or 2 */
			//bits_per_glyph_width = my_ceil(glyph_width, BITS_PER_BYTE) * BITS_PER_BYTE;
			glyph_width_offset = (y / cell_width);

			/* check cursor position */
			if ((term->mode & MODE_CURSOR && line == term->cursor.y)
				&& (col == term->cursor.x || (cp->wide == WIDE && (col + 1) == term->cursor.x))) {
				color.fg = DEFAULT_BG;
				color.bg = CURSOR_COLOR;
			}

			/* set underline */
			if ((glyph_height_offset == (cell_height - 1)) && (cp->attribute & attr_mask[UNDERLINE]))
				color.bg = color.fg;
			
			/* set color palette */
			if (gp->bitmap[glyph_height_offset] & (0x01 << glyph_width_offset))
				pixel = fb->color_palette[color.fg];
			else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
				memcpy(&pixel, fb->wall + (col * cell_width) * fb->bpp
					+ (glyph_height_offset + line * cell_height) * fb->line_length, fb->bpp);
			else
				pixel = fb->color_palette[color.bg];

			memcpy(fb->buf + glyph_height_offset * fb->bpp + y * fb->line_length, &pixel, fb->bpp);
		}
		src = fb->buf + pos + y * fb->line_length;
		dst = fb->fp + pos + y * fb->line_length;
		memcpy(dst, src, copy_size);
	}
}

void set_bit_upside_down(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, char *src)
{
}

void draw_line_upside_down(struct framebuffer *fb, struct terminal *term, int y)
{
}

void set_bit_counter_clockwise(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, char *src)
{
}

void draw_line_counter_clockwise(struct framebuffer *fb, struct terminal *term, int y)
{
}

void check_rotate(struct framebuffer *fb)
{
	int tmp;

	if (ROTATE == CLOCKWISE || ROTATE == COUNTER_CLOCKWISE) {
		tmp = fb->res.x;
		fb->res.x = fb->res.y;
		fb->res.y = tmp;
	}

	if (ROTATE == CLOCKWISE) {
		//fb->set_bit = set_bit_clockwise;
		fb->draw_line = draw_line_clockwise;
	}
	else if (ROTATE == UPSIDE_DOWN) {
		fb->set_bit = set_bit_upside_down;
		fb->draw_line = draw_line_upside_down;
	}
	else if (ROTATE == COUNTER_CLOCKWISE) {
		fb->set_bit = set_bit_counter_clockwise;
		fb->draw_line = draw_line_counter_clockwise;
	}
	else { /* rotate: NORMAL */
		fb->set_bit = set_bit_normal;
		fb->draw_line = draw_line_normal;
	}
}
