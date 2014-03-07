/* See LICENSE for licence details. */
char *load_wallpaper(struct framebuffer *fb)
{
	char *ptr;

	ptr = (char *) emalloc(fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

struct fb_cmap *cmap_create(struct fb_var_screeninfo *vinfo)
{
	struct fb_cmap *cmap;

	cmap = (struct fb_cmap *) emalloc(sizeof(struct fb_cmap));
	cmap->start = 0;
	cmap->len = COLORS;
	cmap->red = (uint16_t *) emalloc(sizeof(uint16_t) * COLORS);
	cmap->green = (uint16_t *) emalloc(sizeof(uint16_t) * COLORS);
	cmap->blue = (uint16_t *) emalloc(sizeof(uint16_t) * COLORS);
	cmap->transp = NULL;

	return cmap;
}

void cmap_die(struct fb_cmap *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		free(cmap->transp);
		free(cmap);
	}
}

void get_rgb(int i, struct color_t *color)
{
	color->r = (color_list[i] >> 16) & bit_mask[8];
	color->g = (color_list[i] >> 8) & bit_mask[8];
	color->b = (color_list[i] >> 0) & bit_mask[8];
}

uint32_t bit_reverse(uint32_t val, int bits)
{
	uint32_t ret = val;
	int shift = bits - 1;

	for (val >>= 1; val; val >>= 1) {
		ret <<= 1;
		ret |= val & 1;
		shift--;
	}

	return ret <<= shift;
}

void cmap_init(struct framebuffer *fb, struct fb_var_screeninfo *vinfo)
{
	int i;
	uint16_t r, g, b;
	struct color_t color;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		get_rgb(i, &color);
		r = (color.r << BITS_PER_BYTE) | color.r;
		g = (color.g << BITS_PER_BYTE) | color.g;
		b = (color.b << BITS_PER_BYTE) | color.b;

		*(fb->cmap->red + i) = (vinfo->red.msb_right) ?
			bit_reverse(r, 16) & bit_mask[16]: r;
		*(fb->cmap->green + i) = (vinfo->green.msb_right) ?
			bit_reverse(g, 16) & bit_mask[16]: g;
		*(fb->cmap->blue + i) = (vinfo->blue.msb_right) ?
			bit_reverse(b, 16) & bit_mask[16]: b;
	}

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0)
		fatal("ioctl: FBIOPUTCMAP failed");
}

uint32_t get_color(struct fb_var_screeninfo *vinfo, int i)
{
	uint32_t r, g, b;
	struct color_t color;

	if (vinfo->bits_per_pixel == 8)
		return i;

	get_rgb(i, &color);
	r = color.r >> (BITS_PER_BYTE - vinfo->red.length);
	g = color.g >> (BITS_PER_BYTE - vinfo->green.length);
	b = color.b >> (BITS_PER_BYTE - vinfo->blue.length);

	if (vinfo->red.msb_right)
		r = bit_reverse(r, vinfo->red.length) & bit_mask[vinfo->red.length];
	if (vinfo->green.msb_right)
		g = bit_reverse(g, vinfo->green.length) & bit_mask[vinfo->green.length];
	if (vinfo->blue.msb_right)
		b = bit_reverse(b, vinfo->blue.length) & bit_mask[vinfo->blue.length];

	return (r << vinfo->red.offset)
		+ (g << vinfo->green.offset)
		+ (b << vinfo->blue.offset);
}

void fb_init(struct framebuffer *fb)
{
	int i;
	char *path, *env;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) < 0)
		fatal("ioctl: FBIOGET_FSCREENINFO failed");

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
		fatal("ioctl: FBIOGET_VSCREENINFO failed");

	fb->res.x = vinfo.xres;
	fb->res.y = vinfo.yres;
	fb->screen_size = finfo.smem_len;
	fb->line_length = finfo.line_length;

	if ((finfo.visual == FB_VISUAL_TRUECOLOR || finfo.visual == FB_VISUAL_DIRECTCOLOR)
		&& (vinfo.bits_per_pixel == 15 || vinfo.bits_per_pixel == 16
		|| vinfo.bits_per_pixel == 24 || vinfo.bits_per_pixel == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = (vinfo.bits_per_pixel + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR
		&& vinfo.bits_per_pixel == 8) {
		fb->cmap = cmap_create(&vinfo);
		fb->cmap_org = cmap_create(&vinfo);
		cmap_init(fb, &vinfo);
		fb->bpp = 1;
	}
	else
		/* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		fb->color_palette[i] = get_color(&vinfo, i);

	fb->fp = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (char *) emalloc(fb->screen_size);

	if ((env = getenv("YAFT")) != NULL && strncmp(env, "wall", 4) == 0 && fb->bpp > 1)
		fb->wall = load_wallpaper(fb);
	else
		fb->wall = NULL;
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		ioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org); /* not fatal */
		cmap_die(fb->cmap_org);
	}
	free(fb->wall);
	free(fb->buf);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}

