/* See LICENSE for licence details. */
#include "common.h"
#include "util.h"
#include "framebuffer.h"
#include "terminal.h"
#include "function.h"
#include "parse.h"

struct tty_state tty = {
	.save_tm = NULL,
	.visible = true,
	.redraw_flag = false,
	.loop_flag = true,
};

void handler(int signo)
{
	sigset_t sigset;

	if (signo == SIGCHLD)
		tty.loop_flag = false;
	else if (signo == SIGUSR1) {
		if (tty.visible) {
			tty.visible = false;
			ioctl(STDIN_FILENO, VT_RELDISP, 1);
			sigfillset(&sigset);
			sigdelset(&sigset, SIGUSR1);
			sigsuspend(&sigset);
		}
		else {
			tty.visible = true;
			tty.redraw_flag = true;
			ioctl(STDIN_FILENO, VT_RELDISP, VT_ACKACQ);
		}
	}
}

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	etcgetattr(fd, save_tm);
	tm = *save_tm;
	tm.c_iflag = tm.c_oflag = RESET;
	tm.c_cflag &= ~CSIZE;
	tm.c_cflag |= CS8;
	tm.c_lflag &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN] = 1;  /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	etcsetattr(fd, TCSAFLUSH, &tm);
}

void tty_init()
{
	extern struct tty_state tty; /* global var */
	struct sigaction sigact;
	struct vt_mode vtm;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGUSR1, &sigact, NULL);

	vtm.mode = VT_PROCESS;
	vtm.waitv = 0;
	vtm.relsig = vtm.acqsig = vtm.frsig = SIGUSR1;
	if (ioctl(STDIN_FILENO, VT_SETMODE, &vtm))
		fatal("ioctl: VT_SETMODE failed (maybe here is not console)");
	if (ioctl(STDIN_FILENO, KDSETMODE, KD_GRAPHICS) < 0)
		fatal("ioctl: KDSETMODE failed (maybe here is not console)");

	tty.save_tm = (struct termios *) emalloc(sizeof(struct termios));
	set_rawmode(STDIN_FILENO, tty.save_tm);
	ewrite(STDIN_FILENO, "\033[?25l", 6); /* make cusor invisible */
}

void tty_die()
{
 	/* no error handling */
	extern struct tty_state tty; /* global var */
	struct sigaction sigact;
	struct vt_mode vtm;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);

	vtm.mode = VT_AUTO;
	vtm.waitv = 0;
	vtm.relsig = vtm.acqsig = vtm.frsig = 0;
	ioctl(STDIN_FILENO, VT_SETMODE, &vtm);
	ioctl(STDIN_FILENO, KDSETMODE, KD_TEXT);

	if (tty.save_tm != NULL)
		tcsetattr(STDIN_FILENO, TCSAFLUSH, tty.save_tm);
	fflush(stdout);
	ewrite(STDIN_FILENO, "\033[?25h", 6); /* make cursor visible */
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

int main()
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct framebuffer fb;
	struct terminal term;

	/* init */
	setlocale(LC_ALL, "");
	if (atexit(tty_die) != 0)
		fatal("atexit failed");

	tty_init();
	fb_init(&fb);
	term_init(&term, fb.res);

	/* fork and exec shell */
	eforkpty(&term.fd, term.lines, term.cols);

	/* main loop */
	while (tty.loop_flag) {
		if (tty.redraw_flag) {
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

	/* die */
	term_die(&term);
	fb_die(&fb);

	return EXIT_SUCCESS;
}
