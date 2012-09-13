/* See LICENSE for licence details. */
#include "../common.h"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct xwindow xwindow;
struct xwindow {
	Display *dsp;
	Window win;
	Pixmap buf;
	GC gc;
	pair res;
	int sc;
};
