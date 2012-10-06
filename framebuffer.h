/* See LICENSE for licence details. */
u8 *load_wallpaper(framebuffer *fb, int width, int height)
{
	int i, j, count = 0, src, dst;
	u8 *ptr;

	ptr = (u8 *) emalloc(width * height * fb->bytes_per_pixel);

	for (i = 0; i < fb->res.y; i++) {
		for (j = 0; j < fb->res.x; j++) {
			if (i < height && j < width) {
				dst = count++ * fb->bytes_per_pixel;
				src = j * fb->bytes_per_pixel + i * fb->line_length;
				memcpy(ptr + dst, fb->fp + src, fb->bytes_per_pixel);
			}
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
	fb->sc_size = finfo.smem_len;
	fb->line_length = finfo.line_length;

	if ((finfo.visual == FB_VISUAL_TRUECOLOR
		|| finfo.visual == FB_VISUAL_DIRECTCOLOR)
		&& (vinfo.bits_per_pixel == 15 || vinfo.bits_per_pixel == 16
		|| vinfo.bits_per_pixel == 24 || vinfo.bits_per_pixel == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bytes_per_pixel = bit2byte(vinfo.bits_per_pixel);
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR
		&& vinfo.bits_per_pixel == 8) {
		fb->cmap = cmap_create(&vinfo);
		fb->cmap_org = cmap_create(&vinfo);
		cmap_init(fb, &vinfo);
		fb->bytes_per_pixel = 1;
	}
	else {
		/* non packed pixel, mono color, grayscale: not implimented */
		fprintf(stderr, "unsupported framebuffer type:%d\n", finfo.type);
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = get_color(&vinfo, i);

	fb->fp = (u8 *) emmap(0, fb->sc_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);

	if (getenv("YAFT_WALLPAPER") != NULL && fb->bytes_per_pixel != 1)
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

	emunmap(fb->fp, fb->sc_size);
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

	/* attribute */
	if ((offset == (term->cell_size.y - 1))
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;

	for (i = 0; i < gp->size.x; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1)))
			pixel = color_palette[color.fg];
		else if (fb->wall && color.bg == DEFAULT_BG)
			memcpy(&pixel, fb->wall + (i + x * term->cell_size.x
				+ (offset + y * term->cell_size.y)
				* term->width) * fb->bytes_per_pixel, fb->bytes_per_pixel);
		else
			pixel = color_palette[color.bg];
		memcpy(src + i * fb->bytes_per_pixel, &pixel, fb->bytes_per_pixel);
	}
}

void draw_line(framebuffer *fb, terminal *term, int line)
{
	int i, j, size;
	u8 *p, *src, *dst;

	p = fb->fp + term->offset.x * fb->bytes_per_pixel
		+ (term->offset.y + line * term->cell_size.y) * fb->line_length;
	size = term->width * fb->bytes_per_pixel;
	src = emalloc(size);

	for (i = 0; i < term->cell_size.y; i++) {
		for (j = 0; j < term->cols; j++)
			set_bitmap(fb, term, line, j, i,
				src + j * term->cell_size.x * fb->bytes_per_pixel);
		dst = p + i * fb->line_length;
		memcpy(dst, src, size);
	}
	term->line_dirty[line] = false;

	free(src);
}

void draw_curs(framebuffer *fb, terminal *term)
{
	int i, size;
	color_pair store_color;
	pair store_curs;
	u8 store_attr;
	cell *cp;
	u8 *p, *src, *dst;

	store_curs = term->cursor;
	if (term->cells[term->cursor.x + term->cursor.y * term->cols].wide == NEXT_TO_WIDE)
		term->cursor.x--;

	p = fb->fp + (term->offset.x + term->cursor.x * term->cell_size.x) * fb->bytes_per_pixel
		+ (term->offset.y + term->cursor.y * term->cell_size.y) * fb->line_length;
	cp = &term->cells[term->cursor.x + term->cursor.y * term->cols];

	store_color = cp->color;
	cp->color.fg = DEFAULT_BG;
	cp->color.bg = CURSOR_COLOR;

	store_attr = cp->attribute;
	cp->attribute = RESET;

	size = (cp->wide == WIDE) ?
		2 * term->cell_size.x * fb->bytes_per_pixel:
		term->cell_size.x * fb->bytes_per_pixel;
	src = emalloc(size);

	for (i = 0; i < term->cell_size.y; i++) {
		set_bitmap(fb, term, term->cursor.y, term->cursor.x, i, src);
		dst = p + i * fb->line_length;
		memcpy(dst, src, size);
	}
	free(src);

	cp->color = store_color;
	cp->attribute = store_attr;
	term->cursor = store_curs;
	term->line_dirty[term->cursor.y] = true;
}

void refresh(framebuffer *fb, terminal *term)
{
	int i;

	for (i = 0; i < term->lines; i++) {
		if (term->line_dirty[i])
			draw_line(fb, term, i);
	}

	if (term->mode & CURSOR)
		draw_curs(fb, term);
}
