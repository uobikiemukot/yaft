/* See LICENSE for licence details. */
#include "yaft.h"
#include "conf.h"
#include "util.h"

#if defined(__linux__)
	#include "fb/linux.h"
#elif defined(__FreeBSD__)
	#include "fb/freebsd.h"
#elif defined(__NetBSD__)
	#include "fb/netbsd.h"
#elif defined(__OpenBSD__)
	#include "fb/openbsd.h"
#endif

#include "draw.h"
#include "terminal.h"
#include "function.h"
#include "osc.h"
#include "dcs.h"
#include "parse.h"

void usage()
{
	fprintf(stderr,
		"yaft (yet another framebuffer terminal)\n\n"
		"usage: just type 'yaft'\n\n"
		"command line option:\n"
		"\tnone (see below)\n\n"
		"compile time configuration:\n"
		"\trewrite conf.h before build\n\n"
		"enviroment variables:\n"
		"\tFRAMEBUFFER:\n"
		"\t\tuse specified framebuffer device (default:/dev/fb0)\n"
		"\t\tex) FRAMEBUFFER=/dev/fb1 yaft\n"
		"\tYAFT:\n"
		"\t\tuse wallpaper (need fbv or idump, see yaft_wall script)\n"
		"\t\tex) YAFT=wall yaft or YAFT=wallpaper yaft\n\n"
		"custom font:\n"
		"\tyou can use your favorite fonts by using mkfont_bdf (please rewrite makefile)\n"
		"\tusage: mkfont_bdf ALIAS BDF1 BDF2 BDF3... > glyph.h\n"
		"\t\tALIAS: glyph substitution rule file (see table/alias)\n"
		"\t\tBDF: monospace bdf files (must be the same size)\n\n"
		"see README for more information\n"
		);
}

void sig_handler(int signo)
{
	extern struct tty_state tty; /* global */
	sigset_t sigset;

	if (signo == SIGCHLD)
		tty.loop_flag = false;
	else if (signo == SIGUSR1) {
		if (tty.visible) { /* vt deactivated */
			ioctl(STDIN_FILENO, VT_RELDISP, 1);
			tty.visible = false;
			if (BACKGROUND_DRAW) /* update passive cursor */
				tty.redraw_flag = true;
			else { /* sleep until next vt switching */
				sigfillset(&sigset);
				sigdelset(&sigset, SIGUSR1);
				sigsuspend(&sigset);
			}
		}
		else { /* vt activated */
			ioctl(STDIN_FILENO, VT_RELDISP, VT_ACKACQ);
			tty.visible     = true;
			tty.redraw_flag = true;
		}
	}
}

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	etcgetattr(fd, save_tm);
	tm = *save_tm;
	tm.c_iflag = tm.c_oflag = 0;
	tm.c_cflag &= ~CSIZE;
	tm.c_cflag |= CS8;
	tm.c_lflag &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN]  = 1; /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	etcsetattr(fd, TCSAFLUSH, &tm);
}

void tty_init(struct termios *save_tm)
{
	struct sigaction sigact;
	struct vt_mode vtm;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);
	esigaction(SIGUSR1, &sigact, NULL);

	vtm.mode   = VT_PROCESS;
	vtm.waitv  = 0;
	vtm.relsig = vtm.acqsig = vtm.frsig = SIGUSR1;
	if (ioctl(STDIN_FILENO, VT_SETMODE, &vtm))
		fatal("ioctl: VT_SETMODE failed (maybe here is not console)");

	if (ioctl(STDIN_FILENO, KDSETMODE, KD_GRAPHICS))
		fatal("ioctl: KDSETMODE failed (maybe here is not console)");

	set_rawmode(STDIN_FILENO, save_tm);
	ewrite(STDIN_FILENO, "\033[?25l", 6); /* make cusor invisible */
}

void tty_die(struct termios *save_tm)
{
 	/* no error handling */
	struct sigaction sigact;
	struct vt_mode vtm;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);
	sigaction(SIGUSR1, &sigact, NULL);

	vtm.mode   = VT_AUTO;
	vtm.waitv  = 0;
	vtm.relsig = vtm.acqsig = vtm.frsig = 0;
	ioctl(STDIN_FILENO, VT_SETMODE, &vtm);
	ioctl(STDIN_FILENO, KDSETMODE, KD_TEXT);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, save_tm);
	fflush(stdout);
	ewrite(STDIN_FILENO, "\033[?25h", 6); /* make cursor visible */
}

void fork_and_exec(int *master, int lines, int cols)
{
	pid_t pid;
	struct winsize ws = {.ws_row = lines, .ws_col = cols,
		.ws_xpixel = 0, .ws_ypixel = 0};

	pid = eforkpty(master, NULL, NULL, &ws);

	if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		eexecvp(shell_cmd, (const char *[]){shell_cmd, NULL});
	}
}

void check_fds(fd_set *fds, struct timeval *tv, int input, int master)
{
	FD_ZERO(fds);
	FD_SET(input, fds);
	FD_SET(master, fds);
	tv->tv_sec  = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	eselect(master + 1, fds, NULL, NULL, tv);
}

int main(int argc, char *argv[])
{
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct framebuffer fb;
	struct terminal term;
	struct termios save_tm;

	/* show usage */
	if (argc > 1
		&& (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "-help") == 0)) {
		usage();
		return EXIT_SUCCESS;
	}

	/* init */
	setlocale(LC_ALL, "");
	fb_init(&fb, term.color_palette);
	term_init(&term, fb.width, fb.height);
	tty_init(&save_tm);

	/* fork and exec shell */
	fork_and_exec(&term.fd, term.lines, term.cols);

	/* main loop */
	while (tty.loop_flag) {
		if (tty.redraw_flag) { 
			/* after VT switching, need to restore cmap (in 8bpp mode) */
			cmap_update(fb.fd, fb.cmap);
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
				refresh(&fb, &term);
			}
		}
	}

	/* normal exit */
	tty_die(&save_tm);
	term_die(&term);
	fb_die(&fb);

	return EXIT_SUCCESS;
}
