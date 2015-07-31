/* See LICENSE for licence details. */
#include "../yaft.h"
#include "../conf.h"
#include "../util.h"
#include "x.h"
#include "../terminal.h"
#include "../ctrlseq/esc.h"
#include "../ctrlseq/csi.h"
#include "../ctrlseq/osc.h"
#include "../ctrlseq/dcs.h"
#include "../parse.h"

void sig_handler(int signo)
{
	extern volatile sig_atomic_t child_alive;

	if (signo == SIGCHLD) {
		child_alive = false;
		wait(NULL);
	}
}

bool sig_set(int signo, void (*handler)(int signo), int flags)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags   = flags;

	if (esigaction(signo, &sigact, NULL) == -1) {
		logging(WARN, "sigaction: signo %d failed\n", signo);
		return false;
	}
	return true;
}

void check_fds(fd_set *fds, struct timeval *tv, int master)
{
	FD_ZERO(fds);
	FD_SET(master, fds);
	tv->tv_sec  = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, NULL, NULL, tv);
}

char *keyremap(KeySym keysym, unsigned int state)
{
	int length;
	unsigned int mask;

	length = sizeof(keymap) / sizeof(keymap[0]);

	for (int i = 0; i < length; i++) {
		mask = keymap[i].mask;
		if (keymap[i].keysym == keysym &&
			((state & mask) == mask || (mask == XK_NO_MOD && !state)))
			return (char *) keymap[i].str;
	}
	return NULL;
}

void xkeypress(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev)
{
	int size;
	char buf[BUFSIZE], *customkey;
	XKeyEvent *e = &ev->xkey;
	KeySym keysym;

	//size = XmbLookupString(xw->ic, e, buf, BUFSIZE, &keysym, NULL);
	(void) xw;

	size = XLookupString(e, buf, BUFSIZE, &keysym, NULL);
	if ((customkey = keyremap(keysym, e->state))) {
		ewrite(term->fd, customkey, strlen(customkey));
	} else {
		if (size == 1 && (e->state & Mod1Mask)) {
			buf[1] = buf[0];
			buf[0] = '\033';
			size = 2;
		}
		ewrite(term->fd, buf, size);
	}
}

void xresize(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev)
{
	XConfigureEvent *e = &ev->xconfigure;
	struct winsize ws;

	logging(DEBUG, "xresize() term.width:%d term.height:%d width:%d height:%d\n",
		term->width, term->height, e->width, e->height);

	(void ) xw; /* unused */

	if (e->width == term->width && e->height == term->height)
		return;

	term->width  = e->width;
	term->height = e->height;

	term->cols  = term->width / CELL_WIDTH;
	term->lines = term->height / CELL_HEIGHT;

	term->scroll.top = 0;
	term->scroll.bottom = term->lines - 1;

	ws.ws_col = term->cols;
	ws.ws_row = term->lines;
	ws.ws_xpixel = CELL_WIDTH * term->cols;
	ws.ws_ypixel = CELL_HEIGHT * term->lines;
	ioctl(term->fd, TIOCSWINSZ, &ws);
}

void xredraw(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev)
{
	XExposeEvent *e = &ev->xexpose;
	int i, lines, update_from;

	logging(DEBUG, "xredraw() count:%d x:%d y:%d width:%d height:%d\n",
		e->count, e->x, e->y, e->width, e->height);

	update_from = e->y / CELL_HEIGHT;
	lines       = my_ceil(e->height, CELL_HEIGHT);

	for (i = 0; i < lines; i++) {
		if ((update_from + i) < term->lines)
			term->line_dirty[update_from + i] = true;
	}

	if (e->count == 0)
		refresh(xw, term);
}

void (*event_func[LASTEvent])(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev) = {
	[KeyPress]         = xkeypress,
	[ConfigureNotify]  = xresize,
	[Expose]           = xredraw,
};

bool fork_and_exec(int *master, int lines, int cols)
{
	extern const char *shell_cmd; /* defined in conf.h */
	char *shell_env;
	pid_t pid;
	struct winsize ws = {.ws_row = lines, .ws_col = cols,
		/* XXX: this variables are UNUSED (man tty_ioctl),
			but useful for calculating terminal cell size */
		.ws_ypixel = CELL_HEIGHT * lines, .ws_xpixel = CELL_WIDTH * cols};

	pid = eforkpty(master, NULL, NULL, &ws);
	if (pid < 0)
		return false;
	else if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		if ((shell_env = getenv("SHELL")) != NULL)
			eexecl(shell_env);
		else
			eexecl(shell_cmd);
		/* never reach here */
		exit(EXIT_FAILURE);
	}
	return true;
}

int main()
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct xwindow_t xw;
	struct terminal_t term;
	XEvent ev;
	XConfigureEvent confev;
	extern volatile sig_atomic_t child_alive;

	/* init */
	if (!setlocale(LC_ALL, ""))
		logging(WARN, "setlocale falied\n");

	if (!sig_set(SIGCHLD, sig_handler, SA_RESTART))
		logging(ERROR, "signal initialize failed\n");

	if (!xw_init(&xw)) {
		logging(FATAL, "xwindow initialize failed\n");
		goto xw_init_failed;
	}

	if (!term_init(&term, xw.width, xw.height)) {
		logging(FATAL, "terminal initialize failed\n");
		goto term_init_failed;
	}

	/* fork and exec shell */
	if (!fork_and_exec(&term.fd, term.lines, term.cols)) {
		logging(FATAL, "forkpty failed\n");
		goto fork_failed;
	}
	child_alive = true;

	/* initial terminal size defined in x.h */
	confev.width  = TERM_WIDTH;
	confev.height = TERM_HEIGHT;
	xresize(&xw, &term, (XEvent *) &confev);

	/* main loop */
	while (child_alive) {
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
	sig_set(SIGCHLD, SIG_DFL, 0);
	return EXIT_SUCCESS;

fork_failed:
	term_die(&term);
term_init_failed:
	xw_die(&xw);
xw_init_failed:
	return EXIT_FAILURE;
}
