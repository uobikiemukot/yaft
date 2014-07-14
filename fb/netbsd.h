/* See LICENSE for licence details. */

/* _XOPEN_SOURCE >= 600 invalidates __BSD_VISIBLE
        so define some types manually */
typedef unsigned char   unchar;
typedef unsigned char   u_char;
typedef unsigned short  ushort;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

#include  <sys/param.h>
#include  <dev/wscons/wsdisplay_usl_io.h>
#include  <dev/wscons/wsconsio.h>
#include  <dev/wscons/wsksymdef.h>

/* some structs for NetBSD */
enum fbtype_t {
	FBTYPE_RGB = 0,
	FBTYPE_INDEX,
};

enum {
	BPP15 = 15,
	BPP16 = 16,
	BPP24 = 24,
	BPP32 = 32,
};

struct bitfield_t {
	uint8_t offset, length;
};

struct fbinfo_t {
	struct bitfield_t red;
	struct bitfield_t green;
	struct bitfield_t blue;
	enum fbtype_t fbtype;
};

struct framebuffer {
	uint8_t *fp;          /* pointer of framebuffer(read only) */
	uint8_t *wall;        /* buffer for wallpaper */
	uint8_t *buf;         /* copy of framebuffer */
	int fd;               /* file descriptor of framebuffer */
	int width, height;    /* display resolution */
	long screen_size;     /* screen data size (byte) */
	int line_length;      /* line length (byte) */
	int bytes_per_pixel;  /* BYTES per pixel */
	struct wsdisplay_cmap /* cmap for legacy framebuffer (8bpp pseudocolor) */
		*cmap, *cmap_org;
	struct fbinfo_t vinfo;
};

const struct fbinfo_t bpp_table[] = {
	[BPP15] = {.red = {.offset   = 10, .length   = 5},
		       .green = {.offset = 5,  .length = 5},
		       .blue  = {.offset = 0,  .length = 5}},
	[BPP16] = {.red = {.offset   = 11, .length   = 5},
		       .green = {.offset = 5,  .length = 6},
		       .blue  = {.offset = 0,  .length = 5}},
	[BPP24] = {.red = {.offset   = 16, .length   = 8},
		       .green = {.offset = 8,  .length = 8},
		       .blue  = {.offset = 0,  .length = 8}},
	[BPP32] = {.red = {.offset   = 16, .length   = 8},
		       .green = {.offset = 8,  .length = 8},
		       .blue  = {.offset = 0,  .length = 8}},
};

/* common functions */
uint8_t *load_wallpaper(struct framebuffer *fb)
{
	uint8_t *ptr;

	ptr = (uint8_t *) ecalloc(1, fb->screen_size);
	memcpy(ptr, fb->fp, fb->screen_size);

	return ptr;
}

/* some functions for NetBSD framebuffer */
void cmap_create(struct wsdisplay_cmap **cmap, int size)
{
	*cmap                = (struct wsdisplay_cmap *) ecalloc(1, sizeof(struct wsdisplay_cmap));
	(*cmap)->index       = 0;
	(*cmap)->count       = size;
	(*cmap)->red         = (u_char *) ecalloc(size, sizeof(u_char));
	(*cmap)->green       = (u_char *) ecalloc(size, sizeof(u_char));
	(*cmap)->blue        = (u_char *) ecalloc(size, sizeof(u_char));
}

void cmap_die(struct wsdisplay_cmap *cmap)
{
	if (cmap) {
		free(cmap->red);
		free(cmap->green);
		free(cmap->blue);
		free(cmap);
	}
}

void cmap_update(int fd, struct wsdisplay_cmap *cmap)
{
	if (cmap) {
		if (ioctl(fd, WSDISPLAYIO_PUTCMAP, cmap))
			fatal("ioctl: WSDISPLAYIO_PUTCMAP failed");
	}
}