int get_rotated_pos(struct framebuffer *fb, struct terminal *term, int x, int y)
{
	int p, q;
	long pos;

	if (ROTATE == CLOCKWISE) {
		p = y;
		q = (term->width - 1) - x;
	}
	else if (ROTATE == UPSIDE_DOWN) {
		p = (term->width - 1) - x;
		q = (term->height - 1) - y;
	}
	else if (ROTATE == COUNTER_CLOCKWISE) {
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

	/*
		1280(width) x 1024(height) = 1310720 pixels

		     0                     1279
		     +-- ...              --+
		     |                      |
		1280 +-- ...              --+2559
		     |                      |
		     .                      .
		     .                      .
		     +-- ...              --+
		    1309440                1310719

		cell size: 8x16
		term cell: line 0 - 63 col 0 - 159
	*/

	pos = get_rotated_pos(fb, term, term->width - 1, line * cell_height);
	copy_size = (ROTATE == CLOCKWISE || ROTATE == COUNTER_CLOCKWISE) ?
		cell_height * fb->bpp: cell_height * fb->line_length;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * cell_width;

		/* get cell color and glyph */
		cp = &term->cells[col + line * term->cols];
		color = cp->color;
		gp = cp->gp;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cp->wide == WIDE && (col + 1) == term->cursor.x)
			|| (cp->wide == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color.fg = DEFAULT_BG;
			color.bg = CURSOR_COLOR;
		}

		for (glyph_height_offset = 0; glyph_height_offset < cell_height; glyph_height_offset++) {
			if ((glyph_height_offset == (cell_height - 1)) && (cp->attribute & attr_mask[UNDERLINE]))
				color.bg = color.fg;

			for (glyph_width_offset = 0; glyph_width_offset < cell_width; glyph_width_offset++) {
				pos = get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset,
					// min: 1280 - 1 - (160 - 1 - 0) * 8 - 7 = 0 / max: 1280 - 1 - (160 - 1 - 159) * 8 - 0 = 1279
						line * cell_height + glyph_height_offset);
					// min: 0 * 16 + 0 = 0 / max: (64 - 1) * 16 + 15 = 1023

				if (cp->wide == WIDE)
					bit_shift = glyph_width_offset + cell_width;
				else
					bit_shift = glyph_width_offset;

				/* set color palette */
				if (gp->bitmap[glyph_height_offset] & (0x01 << bit_shift))
					pixel = fb->color_palette[color.fg];
				else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->bpp);
				else
					pixel = fb->color_palette[color.bg];

				memcpy(fb->buf + pos, &pixel, fb->bpp);
			}
		}

		if (ROTATE == CLOCKWISE || ROTATE == COUNTER_CLOCKWISE) {
			for (glyph_width_offset = 0; glyph_width_offset < cell_width; glyph_width_offset++) {
				pos = (ROTATE == CLOCKWISE) ?
					get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset, line * cell_height):
					// min: 1024 - 1 - (128 - 1 - 0) * 8 - 7 = 0 / max: 1024 - 1 - (128 - 1 - 127) * 8 - 0 = 1023
					// min: 0 * 16 = 0 / max: 79 * 16 = 1264
					get_rotated_pos(fb, term, term->width - 1 - margin_right - glyph_width_offset, (line + 1) * cell_height - 1);
					// min: 1 * 16 - 1 = 15 /  max: 80 * 16 - 1 = 1279
				memcpy(fb->fp + pos, fb->buf + pos, copy_size);
			}
		}
	}

	if (ROTATE == NORMAL || ROTATE == UPSIDE_DOWN) {
		pos = (ROTATE == NORMAL) ?
			get_rotated_pos(fb, term, 0, line * cell_height):
			get_rotated_pos(fb, term, term->width - 1, (line + 1) * cell_height - 1);
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
