/* See LICENSE for licence details. */
/* common framebuffer struct/enum */
enum fb_type_t {
	YAFT_FB_TYPE_PACKED_PIXELS = 0,
	YAFT_FB_TYPE_PLANES,
	YAFT_FB_TYPE_UNKNOWN,
};

enum fb_visual_t {
	YAFT_FB_VISUAL_TRUECOLOR = 0,
	YAFT_FB_VISUAL_DIRECTCOLOR,
	YAFT_FB_VISUAL_PSEUDOCOLOR,
	YAFT_FB_VISUAL_UNKNOWN,
};

struct fb_info_t {
	struct bitfield_t {
		int length;
		int offset;
	} red, green, blue;
	int width, height;       /* display resolution */
	long screen_size;        /* screen data size (byte) */
	int line_length;         /* line length (byte) */
	int bytes_per_pixel;
	int bits_per_pixel;
	enum fb_type_t type;
	enum fb_visual_t visual;
	int reserved;            /* os specific data */
};

/* os dependent typedef/include */
#if defined(__linux__)
    #include "linux.h"
#elif defined(__FreeBSD__)
    #include "freebsd.h"
#elif defined(__NetBSD__)
    #include "netbsd.h"
#elif defined(__OpenBSD__)
    #include "openbsd.h"
#endif

struct framebuffer_t {
	int fd;                        /* file descriptor of framebuffer */
	uint8_t *fp;                   /* pointer of framebuffer */
	uint8_t *buf;                  /* copy of framebuffer */
	uint8_t *wall;                 /* buffer for wallpaper */
	uint32_t real_palette[COLORS]; /* hardware specific color palette */
	struct fb_info_t info;
	cmap_t *cmap, *cmap_orig;
};

/* common framebuffer functions */
uint8_t *load_wallpaper(uint8_t *fp, long screen_size)
{
	uint8_t *ptr;

	if ((ptr = (uint8_t *) ecalloc(1, screen_size)) == NULL) {
		logging(ERROR, "couldn't allocate wallpaper buffer\n");
		return NULL;
	}
	memcpy(ptr, fp, screen_size);

	return ptr;
}

void cmap_die(cmap_t *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		//free(cmap->transp);
		free(cmap);
	}
}

cmap_t *cmap_create(int colors)
{
	cmap_t *cmap;

	if ((cmap = (cmap_t *) ecalloc(1, sizeof(cmap_t))) == NULL) {
		logging(ERROR, "couldn't allocate cmap buffer\n");
		return NULL;
	}

	/* init os specific */
	alloc_cmap(cmap, colors);

	if (!cmap->red || !cmap->green || !cmap->blue) {
		logging(ERROR, "couldn't allocate red/green/blue buffer of cmap\n");
		cmap_die(cmap);
		return NULL;
	}
	return cmap;
}

bool cmap_update(int fd, cmap_t *cmap)
{
	if (cmap) {
		if (put_cmap(fd, cmap)) {
			logging(ERROR, "put_cmap failed\n");
			return false;
		}
	}
	return true;
}

bool cmap_save(int fd, cmap_t *cmap)
{
	if (get_cmap(fd, cmap)) {
		logging(WARN, "get_cmap failed\n");
		return false;
	}
	return true;
}

bool cmap_init(int fd, struct fb_info_t *info, cmap_t *cmap, int colors, int length)
{
	uint16_t r, g, b, r_index, g_index, b_index;

	for (int i = 0; i < colors; i++) {
		if (info->visual == YAFT_FB_VISUAL_DIRECTCOLOR) {
			r_index = (i >> (length - info->red.length))   & bit_mask[info->red.length];
			g_index = (i >> (length - info->green.length)) & bit_mask[info->green.length];
			b_index = (i >> (length - info->blue.length))  & bit_mask[info->blue.length];

			/* XXX: maybe only upper order byte is used */
			r = r_index << (CMAP_COLOR_LENGTH - info->red.length);
			g = g_index << (CMAP_COLOR_LENGTH - info->green.length);
			b = b_index << (CMAP_COLOR_LENGTH - info->blue.length);

			*(cmap->red   + r_index) = r;
			*(cmap->green + g_index) = g;
			*(cmap->blue  + b_index) = b;
		} else { /* YAFT_FB_VISUAL_PSEUDOCOLOR */
			r_index = (i >> info->red.offset)   & bit_mask[info->red.length];
			g_index = (i >> info->green.offset) & bit_mask[info->green.length];
			b_index = (i >> info->blue.offset)  & bit_mask[info->blue.length];

			r = r_index << (CMAP_COLOR_LENGTH - info->red.length);
			g = g_index << (CMAP_COLOR_LENGTH - info->green.length);
			b = b_index << (CMAP_COLOR_LENGTH - info->blue.length);

			*(cmap->red   + i) = r;
			*(cmap->green + i) = g;
			*(cmap->blue  + i) = b;
		}
	}

	if (!cmap_update(fd, cmap))
		return false;

	return true;
}

