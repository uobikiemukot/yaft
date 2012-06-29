/* See LICENSE for licence details. */
#include "common.h"
#include "util.h"
#include "framebuffer.h"
#include "load.h"
#include "terminal.h"
#include "function.h"
#include "parse.h"

static int loop_flag = 1;

void sigchld(int signal)
{
	loop_flag = 0;
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
	int size;
	pid_t pid;
	fd_set fds;
	struct termios save_tm;
	struct timeval tv;
	u8 buf[BUFSIZE];
	framebuffer fb;
	terminal term;

	/* init */
	fb_init(&fb);
	term_init(&term, &fb);
	load_ctrl_func(ctrl_func, CTRL_CHARS);
	load_esc_func(esc_func, ESC_CHARS);
	load_csi_func(csi_func, ESC_CHARS);

	/* fork */
	pid = eforkpty(&term.fd, term.lines, term.cols);
	if (pid == 0) {				/* child */
		putenv(term_name);
		eexecl(shell_cmd);
	}

	/* parent */
	signal(SIGCHLD, sigchld);
	set_rawmode(STDIN_FILENO, &save_tm);

	if (DUMP)
		setvbuf(stdout, NULL, _IONBF, 0);

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, term.fd);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0)
				ewrite(term.fd, buf, size);
		}

		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				if (DUMP)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				if (LAZYDRAW && size == BUFSIZE)
					continue;
				refresh(&fb, &term);
			}
		}
	}

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_tm);
	term_die(&term);
	fb_die(&fb);

	return 0;
}
