/* See LICENSE for licence details. */
uint8_t *load_wallpaper(struct framebuffer *fb)
{
	uint8_t *ptr;

	ptr = (uint8_t *) emalloc(fb->screen_size);
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

	fb->fp = (uint8_t *) emmap(0, fb->screen_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (uint8_t *) emalloc(fb->screen_size);

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

void set_bitmap(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, uint8_t *src)
{
	int i, shift, glyph_width;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = &fonts[cp->code];
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
			memcpy(&pixel, fb->wall + (i + x * cell_width + term->offset.x) * fb->bpp
				+ (offset + y * cell_height + term->offset.y) * fb->line_length, fb->bpp);
		else
			pixel = fb->color_palette[color.bg];
		memcpy(src + i * fb->bpp, &pixel, fb->bpp);
	}
}

void draw_line(struct framebuffer *fb, struct terminal *term, int y)
{
	int offset, x, size, pos;
	uint8_t *src, *dst;

	pos = term->offset.x * fb->bpp + (term->offset.y + y * cell_height) * fb->line_length;
	size = term->width * fb->bpp;

	for (offset = 0; offset < cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(fb, term, y, x, offset,
				fb->buf + pos + x * cell_width * fb->bpp + offset * fb->line_length);
		src = fb->buf + pos + offset * fb->line_length;
		dst = fb->fp + pos + offset * fb->line_length;
		memcpy(dst, src, size);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;
}

void refresh(struct framebuffer *fb, struct terminal *term)
{
	int y;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (y = 0; y < term->lines; y++) {
		if (term->line_dirty[y])
			draw_line(fb, term, y);
	}
}
