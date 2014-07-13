/* See LICENSE for licence details. */
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

/* struct for Linux */
struct framebuffer {
	uint8_t *fp;                     /* pointer of framebuffer (read only) */
	uint8_t *buf;                    /* copy of framebuffer */
	uint8_t *wall;                   /* buffer for wallpaper */
	int fd;                          /* file descriptor of framebuffer */
	int width, height;               /* display resolution */
	long screen_size;                /* screen data size (byte) */
	int line_length;                 /* line length (byte) */
	int bytes_per_pixel;             /* BYTES per pixel */
	struct fb_cmap *cmap, *cmap_org; /* cmap for legacy framebuffer (8bpp pseudocolor) */
	struct fb_var_screeninfo vinfo;
};

/* common functions */
uint8_t *load_wallpaper(struct framebuffer *fb)
{
	uint8_t *ptr;

	ptr = (uint8_t *) ecalloc(1, fb->screen_size);
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
	char *path, *env;
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
		fb->bytes_per_pixel = my_ceil(vinfo.bits_per_pixel, BITS_PER_BYTE);
	}
	else if (finfo.visual == FB_VISUAL_PSEUDOCOLOR && vinfo.bits_per_pixel == 8) {
		cmap_create(&fb->cmap);
		cmap_create(&fb->cmap_org);
		cmap_init(fb, &vinfo);
		fb->bytes_per_pixel = 1;
	}
	else /* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = (fb->bytes_per_pixel == 1) ? (uint32_t) i: color2pixel(&vinfo, color_list[i]);

	fb->fp    = (uint8_t *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf   = (uint8_t *) ecalloc(1, fb->screen_size);
	//fb->wall  = (WALLPAPER && fb->bytes_per_pixel > 1) ? load_wallpaper(fb): NULL;
	fb->vinfo = vinfo;

	if (((env = getenv("YAFT")) != NULL)
		&& ((strstr(env, "wall") != NULL) || (strstr(env, "wallpaper") != NULL)))
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
	free(fb->buf);
	free(fb->wall);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}
