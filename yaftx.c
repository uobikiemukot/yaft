/* See LICENSE for licence details. */
#include "yaft.h"
#include "conf.h"
#include "util.h"
#include "x.h"
#include "terminal.h"
#include "function.h"
#include "osc.h"
#include "dcs.h"
#include "parse.h"

void sig_handler(int signo)
{
	if (signo == SIGCHLD)
		tty.loop_flag = false;
	else if (signo == SIGWINCH)
		tty.window_resized = true;
}

void sig_set()
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGWINCH, &sigact, NULL);
}

void sig_reset()
{
 	/* no error handling */
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGWINCH, &sigact, NULL);
}

void check_fds(fd_set *fds, struct timeval *tv, int master)
{
	FD_ZERO(fds);
	FD_SET(master, fds);
	tv->tv_sec  = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, NULL, NULL, tv);
}

char *keymap(KeySym k, unsigned int state)
{
	int i, length;

	length = sizeof(key) / sizeof(key[0]);

	for(i = 0; i < length; i++) {
		unsigned int mask = key[i].mask;
		if(key[i].k == k &&
			((state & mask) == mask || (mask == XK_NO_MOD && !state)))
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
	else {
		if (size == 1 && (e->state & Mod1Mask)) {
			buf[1] = buf[0];
			buf[0] = '\033';
			size = 2;
		}
		ewrite(term->fd, buf, size);
	}
}

void xresize(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	XConfigureEvent *e = &ev->xconfigure;
	struct winsize ws;

	if (DEBUG)
		fprintf(stderr, "term.width:%d term.height:%d width:%d height:%d\n",
			term->width, term->height, e->width, e->height);

	(void ) xw; /* unused */

	if (e->width != term->width || e->height != term->height) {
		term->width  = e->width;
		term->height = e->height;

		term->cols  = term->width / CELL_WIDTH;
		term->lines = term->height / CELL_HEIGHT;

		ws.ws_row = term->lines;
		ws.ws_col = term->cols;
		ws.ws_xpixel = ws.ws_ypixel + 0;

		//reset(term);
		refresh(xw, term);
		ioctl(term->fd, TIOCSWINSZ, &ws);
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

void xbpress(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	struct paste_t *paste = &xw->paste;

	if (ev->xbutton.button == Button1) { /* select start */
		init_paste(xw);
		paste->begin.x = ev->xbutton.x / CELL_WIDTH;
		paste->begin.y = ev->xbutton.y / CELL_HEIGHT;
		paste->copy_cursor_visible = true;
		term->line_dirty[paste->begin.y] = true;
		refresh(xw, term);

		if (DEBUG)
			fprintf(stderr, "copy begin (x, y) (%d, %d)\n",
				paste->begin.x, paste->begin.y);
	}
}

char *copy_cell2str(struct terminal *term,
	struct point_t begin, struct point_t end)
{
	int i, num;
	char *cp, *buf;
	size_t size;
	struct cell_t *cellp;
	wchar_t code;

	num = (end.x + end.y * term->cols) - (begin.x + begin.y * term->cols);
	if (num < 0) {
		cellp = &term->cells[end.y * term->cols + end.x];
		num *= -1;
	}
	else
		cellp = &term->cells[begin.y * term->cols + begin.x];

	buf = ecalloc(num + 1, 3); /* Unicode BMP: max 3 bytes per character */
	cp  = buf;

	for (i = 0; i <= num; i++) {
		if ((cellp + i)->width == NEXT_TO_WIDE || (cellp + i)->has_bitmap)
			continue;

		code = (cellp + i)->glyphp->code;
		if (DEBUG)
			fprintf(stderr, "num:%d code:%u\n", i, code);

		if (0 <= code && code < UCS2_CHARS) {
			size = wcrtomb(cp, code, NULL);
			cp   += size;
		}
	}

	return buf;
}

void xbrelease(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	Atom clipboard;
	struct paste_t *paste = &xw->paste;

	if (ev->xbutton.button == Button1) { /* select end */
		paste->end.x = ev->xbutton.x / CELL_WIDTH;
		paste->end.y = ev->xbutton.y / CELL_HEIGHT;

		paste->copy_cursor_visible = false;
		term->line_dirty[paste->begin.y] = true;
		term->line_dirty[paste->end.y]   = true;

		if (DEBUG)
			fprintf(stderr, "copy end (x, y) (%d, %d)\n",
				paste->end.x, paste->end.y);

		free(paste->str);
		paste->str = copy_cell2str(term, paste->begin, paste->end);
		clipboard = XInternAtom(xw->display, "CLIPBOARD", 0);
		XSetSelectionOwner(xw->display, XA_PRIMARY, xw->window, ev->xbutton.time);
		XSetSelectionOwner(xw->display, clipboard, xw->window, ev->xbutton.time);
	}
	else if (ev->xbutton.button == Button2) { /* paste */
		clipboard = XInternAtom(xw->display, "CLIPBOARD", 0);
		XConvertSelection(xw->display, clipboard, paste->target,
			XA_PRIMARY, xw->window, ev->xbutton.time);
	}
}

void xselnotify(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	int format;
	unsigned long nitems, ofs, rem;
	uint8_t *data;
	Atom type;

	(void) ev; /* unused */

	ofs = 0;
	do {
		if (XGetWindowProperty(xw->display, xw->window, XA_PRIMARY, ofs, BUFSIZE / 4,
			False, AnyPropertyType, &type, &format, &nitems, &rem, &data))
			return;

		ewrite(term->fd, (uint8_t *) data, nitems * format / 8);
		XFree(data);

		ofs += nitems * format / 32;
	} while (rem > 0);
}

void xselreq(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	XSelectionRequestEvent *xsre;
	XSelectionEvent xev;
	Atom xa_targets, string;
	struct paste_t *paste = &xw->paste;

	(void) term; /* unused */

	xsre = (XSelectionRequestEvent *) ev;
	xev.type = SelectionNotify;
	xev.requestor = xsre->requestor;
	xev.selection = xsre->selection;
	xev.target = xsre->target;
	xev.time = xsre->time;
	/* reject */
	xev.property = None;

	xa_targets = XInternAtom(xw->display, "TARGETS", 0);
	if(xsre->target == xa_targets) {
		/* respond with the supported type */
		string = paste->target;
		XChangeProperty(xsre->display, xsre->requestor, xsre->property,
				XA_ATOM, 32, PropModeReplace, (uint8_t *) &string, 1);
		xev.property = xsre->property;
	} else if(xsre->target == paste->target && paste->str != NULL) {
		XChangeProperty(xsre->display, xsre->requestor, xsre->property,
				xsre->target, 8, PropModeReplace, (uint8_t *) paste->str, strlen(paste->str));
		xev.property = xsre->property;
	}

	/* all done, send a notification to the listener */
	if(!XSendEvent(xsre->display, xsre->requestor, True, 0, (XEvent *) &xev))
		fprintf(stderr, "Error sending SelectionNotify event\n");
}

/*
void xselclear(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	(void) xw;
	(void) term;
	(void) ev;
	struct paste_t *paste = &xw->paste;

	term->line_dirty[paste->begin.y] = true;
	term->line_dirty[paste->end.y]   = true;
	refresh(xw, term);
}
*/

void xmove(struct xwindow *xw, struct terminal *term, XEvent *ev)
{
	struct paste_t *paste = &xw->paste;

	if (paste->copy_cursor_visible) {
		paste->current.x = ev->xmotion.x / CELL_WIDTH;
		paste->current.y = ev->xmotion.y / CELL_HEIGHT;

		if (paste->prev != -1)
			term->line_dirty[paste->prev] = true;
		term->line_dirty[paste->current.y] = true;

		refresh(xw, term);

		paste->prev = paste->current.y;
	}
}

void (*event_func[LASTEvent])(struct xwindow *xw, struct terminal *term, XEvent *ev) = {
	[KeyPress]         = xkeypress,
	[ConfigureNotify]  = xresize,
	[Expose]           = xredraw,
	[FocusIn]          = xfocus,
	[FocusOut]         = xfocus,
	[ButtonRelease]    = xbrelease,
	[ButtonPress]      = xbpress,
	[SelectionNotify]  = xselnotify,
	[SelectionRequest] = xselreq,
	[MotionNotify]     = xmove,
	/*
	[SelectionClear]   = xselclear,
	*/
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
	struct winsize ws;
	XEvent ev;
	XConfigureEvent confev;

	/* init */
	setlocale(LC_ALL, "");
	sig_set();
	xw_init(&xw);
	term_init(&term, xw.width, xw.height);

	/* fork and exec shell */
	fork_and_exec(&term.fd);
	confev.width  = TERM_WIDTH;
	confev.height = TERM_HEIGHT;
	xresize(&xw, &term, (XEvent *) &confev); /* terminal size defined in x.h */

	/* main loop */
	while (tty.loop_flag) {
		if (tty.window_resized) {
			ioctl(term.fd, TIOCGWINSZ, &ws);
			if (DEBUG)
				fprintf(stderr, "window resized! cols:%d lines:%d\n", ws.ws_col, ws.ws_row);
			term.cols   = ws.ws_col;
			term.lines  = ws.ws_row;
			term.width  = ws.ws_col * CELL_WIDTH;
			term.height = ws.ws_row * CELL_HEIGHT;
			reset(&term);
			tty.window_resized = false;
		}

		while(XPending(xw.display)) {
			XNextEvent(xw.display, &ev);
			if (XFilterEvent(&ev, None))
				continue;
			if (event_func[ev.type])
				event_func[ev.type](&xw, &term, &ev);
		}

		check_fds(&fds, &tv, term.fd);
		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				if (DEBUG)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				refresh(&xw, &term);
			}
		}
	}

	/* die */
	term_die(&term);
	xw_die(&xw);
	sig_reset();

	return EXIT_SUCCESS;
}
