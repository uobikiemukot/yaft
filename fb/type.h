/* See LICENSE for licence details. */
#if defined(__linux__)
    #include <linux/fb.h>
    #include <linux/vt.h>
    #include <linux/kd.h>
    typedef struct fb_cmap cmap_t;
    //typedef __u32          cmap_length_t;
    //typedef __u16          cmap_color_t;
    #include "linux.h"
#elif defined(__FreeBSD__)
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
    //typedef video_color_palette_t cmap_t;
    //typedef int                   cmap_length_t;
    //typedef u_char                cmap_color_t;
    #include "freebsd.h"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
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
    //typedef int                   cmap_length_t;
    //typedef u_char                cmap_color_t;
#elif defined(__NetBSD__)
    #include "netbsd.h"
#elif defined(__OpenBSD__)
    #include "openbsd.h"
#endif
