/* See LICENSE for licence details. */
/* XXX: _XOPEN_SOURCE >= 600 invalidates __BSD_VISIBLE
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

typedef struct fbcmap cmap_t;

enum {
	CMAP_COLOR_LENGTH = sizeof(u_char) * BITS_PER_BYTE,
};

/* os specific ioctl */
void alloc_cmap(cmap_t *cmap, int colors)
{
	/* commond member included in struct fbcmap and video_color_palette_t */
	cmap->index       = 0;
	cmap->count       = colors;
	cmap->red         = (u_char *) ecalloc(colors, sizeof(u_char));
	cmap->green       = (u_char *) ecalloc(colors, sizeof(u_char));
	cmap->blue        = (u_char *) ecalloc(colors, sizeof(u_char));
	/* not exist in struct fbcmap */
	//cmap->transparent = NULL;
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
void set_bitfield(video_info_t *vinfo,
	struct bitfield_t *red, struct bitfield_t *green, struct bitfield_t *blue)
{
	red->length   = vinfo->vi_pixel_fsizes[0];
	green->length = vinfo->vi_pixel_fsizes[1];
	blue->length  = vinfo->vi_pixel_fsizes[2];

	red->offset   = vinfo->vi_pixel_fields[0];
	green->offset = vinfo->vi_pixel_fields[1];
	blue->offset  = vinfo->vi_pixel_fields[2];
}

void set_type_visual(struct fb_info_t *info, int type, int visual)
{
	/* ref: jfbterm-FreeBSD http://www.ac.auone-net.jp/~baba/jfbterm/ */
	if (type == V_INFO_MM_PACKED || type == V_INFO_MM_DIRECT)
		info->type = YAFT_FB_TYPE_PACKED_PIXELS;
	else if (type == V_INFO_MM_PLANAR)
		info->type =  YAFT_FB_TYPE_PLANES;
	else
		info->type =  YAFT_FB_TYPE_UNKNOWN;

	/* in FreeBSD, Direct Color is not exist? */
	if (type == V_INFO_MM_DIRECT && info->bits_per_pixel >= 15)
		info->visual =  YAFT_FB_VISUAL_TRUECOLOR;
	else if (type == V_INFO_MM_PACKED && visual & V_ADP_PALETTE
		&& info->bits_per_pixel == 8)
		info->visual =  YAFT_FB_VISUAL_PSEUDOCOLOR;
	else
		info->visual =  YAFT_FB_VISUAL_UNKNOWN;
}

bool set_fbinfo(int fd, struct fb_info_t *info)
{
	int video_mode;
	video_info_t vinfo;
	video_adapter_info_t ainfo;

	if (ioctl(fd, FBIO_GETMODE, &video_mode)) {
		logging(ERROR, "ioctl: FBIO_GETMODE failed\n");
		return false;
	}

	vinfo.vi_mode = video_mode;
	if (ioctl(fd, FBIO_MODEINFO, &vinfo)) {
		logging(ERROR, "ioctl: FBIO_MODEINFO failed\n");
		return false;
	}

	if (ioctl(fd, FBIO_ADPINFO, &ainfo)) {
		logging(ERROR, "ioctl: FBIO_ADPINFO failed\n");
		return false;
	}

	set_bitfield(&vinfo, &info->red, &info->green, &info->blue);

	info->width  = vinfo.vi_width;
	info->height = vinfo.vi_height;
	info->screen_size = ainfo.va_window_size;
	info->line_length = ainfo.va_line_width;

	info->bits_per_pixel  = vinfo.vi_depth;
	info->bytes_per_pixel = my_ceil(vinfo.vi_depth, BITS_PER_BYTE);

	logging(DEBUG, "mem_model:%d va_flags:0x%X\n", vinfo.vi_mem_model, ainfo.va_flags);
	set_type_visual(info, vinfo.vi_mem_model, ainfo.va_flags);

	return true;
}