static inline uint32_t color2pixel(struct fb_info_t *info, uint32_t color)
{
	uint32_t r, g, b;

	r = bit_mask[BITS_PER_RGB] & (color >> (BITS_PER_RGB * 2));
	g = bit_mask[BITS_PER_RGB] & (color >>  BITS_PER_RGB);
	b = bit_mask[BITS_PER_RGB] & (color >>  0);

	r = r >> (BITS_PER_RGB - info->red.length);
	g = g >> (BITS_PER_RGB - info->green.length);
	b = b >> (BITS_PER_RGB - info->blue.length);

	return (r << info->red.offset)
			+ (g << info->green.offset)
			+ (b << info->blue.offset);
}


bool init_truecolor(struct fb_info_t *info, cmap_t **cmap, cmap_t **cmap_orig)
{
	switch(info->bits_per_pixel) {
	case 15: /* XXX: 15 bpp is not tested */
	case 16:
	case 24:
	case 32:
		break;
	default:
		logging(ERROR, "truecolor %d bpp not supported\n", info->bits_per_pixel);
		return false;
	}
	/* we don't use cmap in truecolor */
	*cmap = *cmap_orig = NULL;

	return true;
}

bool init_indexcolor(int fd, struct fb_info_t *info, cmap_t **cmap, cmap_t **cmap_orig)
{
	int colors, max_length;

	if (info->visual == YAFT_FB_VISUAL_DIRECTCOLOR) {
		switch(info->bits_per_pixel) {
		case 15: /* XXX: 15 bpp is not tested */
		case 16:
		case 24: /* XXX: 24 bpp is not tested */
		case 32:
			break;
		default:
			logging(ERROR, "directcolor %d bpp not supported\n", info->bits_per_pixel);
			return false;
		}
		logging(DEBUG, "red.length:%d gree.length:%d blue.length:%d\n",
			info->red.length, info->green.length, info->blue.length);

		max_length = (info->red.length > info->green.length) ? info->red.length: info->green.length;
		max_length = (max_length > info->blue.length) ? max_length: info->blue.length;
	} else { /* YAFT_FB_VISUAL_PSEUDOCOLOR */
		switch(info->bits_per_pixel) {
		case 8:
			break;
		default:
			logging(ERROR, "pseudocolor %d bpp not supported\n", info->bits_per_pixel);
			return false;
		}

		/* in pseudo color, we use fixed palette (red/green: 3bit, blue: 2bit).
			this palette is not compatible with xterm 256 colors,
			but we can convert 24bit color into palette number easily. */
		info->red.length   = 3;
		info->green.length = 3;
		info->blue.length  = 2;

		info->red.offset   = 5;
		info->green.offset = 2;
		info->blue.offset  = 0;

		/* XXX: not 3 but 8, each color has 256 colors palette */
		max_length = 8;
	}

	colors = 1 << max_length;
	logging(DEBUG, "colors:%d max_length:%d\n", colors, max_length);

	*cmap      = cmap_create(colors);
	*cmap_orig = cmap_create(colors);

	if (!(*cmap) || !(*cmap_orig))
		goto cmap_init_err;

	if (!cmap_save(fd, *cmap_orig)) {
		logging(WARN, "couldn't save original cmap\n");
		cmap_die(*cmap_orig);
		*cmap_orig = NULL;
	}

	if (!cmap_init(fd, info, *cmap, colors, max_length))
		goto cmap_init_err;

	return true;

cmap_init_err:
	cmap_die(*cmap);
	cmap_die(*cmap_orig);
	return false;
}

