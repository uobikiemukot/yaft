/* See LICENSE for licence details. */
#include "common.h"
#include "conf.h"
#include "color.h"
#include "glyph.h"
#include "util.h"
#include "x.h"
#include "terminal.h"
#include "function.h"
#include "osc.h"
#include "dcs.h"
#include "parse.h"

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
	XKeyEvent *e = &ev->xkey;
	KeySym keysym;

	size = XmbLookupString(xw->ic, e, buf, BUFSIZE, &keysym, NULL);
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

		term->lines = term->height / CELL_HEIGHT;
		term->cols = term->width / CELL_WIDTH;

		reset(term);
		ioctl(term->fd, TIOCSWINSZ, &(struct winsize)
			{.ws_row = term->lines, .ws_col = term->cols, .ws_xpixel = 0, .ws_ypixel = 0});
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

	if (ev->type == FocusIn) {
		tty.visible = true;
		XSetICFocus(xw->ic);
	}
	else {
		tty.visible = false;
		XUnsetICFocus(xw->ic);
		XmbResetIC(xw->ic);
	}

	refresh(xw, term);
}

static void (*event_func[LASTEvent])(struct xwindow *xw, struct terminal *term, XEvent *ev) = {
	[KeyPress] = xkeypress,
	[ConfigureNotify] = xresize,
	[Expose] = xredraw,
	[FocusIn] = xfocus,
	[FocusOut] = xfocus,
};

void fork_and_exec(int *master)
{
	pid_t pid;

	pid = eforkpty(master, NULL, NULL, NULL);

    if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		eexecvp(shell_cmd, (const char *[]){shell_cmd, NULL});
    }
}

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
	if (atexit(tty_die) != 0)
		fatal("atexit failed");

	tty_init();
	xw_init(&xw);
	term_init(&term, xw.width, xw.height);

	/* fork and exec shell */
	fork_and_exec(&term.fd);
	xresize(&xw, &term, (XEvent *) /* terminal size defined in x.h */
		&(XConfigureEvent){.width = TERM_WIDTH, .height = TERM_HEIGHT});

	/* main loop */
	while (tty.loop_flag) {
		check_fds(&fds, &tv, term.fd);

		while(XPending(xw.display)) {
			XNextEvent(xw.display, &ev);
			if (XFilterEvent(&ev, None))
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
				if (LAZY_DRAW && size == BUFSIZE) /* lazy drawing */
					continue;
				refresh(&xw, &term);
			}
		}
	}

	/* die */
	term_die(&term);
	xw_die(&xw);

	return EXIT_SUCCESS;
}
