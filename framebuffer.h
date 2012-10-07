/* See LICENSE for licence details. */
u8 *load_wallpaper(framebuffer *fb, int width, int height)
{
	int i, j, count = 0;
	u8 *ptr;

	ptr = (u8 *) emalloc(width * height * fb->bpp);

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			memcpy(ptr + fb->bpp * count++,
				fb->fp + j * fb->bpp + i * fb->line_len, fb->bpp);
		}
	}

	return ptr;
}

cmap_t *cmap_create(vinfo_t *vinfo)
{
	cmap_t *cmap;

	cmap = (cmap_t *) emalloc(sizeof(cmap_t));
	memset(cmap, 0, sizeof(cmap_t));

	cmap->red = (u16 *) emalloc(sizeof(u16) * COLORS);
	cmap->green = (u16 *) emalloc(sizeof(u16) * COLORS);
	cmap->blue = (u16 *) emalloc(sizeof(u16) * COLORS);
	cmap->transp = NULL;
	cmap->len = COLORS;

	return cmap;
}

void cmap_init(framebuffer *fb, vinfo_t *vinfo)
{
	int i;
	u16 r, g, b;

	eioctl(fb->fd, FBIOGETCMAP, fb->cmap_org);

	for (i = 0; i < COLORS; i++) {
 		/* color_list defined in color.h */
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

u32 get_color(vinfo_t *vinfo, int i)
{
	u32 r, g, b;

	if (vinfo->bits_per_pixel == 8)
		return i;

 	/* color_list defined in color.h */
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

void fb_init(framebuffer *fb)
{
	int i;
	char *file;
	finfo_t finfo;
	vinfo_t vinfo;

	if ((file = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(file, O_RDWR);
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
		fb->bpp = bit2byte(vinfo.bits_per_pixel);
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

	fb->fp = (u8 *) emmap(0, fb->screen_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);

	if (getenv("YAFT_WALLPAPER") != NULL && fb->bpp != 1)
		fb->wall = load_wallpaper(fb, fb->res.x, fb->res.y);
	else
		fb->wall = NULL;
}

void cmap_die(cmap_t *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		free(cmap->transp);
		free(cmap);
	}
}

void fb_die(framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		eioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org);
		cmap_die(fb->cmap_org);
	}

	emunmap(fb->fp, fb->screen_size);
	free(fb->wall);
	eclose(fb->fd);
}

void set_bitmap(framebuffer *fb, terminal *term, int y, int x, int offset, u8 *src)
{
	int i, shift;
	u32 pixel;
	color_pair color;
	cell *cp;
	glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = term->fonts[cp->code];
	shift = bit2byte(gp->size.x) * BITS_PER_BYTE;
	color = cp->color;

	if ((term->mode & MODE_CURSOR && y == term->cursor.y) /* cursor */
		&& (x == term->cursor.x || (cp->wide == WIDE && (x + 1) == term->cursor.x))) {
		color.fg = DEFAULT_BG;
		color.bg = CURSOR_COLOR;
	}

	if ((offset == (term->cell_size.y - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;
	
	for (i = 0; i < gp->size.x; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1)))
			pixel = fb->color_palette[color.fg];
		else if (fb->wall && color.bg == DEFAULT_BG)
			memcpy(&pixel, fb->wall + (i + x * term->cell_size.x
				+ (offset + y * term->cell_size.y)
				* term->width) * fb->bpp, fb->bpp);
		else
			pixel = fb->color_palette[color.bg];
		memcpy(src + i * fb->bpp, &pixel, fb->bpp);
	}
}

void draw_line(framebuffer *fb, terminal *term, int y)
{
	int offset, x, size;
	u8 *ptr, *src, *dst;

	ptr = fb->fp + term->offset.x * fb->bpp
		+ (term->offset.y + y * term->cell_size.y) * fb->line_len;
	size = term->width * fb->bpp;
	src = emalloc(size);

	for (offset = 0; offset < term->cell_size.y; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(fb, term, y, x, offset, src + x * term->cell_size.x * fb->bpp);
		dst = ptr + offset * fb->line_len;
		memcpy(dst, src, size);
	}
	term->line_dirty[y] =
		(term->mode & MODE_CURSOR && y == term->cursor.y) ? true: false;

	free(src);
}

void refresh(framebuffer *fb, terminal *term)
{
	int y;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (y = 0; y < term->lines; y++) {
		if (term->line_dirty[y])
			draw_line(fb, term, y);
	}
}
