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

	if (signo == SIGCHLD)
		child_alive = false;
}

void sig_set()
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
}

void sig_reset()
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

void xkeypress(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev)
{
	int size;
	char buf[BUFSIZE], *customkey;
	XKeyEvent *e = &ev->xkey;
	KeySym keysym;

	(void) xw;

	size = XLookupString(e, buf, BUFSIZE, &keysym, NULL);
	if ((customkey = keymap(keysym, e->state))) {
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

	if (DEBUG)
		fprintf(stderr, "xresize() term.width:%d term.height:%d width:%d height:%d\n",
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
	ws.ws_xpixel = ws.ws_ypixel = 0;
	ioctl(term->fd, TIOCSWINSZ, &ws);
}

void xredraw(struct xwindow_t *xw, struct terminal_t *term, XEvent *ev)
{
	XExposeEvent *e = &ev->xexpose;
	int i, lines, update_from;

	if (DEBUG)
		fprintf(stderr, "xredraw() count:%d x:%d y:%d width:%d height:%d\n",
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

void fork_and_exec(int *master)
{
	char *shell_env;
	pid_t pid;

	pid = eforkpty(master, NULL, NULL, NULL);

    if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		if ((shell_env = getenv("SHELL")) != NULL)
			eexecvp(shell_env, (const char *[]){shell_env, NULL});
		else
			eexecvp(shell_cmd, (const char *[]){shell_cmd, NULL});
    }
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
	setlocale(LC_ALL, "");
	sig_set();
	xw_init(&xw);
	term_init(&term, xw.width, xw.height);

	/* fork and exec shell */
	fork_and_exec(&term.fd);
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
	sig_reset();

	return EXIT_SUCCESS;
}
