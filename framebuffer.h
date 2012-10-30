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

void cmap_init(struct framebuffer *fb, struct fb_var_screeninfo *vinfo)
{
	int i;
	uint16_t r, g, b;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) {
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		r = (color_list[i].r << BITS_PER_BYTE) | color_list[i].r;
		g = (color_list[i].g << BITS_PER_BYTE) | color_list[i].g;
		b = (color_list[i].b << BITS_PER_BYTE) | color_list[i].b;

		*(fb->cmap->red + i) = (vinfo->red.msb_right) ?
			bit_reverse(r, 16) & bit_mask[16]: r;
		*(fb->cmap->green + i) = (vinfo->green.msb_right) ?
			bit_reverse(g, 16) & bit_mask[16]: g;
		*(fb->cmap->blue + i) = (vinfo->blue.msb_right) ?
			bit_reverse(b, 16) & bit_mask[16]: b;
	}
	eioctl(fb->fd, FBIOPUTCMAP, fb->cmap);
}

uint32_t get_color(struct fb_var_screeninfo *vinfo, int i)
{
	uint32_t r, g, b;

	if (vinfo->bits_per_pixel == 8)
		return i;

	r = color_list[i].r >> (BITS_PER_BYTE - vinfo->red.length);
	g = color_list[i].g >> (BITS_PER_BYTE - vinfo->green.length);
	b = color_list[i].b >> (BITS_PER_BYTE - vinfo->blue.length);

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

	eioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo);
	eioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo);

	fb->res.x = vinfo.xres;
	fb->res.y = vinfo.yres;
	fb->screen_size = finfo.smem_len;
	fb->line_len = finfo.line_length;

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
	else {
		/* non packed pixel, mono color, grayscale: not implimented */
		fprintf(stderr, "unsupported framebuffer type:%d\n", finfo.type);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < COLORS; i++) /* init color palette */
		fb->color_palette[i] = get_color(&vinfo, i);

	fb->fp = (uint8_t *) emmap(0, fb->screen_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);

	fb->buf = (uint8_t *) emalloc(fb->screen_size);

	fb->wall = ((env = getenv("YAFT")) != NULL && strncmp(env, "wall", 4) == 0 && fb->bpp > 1) ?
		load_wallpaper(fb): NULL;
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		eioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org);
		cmap_die(fb->cmap_org);
	}
	free(fb->wall);
	free(fb->buf);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}

void set_bitmap(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, uint8_t *src)
{
	int i, shift;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	struct glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = term->fonts[cp->code].gp;
	shift = ((gp->width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	if ((term->mode & MODE_CURSOR && y == term->cursor.y) /* cursor */
		&& (x == term->cursor.x || (cp->wide == WIDE && (x + 1) == term->cursor.x))) {
		color.fg = DEFAULT_BG;
		color.bg = CURSOR_COLOR;
	}

	if ((offset == (term->cell_height - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;
	
	for (i = 0; i < gp->width; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1)))
			pixel = fb->color_palette[color.fg];
		else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
			memcpy(&pixel, fb->wall + (i + x * term->cell_width + term->offset.x) * fb->bpp
				+ (offset + y * term->cell_height + term->offset.y) * fb->line_len, fb->bpp);
		else
			pixel = fb->color_palette[color.bg];
		memcpy(src + i * fb->bpp, &pixel, fb->bpp);
	}
}

void draw_line(struct framebuffer *fb, struct terminal *term, int y)
{
	int offset, x, size, pos;
	uint8_t *src, *dst;

	pos = term->offset.x * fb->bpp + (term->offset.y + y * term->cell_height) * fb->line_len;
	size = term->width * fb->bpp;

	for (offset = 0; offset < term->cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(fb, term, y, x, offset,
				fb->buf + pos + x * term->cell_width * fb->bpp + offset * fb->line_len);
		src = fb->buf + pos + offset * fb->line_len;
		dst = fb->fp + pos + offset * fb->line_len;
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
