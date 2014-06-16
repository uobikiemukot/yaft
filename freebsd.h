/* See LICENSE for licence details. */

/* _XOPEN_SOURCE >= 600 invalidates __BSD_VISIBLE
	so define some types manually */
typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#include <machine/param.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#include <sys/kbio.h>
#include <sys/types.h>

/* some structs for FreeBSD */
struct framebuffer {
	char *fp;             /* pointer of framebuffer(read only) */
	char *wall;           /* buffer for wallpaper */
	char *buf;            /* copy of framebuffer */
	int fd;               /* file descriptor of framebuffer */
	int width, height;    /* display resolution */
	long screen_size;     /* screen data size (byte) */
	int line_length;      /* line length (byte) */
	int bpp;              /* BYTES per pixel */
	video_color_palette_t /* cmap for legacy framebuffer (8bpp pseudocolor) */
		*cmap, *cmap_org;
	video_info_t vinfo;
};

/* common functions */
char *load_wallpaper(struct framebuffer *fb)
{
	char *ptr;

	ptr = (char *) ecalloc(1, fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

/* some functions for FreeBSD framebuffer */
void cmap_create(video_color_palette_t **cmap)
{
	*cmap                = (video_color_palette_t *) ecalloc(1, sizeof(video_color_palette_t));
	(*cmap)->index       = 0;
	(*cmap)->count       = COLORS;
	(*cmap)->red         = (u_char *) ecalloc(COLORS, sizeof(u_char));
	(*cmap)->green       = (u_char *) ecalloc(COLORS, sizeof(u_char));
	(*cmap)->blue        = (u_char *) ecalloc(COLORS, sizeof(u_char));
	(*cmap)->transparent = NULL;
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

void cmap_init(struct framebuffer *fb)
{
	extern const uint32_t color_list[]; /* global */
	int i;
	u_char r, g, b;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
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

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0)
		fatal("ioctl: FBIOPUTCMAP failed");
}

static inline uint32_t color2pixel(video_info_t *vinfo, uint32_t color)
{
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

	/* pseudo color */
	if (vinfo->vi_depth == 8) {
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
	r = r >> (BITS_PER_BYTE - vinfo->vi_pixel_fsizes[0]);
	g = g >> (BITS_PER_BYTE - vinfo->vi_pixel_fsizes[1]);
	b = b >> (BITS_PER_BYTE - vinfo->vi_pixel_fsizes[2]);

	return (r << vinfo->vi_pixel_fields[0])
		+ (g << vinfo->vi_pixel_fields[1])
		+ (b << vinfo->vi_pixel_fields[2]);
}

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
{
	int i, video_mode;
	char *path;
	video_info_t vinfo;
	video_adapter_info_t ainfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIO_GETMODE, &video_mode) < 0)
		fatal("ioctl: FBIO_GETMODE failed");

	vinfo.vi_mode = video_mode;
	if (ioctl(fb->fd, FBIO_MODEINFO, &vinfo) < 0)
		fatal("ioctl: FBIO_MODEINFO failed");

	if (ioctl(fb->fd, FBIO_ADPINFO, &ainfo) < 0)
		fatal("ioctl: FBIO_ADPINFO failed");

	fb->width  = vinfo.vi_width;
	fb->height = vinfo.vi_height;
	fb->screen_size = ainfo.va_window_size;
	fb->line_length = ainfo.va_line_width;

	if ((vinfo.vi_mem_model == V_INFO_MM_PACKED || vinfo.vi_mem_model == V_INFO_MM_DIRECT)
		&& (vinfo.vi_depth == 15 || vinfo.vi_depth == 16
		|| vinfo.vi_depth == 24 || vinfo.vi_depth == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bpp = my_ceil(vinfo.vi_depth, BITS_PER_BYTE);
	}
	else if ((ainfo.va_flags & V_ADP_PALETTE) &&
		vinfo.vi_mem_model == V_INFO_MM_PACKED && vinfo.vi_depth == 8) {
		cmap_create(&fb->cmap);
		cmap_create(&fb->cmap_org);
		cmap_init(fb);
		fb->bpp = 1;
	}
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = (fb->bpp == 1) ? (uint32_t) i: color2pixel(&vinfo, color_list[i]);

	fb->fp    = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf   = (char *) ecalloc(1, fb->screen_size);
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
	int pos, size, bit_shift, margin_right;
	int col, w, h, src_offset, dst_offset;
	uint32_t pixel, color;
	struct color_pair_t color_pair;
	struct cell_t *cellp;
	const struct glyph_t *glyphp;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* get cell color and glyph */
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
		glyphp = cellp->glyphp;

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
