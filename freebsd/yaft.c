/* See LICENSE for licence details. */
#include "common.h"
#include "../util.h"
#include "framebuffer.h"
#include "../font.h"
#include "../terminal.h"
#include "../function.h"
#include "../parse.h"

struct tty_state tty = {
	.visible = true,
	.redraw_flag = false,
	.loop_flag = true,
};

void handler(int signo)
{
	if (signo == SIGCHLD)
		tty.loop_flag = false;
	else if (signo == SIGUSR1) {
		if (tty.visible) {
			tty.visible = false;
			ioctl(STDIN_FILENO, VT_RELDISP, 1);
			//pause();
		}
		else {
			tty.visible = true;
			tty.redraw_flag = true;
			ioctl(STDIN_FILENO, VT_RELDISP, VT_ACKACQ);
			ioctl(STDIN_FILENO, VT_WAITACTIVE, tty.active);
		}
	}
}

void tty_init(struct tty_state *tty)
{
	struct vt_mode vtmode;
	struct sigaction sig1, sig2;

	tty->fd = eopen("/dev/tty", O_RDWR);

	memset(&sig1, 0, sizeof(struct sigaction));
	sig1.sa_handler = handler;
	sig1.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sig1, NULL);

	memset(&sig2, 0, sizeof(struct sigaction));
	sig2.sa_handler = handler;
	sig2.sa_flags = SA_RESTART;
	sigaction(SIGUSR1, &sig2, NULL);

	ioctl(STDIN_FILENO, VT_RELDISP, VT_ACKACQ);
	ioctl(STDIN_FILENO, VT_GETACTIVE, &tty->active);

	memset(&vtmode, 0, sizeof(struct vt_mode));
	vtmode.mode = VT_PROCESS;
	vtmode.waitv = 0;
	vtmode.relsig = SIGUSR1;
	vtmode.acqsig = SIGUSR1;
	vtmode.frsig = SIGUSR1;

	if (ioctl(STDIN_FILENO, VT_SETMODE, &vtmode) < 0)
		 fatal("ioctl: VT_SETMODE");
	if (ioctl(STDIN_FILENO, KDSETMODE, KD_GRAPHICS) < 0)
		 fatal("ioctl: KDSETMODE");
}

void tty_die(struct tty_state *tty) {
	struct vt_mode vtmode;
	struct sigaction sig1, sig2;

	memset(&sig1, 0, sizeof(struct sigaction));
	sig1.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sig1, NULL);

	memset(&sig2, 0, sizeof(struct sigaction));
	sig2.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sig2, NULL);

	ioctl(STDIN_FILENO, VT_RELDISP, 1);

	memset(&vtmode, 0, sizeof(struct vt_mode));
	vtmode.mode = VT_AUTO;
	vtmode.waitv = 0;
	vtmode.relsig = 0;
	vtmode.acqsig = 0;
	vtmode.frsig = 0;

	if (ioctl(STDIN_FILENO, VT_SETMODE, &vtmode) < 0)
		 fatal("ioctl: VT_SETMODE");
	if (ioctl(STDIN_FILENO, KDSETMODE, KD_TEXT) < 0)
		 fatal("ioctl: KDSETMODE");
	close(tty->fd);
}

void check_fds(fd_set *fds, struct timeval *tv, int stdin, int master)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
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

int main()
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct termios save_tm;
	struct framebuffer fb;
	struct terminal term;

	/* init */
	tty_init(&tty);
	fb_init(&fb);
	term_init(&term, fb.res);

	/* fork */
	eforkpty(&term.fd, term.lines, term.cols);
	set_rawmode(STDIN_FILENO, &save_tm);

	/* main loop */
	while (tty.loop_flag) {
		if (tty.redraw_flag) {
			if (fb.bpp == 1)
				ioctl(fb.fd, FBIOPUTCMAP, fb.cmap);
			redraw(&term);
			refresh(&fb, &term);
			tty.redraw_flag = false;
		}

		check_fds(&fds, &tv, STDIN_FILENO, term.fd);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0)
				ewrite(term.fd, buf, size);
		}
		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				if (DEBUG)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				if (size == BUFSIZE) /* lazy drawing */
					continue;
				refresh(&fb, &term);
			}
		}
	}
	fflush(stdout);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_tm);

	term_die(&term);
	fb_die(&fb);
	tty_die(&tty);
}