void fb_print_info(struct fb_info_t *info)
{
	const char *type_str[] = {
		[YAFT_FB_TYPE_PACKED_PIXELS] = "YAFT_FB_TYPE_PACKED_PIXELS",
		[YAFT_FB_TYPE_PLANES]        = "YAFT_FB_TYPE_PLANES",
		[YAFT_FB_TYPE_UNKNOWN]       = "YAFT_FB_TYPE_UNKNOWN",
	};

	const char *visual_str[] = {
		[YAFT_FB_VISUAL_TRUECOLOR]   = "YAFT_FB_VISUAL_TRUECOLOR",
		[YAFT_FB_VISUAL_DIRECTCOLOR] = "YAFT_FB_VISUAL_DIRECTCOLOR",
		[YAFT_FB_VISUAL_PSEUDOCOLOR] = "YAFT_FB_VISUAL_PSEUDOCOLOR",
		[YAFT_FB_VISUAL_UNKNOWN]     = "YAFT_FB_VISUAL_UNKNOWN",
	};

	logging(DEBUG, "framebuffer info:\n");
	logging(DEBUG, "\tred(off:%d len:%d) green(off:%d len:%d) blue(off:%d len:%d)\n",
		info->red.offset, info->red.length, info->green.offset, info->green.length, info->blue.offset, info->blue.length);
	logging(DEBUG, "\tresolution %dx%d\n", info->width, info->height);
	logging(DEBUG, "\tscreen size:%ld line length:%d\n", info->screen_size, info->line_length);
	logging(DEBUG, "\tbits_per_pixel:%d bytes_per_pixel:%d\n", info->bits_per_pixel, info->bytes_per_pixel);
	logging(DEBUG, "\ttype:%s\n", type_str[info->type]);
	logging(DEBUG, "\tvisual:%s\n", visual_str[info->visual]);
}

bool fb_init(struct framebuffer_t *fb)
{
	extern const uint32_t color_list[COLORS]; /* defined in color.h */
	extern const char *fb_path;               /* defined in conf.h */
	const char *path;
	char *env;

	/* open framebuffer device: check FRAMEBUFFER env at first */
	path = ((env = getenv("FRAMEBUFFER")) == NULL) ? fb_path: env;
	if ((fb->fd = eopen(path, O_RDWR)) < 0)
		return false;

	/* os dependent initialize */
	if (!set_fbinfo(fb->fd, &fb->info))
		goto set_fbinfo_failed;

	if (VERBOSE)
		fb_print_info(&fb->info);

	/* allocate memory */
	fb->fp   = (uint8_t *) emmap(0, fb->info.screen_size,
				PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf  = (uint8_t *) ecalloc(1, fb->info.screen_size);
	fb->wall = ((env = getenv("YAFT")) && strstr(env, "wall")) ?
				load_wallpaper(fb->fp, fb->info.screen_size): NULL;

	/* error check */
	if (fb->fp == MAP_FAILED || !fb->buf)
		goto allocate_failed;

	if (fb->info.type != YAFT_FB_TYPE_PACKED_PIXELS) {
		/* TODO: support planes type */
		logging(ERROR, "unsupport framebuffer type\n");
		goto fb_init_failed;
	}

	if (fb->info.visual == YAFT_FB_VISUAL_TRUECOLOR) {
		if (!init_truecolor(&fb->info, &fb->cmap, &fb->cmap_orig))
			goto fb_init_failed;
	} else if (fb->info.visual == YAFT_FB_VISUAL_DIRECTCOLOR
		|| fb->info.visual == YAFT_FB_VISUAL_PSEUDOCOLOR) {
		if (!init_indexcolor(fb->fd, &fb->info, &fb->cmap, &fb->cmap_orig))
			goto fb_init_failed;
	} else {
		/* TODO: support mono visual */
		logging(ERROR, "unsupport framebuffer visual\n");
		goto fb_init_failed;
	}

	/* init color palette */
	for (int i = 0; i < COLORS; i++)
		fb->real_palette[i] = color2pixel(&fb->info, color_list[i]);

	return true;

fb_init_failed:
allocate_failed:
	free(fb->buf);
	free(fb->wall);
	if (fb->fp != MAP_FAILED)
		emunmap(fb->fp, fb->info.screen_size);
set_fbinfo_failed:
	eclose(fb->fd);
	return false;
}

void fb_die(struct framebuffer_t *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_orig) {
		put_cmap(fb->fd, fb->cmap_orig);
		cmap_die(fb->cmap_orig);
	}
	free(fb->wall);
	free(fb->buf);
	emunmap(fb->fp, fb->info.screen_size);
	eclose(fb->fd);
	//fb_release(fb->fd, &fb->info); /* os specific */
}

