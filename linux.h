/* See LICENSE for licence details. */
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

/* framubuffer device */
const char *fb_path = "/dev/fb0";

/* shell */
const char *shell_cmd = "/bin/bash";

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

void cmap_update(int fd, struct fb_cmap *cmap)
{
	if (cmap) {
		if (ioctl(fd, FBIOPUTCMAP, cmap))
			fatal("ioctl: FBIOPUTCMAP failed");
	}
}

void cmap_init(struct framebuffer *fb, struct fb_var_screeninfo *vinfo)
{
	extern const uint32_t color_list[]; /* global */
	int i;
	uint16_t r, g, b;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org)) { /* not fatal */
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

	cmap_update(fb->fd, fb->cmap);
}

static inline uint32_t color2pixel(struct fb_var_screeninfo *vinfo, uint32_t color)
{
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

	/* pseudo color */
	if (vinfo->bits_per_pixel == 8) {
		if (r == g && r == b) { /* 24 gray scale */
			r = 24 * r / COLORS;
			return 232 + r;
		}                       /* 6x6x6 color cube */
		r = 6 * r / COLORS;
		g = 6 * g / COLORS;
		b = 6 * b / COLORS;
		return 16 + (r * 36) + (g * 6) + b;
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
	char *path;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo))
		fatal("ioctl: FBIOGET_FSCREENINFO failed");

	if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo))
		fatal("ioctl: FBIOGET_VSCREENINFO failed");

	/* check screen offset and initialize because linux console change this */
	if (vinfo.xoffset != 0 || vinfo.yoffset != 0) {
		vinfo.xoffset = vinfo.yoffset = 0;
		ioctl(fb->fd, FBIOPUT_VSCREENINFO, &vinfo);
	}

	fb->width  = vinfo.xres;
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
		color_palette[i] = (fb->bpp == 1) ? (uint32_t) i: color2pixel(&vinfo, color_list[i]);

	fb->fp    = (unsigned char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf   = (unsigned char *) ecalloc(1, fb->screen_size);
	fb->wall  = (WALLPAPER && fb->bpp > 1) ? load_wallpaper(fb): NULL;
	fb->vinfo = vinfo;
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		ioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org); /* not fatal */
		cmap_die(fb->cmap_org);
	}
	free(fb->buf);
	free(fb->wall);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}

static inline void draw_line(struct framebuffer *fb, struct terminal *term, int line)
{
	int pos, size, bdf_padding, glyph_width, margin_right;
	int col, w, h, src_offset, dst_offset;
	uint32_t pixel, color = 0;
	struct color_pair_t color_pair;
	struct cell_t *cellp;
	const struct glyph_t *glyphp;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* target cell */
		cellp = &term->cells[col + line * term->cols];

		/* draw sixel bitmap */
		if (cellp->has_bitmap) {
			for (h = 0; h < CELL_HEIGHT; h++) {
				for (w = 0; w < CELL_WIDTH; w++) {
					src_offset = BYTES_PER_PIXEL * (h * CELL_WIDTH + w);
					memcpy(&color, cellp->bitmap + src_offset, BYTES_PER_PIXEL);

					dst_offset = (line * CELL_HEIGHT + h) * fb->line_length + (col * CELL_WIDTH + w) * fb->bpp;
					pixel = color2pixel(&fb->vinfo, color);
					memcpy(fb->buf + dst_offset, &pixel, fb->bpp);
				}
			}
			continue;
		}

		/* get color and glyph */
		color_pair = cellp->color_pair;
		glyphp     = cellp->glyphp;

		/* check wide character or not */
		glyph_width = (cellp->width == HALF) ? CELL_WIDTH: CELL_WIDTH * 2;
		bdf_padding = my_ceil(glyph_width, BITS_PER_BYTE) * BITS_PER_BYTE - glyph_width;
		if (cellp->width == WIDE)
			bdf_padding += CELL_WIDTH;

		/* check cursor positon */
		if ((term->mode & MODE_CURSOR && line == term->cursor.y)
			&& (col == term->cursor.x
			|| (cellp->width == WIDE && (col + 1) == term->cursor.x)
			|| (cellp->width == NEXT_TO_WIDE && (col - 1) == term->cursor.x))) {
			color_pair.fg = DEFAULT_BG;
			color_pair.bg = (!tty.visible && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		for (h = 0; h < CELL_HEIGHT; h++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((h == (CELL_HEIGHT - 1)) && (cellp->attribute & attr_mask[ATTR_UNDERLINE]))
				color_pair.bg = color_pair.fg;

			for (w = 0; w < CELL_WIDTH; w++) {
				pos = (term->width - 1 - margin_right - w) * fb->bpp
					+ (line * CELL_HEIGHT + h) * fb->line_length;

				/* set color palette */
				if (glyphp->bitmap[h] & (0x01 << (bdf_padding + w)))
					pixel = term->color_palette[color_pair.fg];
				else if (fb->wall && color_pair.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->bpp);
				else
					pixel = term->color_palette[color_pair.bg];

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
