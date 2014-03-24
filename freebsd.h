/* See LICENSE for licence details. */
#include <machine/param.h>
#include <sys/consio.h>
#include <sys/fbio.h>
#include <sys/kbio.h>
#include <sys/types.h>

#include "common.h"

/* some structs for FreeBSD */
struct framebuffer {
	char *fp;			   /* pointer of framebuffer(read only) */
	char *wall;			 /* buffer for wallpaper */
	char *buf;			  /* copy of framebuffer */
	int fd;				 /* file descriptor of framebuffer */
	struct pair res;		/* resolution (x, y) */
	long screen_size;	   /* screen data size (byte) */
	int line_length;		/* line length (byte) */
	int bpp;				/* BYTES per pixel */
	uint32_t color_palette[COLORS];
	video_color_palette_t *cmap, *cmap_org;
	enum rotate_mode rotate;
};

#include "util.h"

/* some functions for FreeBSD framebuffer */
video_color_palette_t *cmap_create(video_info_t *video_info)
{
	video_color_palette_t *cmap;

	cmap = (video_color_palette_t *) emalloc(sizeof(video_color_palette_t));
	cmap->index = 0;
	cmap->count = COLORS;
	cmap->red = (u_char *) emalloc(sizeof(u_char) * COLORS);
	cmap->green = (u_char *) emalloc(sizeof(u_char) * COLORS);
	cmap->blue = (u_char *) emalloc(sizeof(u_char) * COLORS);
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

void fb_init(struct framebuffer *fb)
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

	/*
	if (video_mode != VIDEO_MODE) {
		fprintf(stderr, "current mode:%d request mode:%d\n", video_mode, VIDEO_MODE);
		fatal("video mode unmatch");
	}
	*/

	video_info.vi_mode = video_mode;
	if (ioctl(fb->fd, FBIO_MODEINFO, &video_info) < 0)
		fatal("ioctl: FBIO_MODEINFO failed");

	if (ioctl(fb->fd, FBIO_ADPINFO, &video_adapter_info) < 0)
		fatal("ioctl: FBIO_ADPINFO failed");

	fb->res.x = video_info.vi_width;
	fb->res.y = video_info.vi_height;

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
	else
		/* non packed pixel, mono color, grayscale: not implimented */
		fatal("unsupported framebuffer type");

	for (i = 0; i < COLORS; i++) /* init color palette */
		fb->color_palette[i] = get_color(&video_info, i);

	fb->fp = (char *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf = (char *) emalloc(fb->screen_size);
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

#include "draw.h"
