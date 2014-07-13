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
	uint8_t *fp;          /* pointer of framebuffer(read only) */
	uint8_t *wall;        /* buffer for wallpaper */
	uint8_t *buf;         /* copy of framebuffer */
	int fd;               /* file descriptor of framebuffer */
	int width, height;    /* display resolution */
	long screen_size;     /* screen data size (byte) */
	int line_length;      /* line length (byte) */
	int bytes_per_pixel;  /* BYTES per pixel */
	video_color_palette_t /* cmap for legacy framebuffer (8bpp pseudocolor) */
		*cmap, *cmap_org;
	video_info_t vinfo;
};

/* common functions */
uint8_t *load_wallpaper(struct framebuffer *fb)
{
	uint8_t *ptr;

	ptr = (uint8_t *) ecalloc(1, fb->screen_size);
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

void cmap_update(int fd, video_color_palette_t *cmap)
{
	if (cmap) {
		if (ioctl(fd, FBIOPUTCMAP, cmap))
			fatal("ioctl: FBIOPUTCMAP failed");
	}
}

void cmap_init(struct framebuffer *fb)
{
	extern const uint32_t color_list[]; /* global */
	int i;
	u_char r, g, b;

	if (ioctl(fb->fd, FBIOGETCMAP, fb->cmap_org)) { /* not fatal */
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
	char *path, *env;
	video_info_t vinfo;
	video_adapter_info_t ainfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	if (ioctl(fb->fd, FBIO_GETMODE, &video_mode))
		fatal("ioctl: FBIO_GETMODE failed");

	vinfo.vi_mode = video_mode;
	if (ioctl(fb->fd, FBIO_MODEINFO, &vinfo))
		fatal("ioctl: FBIO_MODEINFO failed");

	if (ioctl(fb->fd, FBIO_ADPINFO, &ainfo))
		fatal("ioctl: FBIO_ADPINFO failed");

	fb->width  = vinfo.vi_width;
	fb->height = vinfo.vi_height;
	fb->screen_size = ainfo.va_window_size;
	fb->line_length = ainfo.va_line_width;

	if ((vinfo.vi_mem_model == V_INFO_MM_PACKED || vinfo.vi_mem_model == V_INFO_MM_DIRECT)
		&& (vinfo.vi_depth == 15 || vinfo.vi_depth == 16
		|| vinfo.vi_depth == 24 || vinfo.vi_depth == 32)) {
		fb->cmap = fb->cmap_org = NULL;
		fb->bytes_per_pixel = my_ceil(vinfo.vi_depth, BITS_PER_BYTE);
	}
	else if ((ainfo.va_flags & V_ADP_PALETTE) &&
		vinfo.vi_mem_model == V_INFO_MM_PACKED && vinfo.vi_depth == 8) {
		cmap_create(&fb->cmap);
		cmap_create(&fb->cmap_org);
		cmap_init(fb);
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
