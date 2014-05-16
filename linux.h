/* See LICENSE for licence details. */
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

/* struct for Linux */
struct framebuffer {
	char *fp;						 /* pointer of framebuffer (read only) */
	char *buf;						 /* copy of framebuffer */
	char *wall;						 /* buffer for wallpaper */
	int fd;							 /* file descriptor of framebuffer */
	int width, height;				 /* display resolution */
	long screen_size;				 /* screen data size (byte) */
	int line_length;				 /* line length (byte) */
	int bpp;						 /* BYTES per pixel */
	struct fb_cmap *cmap, *cmap_org; /* cmap for legacy framebuffer (8bpp pseudocolor) */
};

/* common functions */
char *load_wallpaper(struct framebuffer *fb)
{
	char *ptr;

	ptr = (char *) ecalloc(fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

void get_rgb(int i, struct color_t *color)
{
	extern const uint32_t color_list[]; /* global: see color.h */

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

/* some functions for Linux framebuffer */
struct fb_cmap *cmap_create(struct fb_var_screeninfo *vinfo)
{
	struct fb_cmap *cmap;

	cmap = (struct fb_cmap *) ecalloc(sizeof(struct fb_cmap));
	cmap->start = 0;
	cmap->len = COLORS;
	cmap->red = (uint16_t *) ecalloc(sizeof(uint16_t) * COLORS);
	cmap->green = (uint16_t *) ecalloc(sizeof(uint16_t) * COLORS);
	cmap->blue = (uint16_t *) ecalloc(sizeof(uint16_t) * COLORS);
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

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
{
	int i;
	char *path;
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

	/* check screen offset and initialize because linux console change this */
	if (vinfo.xoffset != 0 || vinfo.yoffset != 0) {
		vinfo.xoffset = vinfo.yoffset = 0;
		ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vinfo);
	}

	fb->width = vinfo.xres;
	fb->height = vinfo.yres;
	fb->screen_size = finfo.smem_len;
	fb->line_length = finfo.line_length;

	if ((finfo.visual == FB_VISUAL_TRUECOLOR || finfo.visual == FB_VISUAL_DIRECTCOLOR)
		&& (vinfo.bits_per_pixel == 15 || vinfo.bits_per_pixel == 16
		|| vinfo.bits_per_pixel == 24 || vinfo.bits_per_pixel == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = (vinfo.bits_per_pixel + BITS_PER_BYTE - 1) / BITS_PER_BYTE; /* ceil() */
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR
		&& vinfo.bits_per_pixel == 8) {
		fb->cmap = cmap_create(&vinfo);
		fb->cmap_org = cmap_create(&vinfo);
		cmap_init(fb, &vinfo);
		fb->bpp = 1;
	}
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = get_color(&vinfo, i);

	fb->fp = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (char *) ecalloc(fb->screen_size);
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

void draw_line(struct framebuffer *fb, struct terminal *term, int line)
{
	int pos, size, bit_shift, margin_right;
	int col, glyph_width_offset, glyph_height_offset;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

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
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((glyph_height_offset == (CELL_HEIGHT - 1)) && (cp->attribute & attr_mask[UNDERLINE]))
				color.bg = color.fg;

			for (glyph_width_offset = 0; glyph_width_offset < CELL_WIDTH; glyph_width_offset++) {
				pos = (term->width - 1 - margin_right - glyph_width_offset) * fb->bpp
					+ (line * CELL_HEIGHT + glyph_height_offset) * fb->line_length;

				/* check wide character or not */
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

				/* update copy buffer only */
				memcpy(fb->buf + pos, &pixel, fb->bpp);
			}
		}
	}

	/* actual display update (bit blit) */
	pos = (line * CELL_HEIGHT) * fb->line_length;
	size = CELL_HEIGHT * fb->line_length;
	memcpy(fb->fp + pos, fb->buf + pos, size);

	/* TODO: page flip
		if fb_fix_screeninfo.ypanstep > 0, we can use hardware panning.
		set fb_fix_screeninfo.{yres_virtual,yoffset} and call ioctl(FBIOPAN_DISPLAY)
		but drivers  of recent hardware (inteldrmfb, nouveaufb, radeonfb) don't support...
		(we can use this by using libdrm)
	*/

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
