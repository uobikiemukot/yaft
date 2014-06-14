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
	int i;
	uint8_t r, g, b;
	//struct color_rgb color;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org) < 0) { /* not fatal */
		cmap_die(fb->cmap_org);
		fb->cmap_org = NULL;
	}

	for (i = 0; i < COLORS; i++) {
		/* where is endian info? */
		get_rgb(i, &color);
		*(fb->cmap->red + i)   = color.r;
		*(fb->cmap->green + i) = color.g;
		*(fb->cmap->blue + i)  = color.b;
	}

	if (ioctl(fb->fd, FBIOPUTCMAP, fb->cmap) < 0)
		fatal("ioctl: FBIOPUTCMAP failed");
}

uint32_t color2pixel(video_info_t *vinfo, uint32_t color, bool similar)
{
/*
	uint32_t r, g, b;
	struct color_rgb color;

	if (video_info->vi_depth == 8)
		return i;

	//get_rgb(i, &color);
	r = bit_mask[8] & (color_list[i] >> 16);
	g = bit_mask[8] & (color_list[i] >> 8);
	b = bit_mask[8] & (color_list[i] >> 0);
	r = r >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[0]);
	g = g >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[1]);
	b = b >> (BITS_PER_BYTE - video_info->vi_pixel_fsizes[2]);

	return (r << video_info->vi_pixel_fields[0])
		+ (g << video_info->vi_pixel_fields[1])
		+ (b << video_info->vi_pixel_fields[2]);
*/
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

	/* pseudo color */
	if (vinfo->vi_depth == 8) {
		if (similar) {              /* search similar color */
			if (r == g && r == b) { /* gray scale */
				r = 24 * r / 256;
				return 232 + r;
			}                       /* 6x6x6 color cube */
			r = 6 * r / 256;
			g = 6 * g / 256;
			b = 6 * b / 256;
			return 16 + (r * 36) + (g * 6) + b;
		}
		return color; /* just return color palette index */
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
	char *path, *env;
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
		color_palette[i] = color2pixel(&vinfo, i, false);

	fb->fp    = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf   = (char *) ecalloc(1, fb->screen_size);
	fb->vinfo = *vinfo;

	fb->wall = NULL;
	if (((env = getenv("YAFT")) != NULL)
		&& (strstr(env, "wallpaper") != NULL || strstr(env, "wall") != NULL))
			fb->wall = load_wallpaper(fb);
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		ioctl(fb->fd, FBIOPUTCMAP, fb->cmap_org); // not fatal
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
	const struct glyph_t *gp;

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
			color.bg = (!tty.visible && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
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
		don't know how to page flip in BSD */

	term->line_dirty[line] = ((term->mode & MODE_CURSOR) && term->cursor.y == line) ? true: false;
}

void draw_sixel(struct framebuffer *fb, struct sixel_canvas_t *sc)
{
	int h, w, fb_pos, sc_pos;
	uint32_t color, similar;
	struct point_t offset;

	offset.x = sc->ref_cell.x * CELL_WIDTH;
	offset.y = sc->ref_cell.y * CELL_HEIGHT;

	if (DEBUG)
		fprintf(stderr, "offset(%d, %d) width:%d height:%d\n",
			offset.x, offset.y, sc->width, sc->height);

	for (h = 0; h < sc->max_height; h++) {
		if ((offset.y + h) >= fb->height)
			break;
		for (w = 0; w < sc->max_width; w++) {
			if ((offset.x + w) >= fb->width)
				break;

			sc_pos  = w * sizeof(uint32_t) + h * sc->line_length;
			fb_pos  = (offset.x + w) * fb->bpp + (offset.y + h) * fb->line_length;

			memcpy(&color, sc->data + sc_pos, sizeof(uint32_t));
			similar = color2pixel(&fb->vinfo, color, true);

			memcpy(fb->buf + fb_pos, &similar, fb->bpp);
		}
		fb_pos = offset.x * fb->bpp + (offset.y + h) * fb->line_length;
		memcpy(fb->fp + fb_pos, fb->buf + fb_pos, sc->max_width * fb->bpp);
	}
}

void refresh(struct framebuffer *fb, struct terminal *term)
{
	int i, line;
	struct sixel_canvas_t *sc;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (line = 0; line < term->lines; line++) {
		if (term->line_dirty[line])
			draw_line(fb, term, line);
	}

	for (i = 0; i < MAX_SIXEL_CANVAS; i++) {
		sc = &term->sixel_canvas[i];
		if (sc->data != NULL)
			draw_sixel(fb, sc);
	}
}