void cmap_init(struct framebuffer *fb)
{
	extern const uint32_t color_list[]; /* global */
	int i;
	u_char r, g, b;

	if (ioctl(fb->fd, WSDISPLAYIO_GETCMAP, fb->cmap_org)) { /* not fatal */
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

static inline uint32_t color2pixel(struct fbinfo_t *vinfo, uint32_t color)
{
	uint32_t r, g, b;

	r = bit_mask[8] & (color >> 16);
	g = bit_mask[8] & (color >> 8);
	b = bit_mask[8] & (color >> 0);

	/* pseudo color */
	if (vinfo->fbtype == FBTYPE_INDEX) {
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

	return (r << vinfo->red.offset)
		+ (g << vinfo->green.offset)
		+ (b << vinfo->blue.offset);
}

void fb_init(struct framebuffer *fb, uint32_t *color_palette)
{
	int i, orig_mode, mode;
	char *path, *env;
	struct wsdisplay_fbinfo finfo;

	if ((path = getenv("FRAMEBUFFER")) != NULL)
		fb->fd = eopen(path, O_RDWR);
	else
		fb->fd = eopen(fb_path, O_RDWR);

	ioctl(fb->fd, WSDISPLAYIO_GMODE, &orig_mode);

	mode = WSDISPLAYIO_MODE_DUMBFB;
	if (ioctl(fb->fd, WSDISPLAYIO_SMODE, &mode))
		fatal("ioctl: WSDISPLAYIO_SMODE failed");

	if (ioctl(fb->fd, WSDISPLAYIO_GINFO, &finfo)) {
		fprintf(stderr, "ioctl: WSDISPLAYIO_GINFO failed");
		goto fb_init_error;
	}

	fb->width  = finfo.width;
	fb->height = finfo.height;
	fb->bytes_per_pixel = my_ceil(finfo.depth, BITS_PER_BYTE);

	fb->line_length = fb->width * fb->bytes_per_pixel;
	fb->screen_size = fb->height * fb->line_length;
	fb->vinfo = bpp_table[finfo.depth];

	if (finfo.depth == 15 || finfo.depth == 16
		|| finfo.depth == 24 || finfo.depth == 32) {
		fb->cmap = fb->cmap_org = NULL;
		fb->vinfo.fbtype = FBTYPE_RGB;
	}
	else if (finfo.depth == 8) {
		cmap_create(&fb->cmap, COLORS);
		cmap_create(&fb->cmap_org, finfo.cmsize);
		cmap_init(fb);
		fb->vinfo.fbtype = FBTYPE_INDEX;
	}
	else
		fatal("unsupported framebuffer type");

	if (DEBUG)
		fprintf(stderr, "cmsize:%d depth:%d width:%d height:%d line_length:%d\n",
			finfo.cmsize, finfo.depth, finfo.width, finfo.height, fb->line_length);

	for (i = 0; i < COLORS; i++) /* init color palette */
		color_palette[i] = (fb->bytes_per_pixel == 1) ? (uint32_t) i: color2pixel(&fb->vinfo, color_list[i]);

	fb->fp    = (uint8_t *) emmap(0, fb->screen_size, PROT_WRITE | PROT_READ, MAP_SHARED, fb->fd, 0);
	fb->buf   = (uint8_t *) ecalloc(1, fb->screen_size);
	//fb->wall  = (WALLPAPER && fb->bytes_per_pixel > 1) ? load_wallpaper(fb): NULL;

	if (((env = getenv("YAFT")) != NULL) && (strstr(env, "wall") != NULL))
		fb->wall = load_wallpaper(fb);
	else
		fb->wall = NULL;

	return;

fb_init_error:
	ioctl(fb->fd, WSDISPLAYIO_SMODE, &orig_mode);
	exit(EXIT_FAILURE);
}

void fb_die(struct framebuffer *fb)
{
	cmap_die(fb->cmap);
	if (fb->cmap_org) {
		ioctl(fb->fd, WSDISPLAYIO_PUTCMAP, fb->cmap_org); /* not fatal */
		cmap_die(fb->cmap_org);
	}
	free(fb->buf);
	free(fb->wall);
	emunmap(fb->fp, fb->screen_size);
	eclose(fb->fd);
}
