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

/* some functions for FreeBSD framebuffer */
video_color_palette_t *cmap_create(video_info_t *video_info)
{
	video_color_palette_t *cmap;

	cmap = (video_color_palette_t *) ecalloc(sizeof(video_color_palette_t));
	cmap->index = 0;
	cmap->count = COLORS;
	cmap->red = (u_char *) ecalloc(sizeof(u_char) * COLORS);
	cmap->green = (u_char *) ecalloc(sizeof(u_char) * COLORS);
	cmap->blue = (u_char *) ecalloc(sizeof(u_char) * COLORS);
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

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
{
	int i, video_mode;
	char *path;
	video_info_t video_info;
	video_adapter_info_t video_adapter_info;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIO_GETMODE, &video_mode) < 0)
		fatal("ioctl: FBIO_GETMODE failed");

	video_info.vi_mode = video_mode;
	if (ioctl(fb->fd, FBIO_MODEINFO, &video_info) < 0)
		fatal("ioctl: FBIO_MODEINFO failed");

	if (ioctl(fb->fd, FBIO_ADPINFO, &video_adapter_info) < 0)
		fatal("ioctl: FBIO_ADPINFO failed");

	fb->width  = video_info.vi_width;
	fb->height = video_info.vi_height;

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
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = get_color(&video_info, i);

	fb->fp = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (char *) ecalloc(fb->screen_size);
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
