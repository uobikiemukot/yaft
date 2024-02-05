/* See LICENSE for licence details. */
#include <linux/fb.h>
#include <linux/vt.h>
#include <linux/kd.h>

typedef struct fb_cmap cmap_t;

enum {
	CMAP_COLOR_LENGTH = sizeof(__u16) * BITS_PER_BYTE,
};

/* os specific ioctl */
void alloc_cmap(cmap_t *cmap, int colors)
{
	cmap->start  = 0;
	cmap->len    = colors;
	cmap->red    = (__u16 *) ecalloc(colors, sizeof(__u16));
	cmap->green  = (__u16 *) ecalloc(colors, sizeof(__u16));
	cmap->blue   = (__u16 *) ecalloc(colors, sizeof(__u16));
	cmap->transp = NULL;
}

int put_cmap(int fd, cmap_t *cmap)
{
	return ioctl(fd, FBIOPUTCMAP, cmap);
}

int get_cmap(int fd, cmap_t *cmap)
{
	return ioctl(fd, FBIOGETCMAP, cmap);
}

/* initialize struct fb_info_t */
void set_bitfield(struct fb_var_screeninfo *vinfo,
	struct bitfield_t *red, struct bitfield_t *green, struct bitfield_t *blue)
{
	red->length   = vinfo->red.length;
	green->length = vinfo->green.length;
	blue->length  = vinfo->blue.length;

	red->offset   = vinfo->red.offset;
	green->offset = vinfo->green.offset;
	blue->offset  = vinfo->blue.offset;
}

enum fb_type_t set_type(__u32 type)
{
	if (type == FB_TYPE_PACKED_PIXELS)
		return YAFT_FB_TYPE_PACKED_PIXELS;
	else if (type == FB_TYPE_PLANES)
		return YAFT_FB_TYPE_PLANES;
	else
		return YAFT_FB_TYPE_UNKNOWN;
}

enum fb_visual_t set_visual(__u32 visual)
{
	if (visual == FB_VISUAL_TRUECOLOR)
		return YAFT_FB_VISUAL_TRUECOLOR;
	else if (visual == FB_VISUAL_DIRECTCOLOR)
		return YAFT_FB_VISUAL_DIRECTCOLOR;
	else if (visual == FB_VISUAL_PSEUDOCOLOR)
		return YAFT_FB_VISUAL_PSEUDOCOLOR;
	else
		return YAFT_FB_VISUAL_UNKNOWN;
}

bool set_fbinfo(int fd, struct fb_info_t *info)
{
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo)) {
		logging(ERROR, "ioctl: FBIOGET_FSCREENINFO failed\n");
		return false;
	}

	if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo)) {
		logging(ERROR, "ioctl: FBIOGET_VSCREENINFO failed\n");
		return false;
	}

	set_bitfield(&vinfo, &info->red, &info->green, &info->blue);

	info->width  = vinfo.xres;
	info->height = vinfo.yres;
	info->screen_size = finfo.smem_len;
	info->line_length = finfo.line_length;

	info->bits_per_pixel  = vinfo.bits_per_pixel;
	info->bytes_per_pixel = my_ceil(info->bits_per_pixel, BITS_PER_BYTE);

	info->type   = set_type(finfo.type);
	info->visual = set_visual(finfo.visual);

	/* check screen [xy]offset and initialize because linux console changes these values */
	if (vinfo.xoffset != 0 || vinfo.yoffset != 0) {
		vinfo.xoffset = vinfo.yoffset = 0;
		if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo))
			logging(WARN, "couldn't reset offset (x:%d y:%d)\n", vinfo.xoffset, vinfo.yoffset);
	}

	return true;
}
