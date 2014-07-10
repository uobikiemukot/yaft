/* See LICENSE for licence details. */

/* _XOPEN_SOURCE >= 600 invalidates __BSD_VISIBLE
        so define some types manually */
typedef unsigned char   unchar;
typedef unsigned char   u_char;
typedef unsigned short  ushort;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#include  <sys/param.h>
#include  <dev/wscons/wsdisplay_usl_io.h>
#include  <dev/wscons/wsconsio.h>
#include  <dev/wscons/wsksymdef.h>

/* framubuffer device */
const char *fb_path = "/dev/ttyE0"; /* for NetBSD */
//const char *fb_path = "/dev/ttyC0"; /* for OpenBSD */

/* shell */
const char *shell_cmd = "/bin/csh"; /* for NetBSD */

/* some structs for NetBSD */
struct framebuffer {
	unsigned char *fp;    /* pointer of framebuffer(read only) */
	unsigned char *wall;  /* buffer for wallpaper */
	unsigned char *buf;   /* copy of framebuffer */
	int fd;               /* file descriptor of framebuffer */
	int width, height;    /* display resolution */
	long screen_size;     /* screen data size (byte) */
	int line_length;      /* line length (byte) */
	int bpp;              /* BYTES per pixel */
	struct wsdisplay_cmap /* cmap for legacy framebuffer (8bpp pseudocolor) */
		*cmap, *cmap_org;
	struct wsdisplayio_fbinfo vinfo;
};

/* common functions */
unsigned char *load_wallpaper(struct framebuffer *fb)
{
	unsigned char *ptr;

	ptr = (unsigned char *) ecalloc(1, fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

/* some functions for NetBSD framebuffer */
void cmap_create(struct wsdisplay_cmap **cmap)
{
	*cmap                = (struct wsdisplay_cmap *) ecalloc(1, sizeof(struct wsdisplay_cmap));
	(*cmap)->index       = 0;
	(*cmap)->count       = COLORS;
	(*cmap)->red         = (u_char *) ecalloc(COLORS, sizeof(u_char));
	(*cmap)->green       = (u_char *) ecalloc(COLORS, sizeof(u_char));
	(*cmap)->blue        = (u_char *) ecalloc(COLORS, sizeof(u_char));
	//(*cmap)->transparent = NULL;
}

void cmap_die(struct wsdisplay_cmap *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		//free(cmap->transparent);
		free(cmap);
	}
}

void cmap_update(int fd, struct wsdisplay_cmap *cmap)
{
	if (cmap) {
		if (ioctl(fb->fd, WSDISPLAYIO_PUTCMAP, cmap))
			fatal("ioctl: WSDISPLAYIO_PUTCMAP failed");
	}
}

void cmap_init(struct framebuffer *fb)
{
	extern const uint32_t color_list[]; /* global */
	int i;
	u_char r, g, b;

	if (ioctl(fb->fd, WSDISPLAYIO_GETCMAP, fb->cmap_org)) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		/* where is endian info? */
		r = bit_mask[8] & (color_list[i] >> 16);
		g = bit_mask[8] & (color_list[i] >> 8);
		b = bit_mask[8] & (color_list[i] >> 0);

		*(fb->cmap->red   + i) = r;
		*(fb->cmap->green + i) = g;
		*(fb->cmap->blue  + i) = b;
	}

	cmap_update(fb->fd, fb->cmap);
}

static inline uint32_t color2pixel(struct wsdisplayio_fbinfo *vinfo, uint32_t color)
{
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

	/* pseudo color */
	if (vinfo->fbi_pixeltype == WSFB_CI) {
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
	r = r >> (BITS_PER_BYTE - vinfo->fbi_subtype.fbi_rgbmasks.red_size);
	g = g >> (BITS_PER_BYTE - vinfo->fbi_subtype.fbi_rgbmasks.green_size);
	b = b >> (BITS_PER_BYTE - vinfo->fbi_subtype.fbi_rgbmasks.blue_size);

	return (r << vinfo->fbi_subtype.fbi_rgbmasks.red_offset)
		+ (g << vinfo->fbi_subtype.fbi_rgbmasks.green_offset)
		+ (b << vinfo->fbi_subtype.fbi_rgbmasks.blue_offset);
}

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
{
	int i;
	char *path;
	struct wsdisplay_fbinfo finfo;
	struct wsdisplayio_fbinfo vinfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, WSDISPLAYIO_GINFO, &finfo))
		fatal("ioctl: WSDISPLAYIO_GINFO failed");

	if (ioctl(fb->fd, WSDISPLAYIO_GET_FBINFO, &vinfo))
		fatal("ioctl: WSDISPLAYIO_GET_FBINFO failed");

	fb->width  = finfo.width;
	fb->height = finfo.height;
	fb->line_length = vinfo.fbi_stride;
	fb->screen_size = vinfo.fbi_fbsize;

	if (vinfo.fbi_pixeltype == WSFB_RGB) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = my_ceil(vinfo.fbi_bitsperpixel, BITS_PER_BYTE);
	}
	else if (vinfo.fbi_pixeltype == WSFB_CI) {
		cmap_create(&fb->cmap);
		cmap_create(&fb->cmap_org);
		cmap_init(fb);
		fb->bpp = my_ceil(vinfo.fbi_bitsperpixel, BITS_PER_BYTE);
	}
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	//if (DEBUG)
		fprintf(stderr, "pixeltype:%d bitsperpixel:%d\n",
			vinfo.fbi_pixeltype, vinfo.fbi_bitsperpixel);

	for (i = 0; i < COLORS; i++) // init color palette
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
		ioctl(fb->fd, WSDISPLAYIO_PUTCMAP, fb->cmap_org); /* not fatal */
		cmap_die(fb->cmap_org);
	}
	free(fb->buf);
	free(fb->wall);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}

static inline void draw_line(struct framebuffer *fb, struct terminal *term, int line)
{
	int pos, size, bit_shift, margin_right;
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
		bit_shift = (cellp->width == WIDE) ? CELL_WIDTH: 0;

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
				if (glyphp->bitmap[h] & (0x01 << (bit_shift + w)))
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
		don't know how to page flip in BSD */

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
