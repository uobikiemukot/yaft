/* See LICENSE for licence details. */
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

/* struct for Linux */
struct framebuffer {
	unsigned char *fp;               /* pointer of framebuffer (read only) */
	unsigned char *buf;              /* copy of framebuffer */
	unsigned char *wall;             /* buffer for wallpaper */
	int fd;                          /* file descriptor of framebuffer */
	int width, height;               /* display resolution */
	long screen_size;                /* screen data size (byte) */
	int line_length;                 /* line length (byte) */
	int bpp;                         /* BYTES per pixel */
	struct fb_cmap *cmap, *cmap_org; /* cmap for legacy framebuffer (8bpp pseudocolor) */
	struct fb_var_screeninfo vinfo;
};

/* common functions */
unsigned char *load_wallpaper(struct framebuffer *fb)
{
	unsigned char *ptr;

	ptr = (unsigned char *) ecalloc(1, fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

/* some functions for Linux framebuffer */
void cmap_create(struct fb_cmap **cmap)
{
	*cmap           = (struct fb_cmap *) ecalloc(1, sizeof(struct fb_cmap));
	(*cmap)->start  = 0;
	(*cmap)->len    = COLORS;
	(*cmap)->red    = (uint16_t *) ecalloc(COLORS, sizeof(uint16_t));
	(*cmap)->green  = (uint16_t *) ecalloc(COLORS, sizeof(uint16_t));
	(*cmap)->blue   = (uint16_t *) ecalloc(COLORS, sizeof(uint16_t));
	(*cmap)->transp = NULL;
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
	extern const uint32_t color_list[]; /* global */
	int i;
	uint16_t r, g, b;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		r = bit_mask[8] & (color_list[i] >> 16);
		g = bit_mask[8] & (color_list[i] >> 8);
		b = bit_mask[8] & (color_list[i] >> 0);

		r = (r << BITS_PER_BYTE) | r;
		g = (g << BITS_PER_BYTE) | g;
		b = (b << BITS_PER_BYTE) | b;

		*(fb->cmap->red + i) = (vinfo->red.msb_right) ?
			bit_mask[16] & bit_reverse(r, 16): r;
		*(fb->cmap->green + i) = (vinfo->green.msb_right) ?
			bit_mask[16] & bit_reverse(g, 16): g;
		*(fb->cmap->blue + i) = (vinfo->blue.msb_right) ?
			bit_mask[16] & bit_reverse(b, 16): b;
	}

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0)
		fatal("ioctl: FBIOPUTCMAP failed");
}

