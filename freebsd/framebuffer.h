/* See LICENSE for licence details. */
char *load_wallpaper(struct framebuffer *fb)
{
	char *ptr;

	ptr = (char *) emalloc(fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

video_color_palette_t *cmap_create(video_info_t *video_info)
{
	video_color_palette_t *cmap;

	cmap = (video_color_palette_t *) emalloc(sizeof(video_color_palette_t));
	cmap->index = 0;
	cmap->count = COLORS;
	cmap->red = (u_char *) emalloc(sizeof(u_char) * COLORS);
	cmap->green = (u_char *) emalloc(sizeof(u_char) * COLORS);
	cmap->blue = (u_char *) emalloc(sizeof(u_char) * COLORS);
	cmap->transparent = NULL;

	return cmap;
}

void cmap_die(video_color_palette_t *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		free(cmap->transparent);
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

void cmap_init(struct framebuffer *fb, video_info_t *video_info)
{
	int i;
	struct color_t color;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		/* where is endian info? */
		get_rgb(i, &color);
		*(fb->cmap->red + i) = color.r;
		*(fb->cmap->green + i) = color.g;
		*(fb->cmap->blue + i) = color.b;
	}

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0)
		fatal("ioctl: FBIOPUTCMAP failed");
}

uint32_t get_color(video_info_t *video_info, int i)
{
	uint32_t r, g, b;
	struct color_t color;

	if (video_info->vi_depth == 8)
		return i;

	/* where is endian info? */
	get_rgb(i, &color);
	r = color.r >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[0]);
	g = color.g >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[1]);
	b = color.b >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[2]);

	return (r << video_info->vi_pixel_fields[0])
		+ (g << video_info->vi_pixel_fields[1])
		+ (b << video_info->vi_pixel_fields[2]);
}

void fb_init(struct framebuffer *fb)
{
	int i, video_mode;
	char *path, *env;
	video_info_t video_info;
	video_adapter_info_t video_adapter_info;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIO_GETMODE, &video_mode) < 0)
		fatal("ioctl: FBIO_GETMODE failed");

	if (video_mode != VIDEO_MODE) {
		fprintf(stderr, "current mode:%d request mode:%d\n", video_mode, VIDEO_MODE);
		fatal("video mode unmatch");
	}

	video_info.vi_mode = video_mode;
	if (ioctl(fb->fd, FBIO_MODEINFO, &video_info) < 0)
		fatal("ioctl: FBIO_MODEINFO failed");

	if (ioctl(fb->fd, FBIO_ADPINFO, &video_adapter_info) < 0)
		fatal("ioctl: FBIO_ADPINFO failed");

	fb->res.x = video_info.vi_width;
	fb->res.y = video_info.vi_height;

	fb->screen_size = video_adapter_info.va_window_size;
	fb->line_length = video_adapter_info.va_line_width;

	if ((video_info.vi_mem_model == V_INFO_MM_PACKED || video_info.vi_mem_model == V_INFO_MM_DIRECT)
		&& (video_info.vi_depth == 15 || video_info.vi_depth == 16
		|| video_info.vi_depth == 24 || video_info.vi_depth == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = (video_info.vi_depth + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
	}
	else if ((video_adapter_info.va_flags & V_ADP_PALETTE) &&
		video_info.vi_mem_model == V_INFO_MM_PACKED && video_info.vi_depth == 8) {
		fb->cmap = cmap_create(&video_info);
		fb->cmap_org = cmap_create(&video_info);
		cmap_init(fb, &video_info);
		fb->bpp = 1;
	}
	else
		/* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		fb->color_palette[i] = get_color(&video_info, i);

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

void set_bitmap(struct framebuffer *fb, struct terminal *term, int y, int x, int offset, char *src)
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
	char *src, *dst;

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
