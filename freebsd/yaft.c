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
	sigset_t sigset;

	if (signo == SIGCHLD)
		tty.loop_flag = false;
	else if (signo == SIGUSR1) {
		if (tty.visible) {
			tty.visible = false;
			ioctl(tty.fd, VT_RELDISP, 1);
			sigfillset(&sigset);
			sigdelset(&sigset, SIGUSR1);
			sigsuspend(&sigset);
		}
		else {
			tty.visible = true;
			tty.redraw_flag = true;
			ioctl(tty.fd, VT_RELDISP, VT_ACKACQ);
		}
	}
}

void tty_init(struct tty_state *tty)
{
	struct sigaction sigact;
	keyboard_repeat_t keyboard_repeat;

	tty->fd = eopen("/dev/tty", O_RDWR);

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGUSR1, &sigact, NULL);

	if (ioctl(tty->fd, VT_SETMODE, &(struct vt_mode){.mode = VT_PROCESS,
		.waitv = 0, .relsig = SIGUSR1, .acqsig = SIGUSR1, .frsig = SIGUSR1}) < 0)
		fatal("ioctl: VT_SETMODE");
	if (ioctl(tty->fd, KDSETMODE, KD_GRAPHICS) < 0)
		fatal("ioctl: KDSETMODE");

	if (ioctl(tty->fd, KDGETREPEAT, &keyboard_repeat) < 0)
		fatal("ioctl: KDSETREPEAT");
	tty->kb_delay = keyboard_repeat.kb_repeat[0];
	tty->kb_repeat = keyboard_repeat.kb_repeat[1];

	keyboard_repeat.kb_repeat[0] = KB_DELAY;
	keyboard_repeat.kb_repeat[1] = KB_REPEAT;
	if (ioctl(tty->fd, KDSETREPEAT, &keyboard_repeat) < 0)
		fatal("ioctl: KDSETREPEAT");
}

void tty_die(struct tty_state *tty)
{
	struct sigaction sigact;
	keyboard_repeat_t keyboard_repeat;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGUSR1, &sigact, NULL);

	if (ioctl(tty->fd, VT_SETMODE, &(struct vt_mode){.mode = VT_AUTO,
		.waitv = 0, .relsig = 0, .acqsig = 0, .frsig = 0}) < 0)
		fatal("ioctl: VT_SETMODE");
	if (ioctl(tty->fd, KDSETMODE, KD_PIXEL) < 0)
		fatal("ioctl: KDSETMODE");

	keyboard_repeat.kb_repeat[0] = tty->kb_delay;
	keyboard_repeat.kb_repeat[1] = tty->kb_repeat;
	if (ioctl(tty->fd, KDSETREPEAT, &keyboard_repeat) < 0)
		fatal("ioctl: KDSETREPEAT");

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
	fb_init(&fb);
	term_init(&term, fb.res);
	tty_init(&tty);

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

	tty_die(&tty);
	term_die(&term);
	fb_die(&fb);
}
