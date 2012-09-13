/* See LICENSE for licence details. */
bool wallpaper = false;

u32 *load_wallpaper(framebuffer *fb, int width, int height)
{
	int i, j, count = 0;
	u32 *ptr;

	ptr = (u32 *) emalloc(width * height * sizeof(u32));

	for (i = 0; i < fb->res.y; i++) {
		for (j = 0; j < fb->res.x; j++) {
			if (i < height && j < width)
				*(ptr + count++) = *(fb->fp + j + i * fb->line_length);
		}
	}

	return ptr;
}

void fb_init(framebuffer *fb)
{
	char *file;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	if ((file = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(file, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	eioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo);
	eioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo);

	fb->res.x = vinfo.xres;
	fb->res.y = vinfo.yres;
	fb->sc_size = finfo.smem_len;
	fb->line_length = finfo.line_length / sizeof(u32);

	fb->fp = emmap(0, fb->sc_size,
		PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);

	if (getenv("YAFT_WALLPAPER") != NULL)
		wallpaper = true;

	fb->wall = wallpaper ?
		load_wallpaper(fb, fb->res.x, fb->res.y): NULL;
}

void fb_die(framebuffer *fb)
{
	free(fb->wall);
	emunmap(fb->fp, fb->sc_size);
	eclose(fb->fd);
}

void set_bitmap(framebuffer *fb, terminal *term, int y, int x, int offset, u32 *src)
{
	int i, shift;
	color_pair color;
	cell *cp;
	glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = term->fonts[cp->code];
	shift = ceil((double) gp->size.x / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	/* attribute */
	if ((cp->attribute & attr_mask[UNDERLINE])
		&& (offset == (term->cell_size.y - 1)))
		color.bg = color.fg;

	for (i = 0; i < gp->size.x; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1)))
			*(src + i) = color_palette[color.fg];
		else if (fb->wall && color.bg == DEFAULT_BG)
			*(src + i) = *(fb->wall + i + x * term->cell_size.x
				+ (offset + y * term->cell_size.y) * term->width);
		else
			*(src + i) = color_palette[color.bg];
	}
}

void draw_line(framebuffer *fb, terminal *term, int line)
{
	int i, j, size;
	u32 *p, *src, *dst;

	p = fb->fp + term->offset.x 
		+ (term->offset.y + line * term->cell_size.y) * fb->line_length;
	size = term->width * sizeof(u32);
	src = emalloc(size);

	for (i = 0; i < term->cell_size.y; i++) {
		for (j = 0; j < term->cols; j++)
			set_bitmap(fb, term, line, j, i, src + j * term->cell_size.x);
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
	u32 *p, *src, *dst;

	store_curs = term->cursor;
	if (term->cells[term->cursor.x + term->cursor.y * term->cols].wide == NEXT_TO_WIDE)
		term->cursor.x--;

	p = fb->fp + term->offset.x + term->cursor.x * term->cell_size.x
		+ (term->offset.y + term->cursor.y * term->cell_size.y) * fb->line_length;
	cp = &term->cells[term->cursor.x + term->cursor.y * term->cols];

	store_color = cp->color;
	cp->color.fg = DEFAULT_BG;
	cp->color.bg = CURSOR_COLOR;

	store_attr = cp->attribute;
	cp->attribute = RESET;

	size = (cp->wide == WIDE) ?
		2 * term->cell_size.x * sizeof(u32):
		term->cell_size.x * sizeof(u32);
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
