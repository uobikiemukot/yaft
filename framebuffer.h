void fb_init(framebuffer *fb)
{
	char *file;
	int ret;
	fix_info finfo;
	var_info vinfo;

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

	fb->fp = emmap(0, fb->sc_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
}

void fb_die(framebuffer *fb)
{
	emunmap(fb->fp, fb->sc_size);
	eclose(fb->fd);
}

void set_bitmap(terminal *term, int y, int x, int offset, u32 *src)
{
	int i;
	color_pair color;
	u32 tmp;
	cell *cp;
	glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	gp = term->fonts[cp->code];
	color = cp->color;

	/* attribute */
	if ((cp->attribute & UNDERLINE) && (offset == (term->cell_size.y - 1)))
		color.bg = color.fg;
	if (cp->attribute & REVERSE) {
		tmp = color.fg;
		color.fg = color.bg;
		color.bg = tmp;
	}

	switch(cp->wide) {
	case HALF:
		for (i = 1; i <= gp->size.x; i++) {
			if ((gp->bitmap[offset] >> (gp->size.x - i)) & 0x01)
				*(src + i - 1) = color.fg;
			else if (term->wall != NULL && color.bg == DEFAULT_BG)
				*(src + i - 1) = *(term->wall + x * term->cell_size.x + (i - 1)
					+ (y * term->cell_size.y + offset) * term->width);
			else
				*(src + i - 1) = color.bg;
		}
		break;
	case WIDE:
		for (i = 1; i <= gp->size.x; i++) {
			if (i <= term->cell_size.x) {
				if ((gp->bitmap[offset * 2] >> (term->cell_size.x - i)) & 0x01)
					*(src + i - 1) = color.fg;
				else if (term->wall != NULL && color.bg == DEFAULT_BG)
					*(src + i - 1) = *(term->wall + x * term->cell_size.x + (i - 1)
						+ (y * term->cell_size.y + offset) * term->width);
				else
					*(src + i - 1) = color.bg;
			}
			else {
				if ((gp->bitmap[offset * 2 + 1] >> (gp->size.x - i)) & 0x01)
					*(src + i - 1) = color.fg;
				else if (term->wall != NULL && color.bg == DEFAULT_BG)
					*(src + i - 1) = *(term->wall + x * term->cell_size.x + (i - 1)
						+ (y * term->cell_size.y + offset) * term->width);
				else
					*(src + i - 1) = color.bg;
			}
		}
		break;
	case NEXT_TO_WIDE:
	default:
		break;
	}
}

void draw_line(framebuffer *fb, terminal *term, int line)
{
	int i, j, size;
	cell *cp;
	u32 *p, *src, *dst, save_bg;

	p = fb->fp + term->offset.x + (term->offset.y + line * term->cell_size.y) * fb->line_length;
	size = term->width * sizeof(u32);
	src = emalloc(size);

	for (i = 0; i < term->cell_size.y; i++) {
		for (j = 0; j < term->cols; j++)
			set_bitmap(term, line, j, i, src + j * term->cell_size.x);
		dst = p + i * fb->line_length;
		memcpy(dst, src, size);
	}
	term->line_dirty[line] = false;

	free(src);
}

void draw_curs(framebuffer *fb, terminal *term)
{
	int i, size;
	color_pair store;
	cell *cp;
	u32 *p, *src, *dst;

	p = fb->fp + term->offset.x + term->cursor.x * term->cell_size.x
		+ (term->offset.y + term->cursor.y * term->cell_size.y) * fb->line_length;
	cp = &term->cells[term->cursor.x + term->cursor.y * term->cols];

	store = cp->color;
	cp->color.fg = DEFAULT_BG;
	cp->color.bg = CURSOR_COLOR;

	if (cp->wide == NEXT_TO_WIDE)
		return;

	size = (cp->wide == WIDE) ?
		size = 2 * term->cell_size.x * sizeof(u32):
		term->cell_size.x * sizeof(u32);
		
	src = emalloc(size);

	for (i = 0; i < term->cell_size.y; i++) {
		set_bitmap(term, term->cursor.y, term->cursor.x, i, src);
		dst = p + i * fb->line_length;
		memcpy(dst, src, size);
	}
	cp->color = store;
	term->line_dirty[term->cursor.y] = true;

	free(src);
}

void refresh(framebuffer *fb, terminal *term)
{
	int i, j;
	cell *cp;

	for (i = 0; i < term->lines; i++) {
		if (term->line_dirty[i])
			draw_line(fb, term, i);
	}

	if (term->mode & CURSOR)
		draw_curs(fb, term);
}
