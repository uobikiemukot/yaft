/* See LICENSE for licence details. */
#include "commonx.h"
#include "../util.h"
#include "x.h"
#include "../load.h"
#include "../terminal.h"
#include "../function.h"
#include "../parse.h"

static int loop_flag = 1;

void sigchld(int signal)
{
	loop_flag = 0;
}

void check_fds(fd_set *fds, struct timeval *tv, int master)
{
	FD_ZERO(fds);
	FD_SET(master, fds);
	tv->tv_sec = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, tv);
}

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	tcgetattr(fd, save_tm);
	tm = *save_tm;
	tm.c_iflag = tm.c_oflag = RESET;
	tm.c_cflag &= ~CSIZE;
	tm.c_cflag |= CS8;
	tm.c_lflag &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN] = 1;			/* min data size (byte) */
	tm.c_cc[VTIME] = 0;			/* time out */
	tcsetattr(fd, TCSAFLUSH, &tm);
}

void xkeypress(xwindow *xw, terminal *term, XEvent *ev)
{
	int size;
	char buf[BUFSIZE];
	XKeyEvent *e = &ev->xkey;
	KeySym keysym;

	size = XLookupString(e, buf, BUFSIZE, &keysym, NULL);
	ewrite(term->fd, (u8 *) buf, size);
}

void xresize(xwindow *xw, terminal *term, XEvent *ev)
{
	int lines, cols;

	XFreePixmap(xw->dsp, xw->buf);

	lines = ev->xconfigure.height / term->cell_size.y;
	cols = ev->xconfigure.width / term->cell_size.x;

	xw->buf = XCreatePixmap(xw->dsp, xw->win,
		ev->xconfigure.width, ev->xconfigure.height,
		XDefaultDepth(xw->dsp, xw->sc));

	resize(term, lines, cols);
}

void xredraw(xwindow *xw, terminal *term, XEvent *ev)
{
	int i;

	for (i = 0; i < term->lines; i++)
		term->line_dirty[i] = true;

	refresh(xw, term);
}

static void (*event_func[LASTEvent])(xwindow *xw, terminal *term, XEvent *ev) = {
	[KeyPress] = xkeypress,
	[ConfigureNotify] = xresize,
	[Expose] = xredraw,
};

int main()
{
	int size;
	fd_set fds;
	struct termios save_tm;
	struct timeval tv;
	u8 buf[BUFSIZE];
	XEvent ev;
	xwindow xw;
	terminal term;

	/* init */
	x_init(&xw);
	term_init(&term, xw.res);
	load_ctrl_func(ctrl_func, CTRL_CHARS);
	load_esc_func(esc_func, ESC_CHARS);
	load_csi_func(csi_func, ESC_CHARS);

	/* fork */
	eforkpty(&term.fd, term.lines, term.cols);

	signal(SIGCHLD, sigchld);
	setvbuf(stdout, NULL, _IONBF, 0);

	/* main loop */
	while (loop_flag) {
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
				if (DUMP)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				if (LAZYDRAW && size == BUFSIZE)
					continue;
				refresh(&xw, &term);
			}
		}
	}

	term_die(&term);
	x_die(&xw);

	return 0;
}
