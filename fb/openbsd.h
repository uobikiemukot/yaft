/* See LICENSE for licence details. */
/* XXX: _XOPEN_SOURCE >= 600 invalidates __BSD_VISIBLE
	so define some types manually */
typedef unsigned char   unchar;
typedef unsigned char   u_char;
typedef unsigned short  ushort;
typedef unsigned int    u_int;
typedef unsigned long   u_long;
#include <sys/param.h>
#include <dev/wscons/wsdisplay_usl_io.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymdef.h>

typedef struct wsdisplay_cmap cmap_t;

enum {
	CMAP_COLOR_LENGTH = sizeof(u_char) * BITS_PER_BYTE,
	FB_WIDTH  = 640,
	FB_HEIGHT = 480,
	FB_DEPTH  = 8,
};

void alloc_cmap(cmap_t *cmap, int colors)
{
	cmap->index = 0;
	cmap->count = colors;
	cmap->red   = (u_char *) ecalloc(colors, sizeof(u_char));
	cmap->green = (u_char *) ecalloc(colors, sizeof(u_char));
	cmap->blue  = (u_char *) ecalloc(colors, sizeof(u_char));
}

int put_cmap(int fd, cmap_t *cmap)
{
	return ioctl(fd, WSDISPLAYIO_PUTCMAP, cmap);
}

int get_cmap(int fd, cmap_t *cmap)
{
	return ioctl(fd, WSDISPLAYIO_GETCMAP, cmap);
}

void set_bitfield(int depth, struct bitfield_t *red, struct bitfield_t *green, struct bitfield_t *blue)
{
	switch (depth) {
	case 15:
		red->offset = 10; green->offset = 5;  blue->offset = 0;
		red->length = 5;  green->length = 5;  blue->length = 5;
		break;
	case 16:
		red->offset = 11; green->offset = 5;  blue->offset = 0;
		red->length = 5;  green->length = 6;  blue->length = 5;
		break;
	case 24:
	case 32:
		red->offset = 16; green->offset = 8;  blue->offset = 0;
		red->length = 8;  green->length = 8;  blue->length = 8;
		break;
	default:
		break;
	}
}

void set_type_visual(struct fb_info_t *info)
{
	info->type = YAFT_FB_TYPE_PACKED_PIXELS;

	if (info->bits_per_pixel == 8)
		info->visual = YAFT_FB_VISUAL_PSEUDOCOLOR;
	else
		info->visual = YAFT_FB_VISUAL_TRUECOLOR;
}

bool set_fbinfo(int fd, struct fb_info_t *info)
{
	int mode;
	struct wsdisplay_fbinfo finfo;
	struct wsdisplay_gfx_mode gfx_mode = {.width = FB_WIDTH, .height = FB_HEIGHT, .depth = FB_DEPTH};

	/*
	if (ioctl(fd, WSDISPLAYIO_GMODE, &info->reserved)) {
		logging(ERROR, "ioctl: WSDISPLAYIO_GMODE failed\n");
		return false;
	}
	logging(DEBUG, "orig_mode:%d\n", info->reserved);
	*/

	if (ioctl(fd, WSDISPLAYIO_SETGFXMODE, &gfx_mode)) {
		logging(ERROR, "ioctl: WSDISPLAYIO_SETGFXMODE failed\n");
		goto set_fbinfo_failed;
	}

	mode = WSDISPLAYIO_MODE_DUMBFB;
	if (ioctl(fd, WSDISPLAYIO_SMODE, &mode)) {
		logging(ERROR, "ioctl: WSDISPLAYIO_SMODE failed\n");
		goto set_fbinfo_failed;
	}

	if (ioctl(fd, WSDISPLAYIO_GINFO, &finfo)) {
		logging(ERROR, "ioctl: WSDISPLAYIO_GINFO failed\n");
		goto set_fbinfo_failed;
	}
	logging(DEBUG, "finfo width:%d height:%d depth:%d\n", finfo.width, finfo.height, finfo.depth);

	info->width  = FB_WIDTH;
	info->height = FB_HEIGHT;

	info->bits_per_pixel  = FB_DEPTH;
	info->bytes_per_pixel = my_ceil(FB_DEPTH, BITS_PER_BYTE);

	info->line_length = info->bytes_per_pixel * info->width;
	info->screen_size = info->height * info->line_length;

	set_bitfield(info->bits_per_pixel, &info->red, &info->green, &info->blue);
	set_type_visual(info);

	return true;

set_fbinfo_failed:
	//ioctl(fd, WSDISPLAYIO_SMODE, &info->reserved);
	return false;
}

/*
void fb_release(int fd, struct fb_info_t *info)
{
	ioctl(fd, WSDISPLAYIO_SMODE, &info->reserved);
}
*/
