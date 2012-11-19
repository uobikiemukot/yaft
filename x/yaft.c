/* See LICENSE for licence details. */
#include "common.h"
#include "../glyph.h"
#include "../util.h"
#include "x.h"
#include "../terminal.h"
#include "../function.h"
#include "../parse.h"

struct tty_state tty = {
	.save_tm = NULL,
	.visible = true,
	.redraw_flag = false,
	.loop_flag = true,
	.setmode = false,
};

void handler(int signo)
{
	if (signo == SIGCHLD)
		tty.loop_flag = false;
}

void tty_init()
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
}

void tty_die()
{
 	/* no error handling */
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
}

void check_fds(fd_set *fds, struct timeval *tv, int master)
{
	FD_ZERO(fds);
	FD_SET(master, fds);
	tv->tv_sec = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, tv);
}

char *keymap(KeySym k, unsigned int state)
{
	int i, length;

	length = sizeof(key) / sizeof(key[0]);

	for(i = 0; i < length; i++) {
		unsigned int mask = key[i].mask;
		if(key[i].k == k && ((state & mask) == mask || (mask == XK_NO_MOD && !state)))
			return (char *) key[i].s;
	}
	return NULL;
}

void xkeypress(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	int size;
	char buf[BUFSIZE], *customkey;
	XKeyEvent *e = &ev->xkey;;
	KeySym keysym;

	size = XLookupString(e, buf, BUFSIZE, &keysym, NULL);
	if ((customkey = keymap(keysym, e->state)))
		ewrite(term->fd, customkey, strlen(customkey));
	else
		ewrite(term->fd, buf, size);
}

void xresize(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	XConfigureEvent *e = &ev->xconfigure;

	if (e->width != term->width || e->height != term->height) {
		term->width = e->width;
		term->height = e->height;

		term->lines = term->height / cell_height;
		term->cols = term->width / cell_width;

		XFreePixmap(xw->dsp, xw->buf);
		xw->buf = XCreatePixmap(xw->dsp, xw->win,
			term->width, term->height, XDefaultDepth(xw->dsp, xw->sc));

		free(term->line_dirty);
		term->line_dirty = (bool *) emalloc(sizeof(bool) * term->lines);

		free(term->tabstop);
		term->tabstop = (bool *) emalloc(sizeof(bool) * term->cols);

		free(term->cells);
		term->cells = (struct cell *) emalloc(sizeof(struct cell) * term->cols * term->lines);

		reset(term);
		ioctl(term->fd, TIOCSWINSZ, &(struct winsize){.ws_row = term->lines, .ws_col = term->cols, .ws_xpixel = 0, .ws_ypixel = 0});
	}
}

void xredraw(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	XExposeEvent *e = &ev->xexpose;

	if (e->count == 0) {
		redraw(term);
		refresh(xw, term);
	}
}

void xfocus(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	extern struct tty_state tty;

	if (ev->type == FocusIn)
		tty.visible = true;
	else
		tty.visible = false;

	refresh(xw, term);
}

static void (*event_func[LASTEvent])(struct xwindow *xw, struct terminal *term, XEvent *ev) = {
	[KeyPress] = xkeypress,
	[ConfigureNotify] = xresize,
	[Expose] = xredraw,
	[FocusIn] = xfocus,
	[FocusOut] = xfocus,
};

int main()
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct xwindow xw;
	struct terminal term;
	XEvent ev;

	/* init */
	setlocale(LC_ALL, "");
	if (atexit(tty_die) != 0) {
		fprintf(stderr, "atexit failed\n");
		exit(EXIT_FAILURE);
	}

	x_init(&xw);
	term_init(&term, xw.res);
	tty_init(&tty);

	/* fork and exec shell */
	eforkpty(&term.fd, term.lines, term.cols);

	/* main loop */
	while (tty.loop_flag) {
		check_fds(&fds, &tv, term.fd);

		while(XPending(xw.dsp)) {
			XNextEvent(xw.dsp, &ev);
			if(XFilterEvent(&ev, xw.win))
				continue;
			if (event_func[ev.type])
				event_func[ev.type](&xw, &term, &ev);
		}

		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				if (DEBUG)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				if (size == BUFSIZE) /* lazy drawing */
					continue;
				refresh(&xw, &term);
			}
		}
	}

	/* die */
	term_die(&term);
	x_die(&xw);

	return EXIT_SUCCESS;
}