static inline void draw_sixel(struct framebuffer_t *fb, int line, int col, uint8_t *pixmap)
{
	int h, w, src_offset, dst_offset;
	uint32_t pixel, color = 0;

	for (h = 0; h < CELL_HEIGHT; h++) {
		for (w = 0; w < CELL_WIDTH; w++) {
			src_offset = BYTES_PER_PIXEL * (h * CELL_WIDTH + w);
			memcpy(&color, pixmap + src_offset, BYTES_PER_PIXEL);

			dst_offset = (line * CELL_HEIGHT + h) * fb->info.line_length
				+ (col * CELL_WIDTH + w) * fb->info.bytes_per_pixel;
			pixel = color2pixel(&fb->info, color);
			memcpy(fb->buf + dst_offset, &pixel, fb->info.bytes_per_pixel);
		}
	}
}

static inline void draw_line(struct framebuffer_t *fb, struct terminal_t *term, int line)
{
	int pos, size, bdf_padding, glyph_width, margin_right;
	int col, w, h;
	uint32_t pixel;
	struct color_pair_t color_pair;
	struct cell_t *cellp;

	for (col = term->cols - 1; col >= 0; col--) {
		margin_right = (term->cols - 1 - col) * CELL_WIDTH;

		/* target cell */
		cellp = &term->cells[line][col];

		/* draw sixel pixmap */
		if (cellp->has_pixmap) {
			draw_sixel(fb, line, col, cellp->pixmap);
			continue;
		}

		/* copy current color_pair (maybe changed) */
		color_pair = cellp->color_pair;

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
			color_pair.bg = (!vt_active && BACKGROUND_DRAW) ? PASSIVE_CURSOR_COLOR: ACTIVE_CURSOR_COLOR;
		}

		for (h = 0; h < CELL_HEIGHT; h++) {
			/* if UNDERLINE attribute on, swap bg/fg */
			if ((h == (CELL_HEIGHT - 1)) && (cellp->attribute & attr_mask[ATTR_UNDERLINE]))
				color_pair.bg = color_pair.fg;

			for (w = 0; w < CELL_WIDTH; w++) {
				pos = (term->width - 1 - margin_right - w) * fb->info.bytes_per_pixel
					+ (line * CELL_HEIGHT + h) * fb->info.line_length;

				/* set color palette */
				if (cellp->glyphp->bitmap[h] & (0x01 << (bdf_padding + w)))
					pixel = fb->real_palette[color_pair.fg];
				else if (fb->wall && color_pair.bg == DEFAULT_BG) /* wallpaper */
					memcpy(&pixel, fb->wall + pos, fb->info.bytes_per_pixel);
				else
					pixel = fb->real_palette[color_pair.bg];

				/* update copy buffer only */
				memcpy(fb->buf + pos, &pixel, fb->info.bytes_per_pixel);
			}
		}
	}

	/* actual display update (bit blit) */
	pos = (line * CELL_HEIGHT) * fb->info.line_length;
	size = CELL_HEIGHT * fb->info.line_length;
	memcpy(fb->fp + pos, fb->buf + pos, size);

	/* TODO: page flip
		if fb_fix_screeninfo.ypanstep > 0, we can use hardware panning.
		set fb_fix_screeninfo.{yres_virtual,yoffset} and call ioctl(FBIOPAN_DISPLAY)
		but drivers  of recent hardware (inteldrmfb, nouveaufb, radeonfb) don't support...
		(maybe we can use this by using libdrm) */
	/* TODO: vertical synchronizing */

	term->line_dirty[line] = ((term->mode & MODE_CURSOR) && term->cursor.y == line) ? true: false;
}

void refresh(struct framebuffer_t *fb, struct terminal_t *term)
{
	if (term->palette_modified) {
		term->palette_modified = false;
		for (int i = 0; i < COLORS; i++)
			fb->real_palette[i] = color2pixel(&fb->info, term->virtual_palette[i]);
	}

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (int line = 0; line < term->lines; line++) {
		if (term->line_dirty[line]) {
			draw_line(fb, term, line);
		}
	}
}