inline uint32_t color2pixel(struct fb_var_screeninfo *vinfo, uint32_t color, bool similar)
{
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

 	/* pseudo color */
	if (vinfo->bits_per_pixel == 8) {
		if (similar) {                /* search similar color */
			if (r == g && r == b) {   /* gray scale */
				r = 24 * r / 256;
				return 232 + r;
			}                         /* 6x6x6 color cube */
			r = 6 * r / 256;
			g = 6 * g / 256;
			b = 6 * b / 256;
			return 16 + (r * 36) + (g * 6) + b;
		}
		return color; /* just return color palette index */
	}

	/* direct color */
	r = r >> (BITS_PER_BYTE - vinfo->red.length);
	g = g >> (BITS_PER_BYTE - vinfo->green.length);
	b = b >> (BITS_PER_BYTE - vinfo->blue.length);

	/* check bit reverse flag */
	if (vinfo->red.msb_right)
		r = bit_mask[vinfo->red.length]   & bit_reverse(r, vinfo->red.length);
	if (vinfo->green.msb_right)
		g = bit_mask[vinfo->green.length] & bit_reverse(g, vinfo->green.length);
	if (vinfo->blue.msb_right)
		b = bit_mask[vinfo->blue.length]  & bit_reverse(b, vinfo->blue.length);

	return (r << vinfo->red.offset)
		+ (g << vinfo->green.offset)
		+ (b << vinfo->blue.offset);
}

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
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
		fb->bpp = my_ceil(vinfo.bits_per_pixel, BITS_PER_BYTE);
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR && vinfo.bits_per_pixel == 8) {
		cmap_create(&fb->cmap);
		cmap_create(&fb->cmap_org);
		cmap_init(fb, &vinfo);
		fb->bpp = 1;
	}
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = color2pixel(&vinfo, i, false);
		//color_palette[i] = color_list[i];

	fb->fp = (unsigned char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (unsigned char *) ecalloc(1, fb->screen_size);
	fb->vinfo = vinfo;

	fb->wall = NULL;
	if (((env = getenv("YAFT")) != NULL)
		&& (strstr(env, "wallpaper") != NULL || strstr(env, "wall") != NULL))
			fb->wall = load_wallpaper(fb);
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

inline void draw_line(struct framebuffer *fb, struct terminal *term, int line)
{
	int pos, size, bit_shift, margin_right;
	int col, glyph_width_offset, glyph_height_offset;
	uint32_t pixel;
	struct color_pair color;
	struct cell *cp;
	const struct glyph_t *gp;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* get cell color and glyph */
		cp = &term->cells[col + line * term->cols];
		color = cp->color;
		gp = cp->gp;

		/* check wide character or not */
		bit_shift = (cp->width == WIDE) ? CELL_WIDTH: 0;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color.fg = DEFAULT_BG;
			color.bg = (!tty.visible && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		for (glyph_height_offset = 0; glyph_height_offset < CELL_HEIGHT; glyph_height_offset++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((glyph_height_offset == (CELL_HEIGHT - 1)) && (cp->attribute & attr_mask[UNDERLINE]))
				color.bg = color.fg;

			for (glyph_width_offset = 0; glyph_width_offset < CELL_WIDTH; glyph_width_offset++) {
				pos = (term->width - 1 - margin_right - glyph_width_offset) * fb->bpp
					+ (line * CELL_HEIGHT + glyph_height_offset) * fb->line_length;

				/* set color palette */
				if (gp->bitmap[glyph_height_offset] & (0x01 << (bit_shift + glyph_width_offset)))
					pixel = term->color_palette[color.fg];
					//pixel = color2pixel(&fb->vinfo, term->color_palette[color.fg], true);
				else if (fb->wall && color.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->bpp);
				else
					pixel = term->color_palette[color.bg];
					//pixel = color2pixel(&fb->vinfo, term->color_palette[color.bg], true);

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

inline void draw_sixel(struct framebuffer *fb, struct sixel_canvas_t *sc)
{
	int h, w, fb_pos, sc_pos;
	uint32_t color, similar;
	struct point_t offset;

	offset.x = sc->ref_cell.x * CELL_WIDTH;
	offset.y = sc->ref_cell.y * CELL_HEIGHT;

	if (DEBUG)
		fprintf(stderr, "offset(%d, %d) width:%d height:%d\n",
			offset.x, offset.y, sc->width, sc->height);

	for (h = 0; h < sc->height; h++) {
		if ((offset.y + h) >= fb->height)
			break;
		for (w = 0; w < sc->width; w++) {
			if ((offset.x + w) >= fb->width)
				break;

			sc_pos  = w * sizeof(uint32_t) + h * sc->line_length;
			fb_pos  = (offset.x + w) * fb->bpp + (offset.y + h) * fb->line_length;

			memcpy(&color, sc->data + sc_pos, sizeof(uint32_t));
			similar = color2pixel(&fb->vinfo, color, true);

			memcpy(fb->buf + fb_pos, &similar, fb->bpp);
		}
		fb_pos = offset.x * fb->bpp + (offset.y + h) * fb->line_length;
		memcpy(fb->fp + fb_pos, fb->buf + fb_pos, sc->width * fb->bpp);
	}
}

void refresh(struct framebuffer *fb, struct terminal *term)
{
	int i, line;
	struct sixel_canvas_t *sc;

	if (term->esc.state == STATE_RESET) {
	for (i = 0; i < MAX_SIXEL_CANVAS; i++) {
		sc = &term->sixel_canvas[i];
		if (sc->data != NULL)
			draw_sixel(fb, sc);
	}
	}

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (line = 0; line < term->lines; line++) {
		if (term->line_dirty[line])
			draw_line(fb, term, line);
	}
}
