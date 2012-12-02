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

void check_fds(fd_set *fds, timeval *tv, int stdin, int master_fd, int filter_in)
{
	int fd_max;

	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master_fd, fds);

	if (filter_in != -1) {
		FD_SET(filter_in, fds);
		fd_max = filter_in;
	}
	else
		fd_max = master_fd;

	tv->tv_sec = 0;
	tv->tv_usec = INTERVAL;
	eselect(fds, tv, fd_max + 1);
}

void set_rawmode(int fd, termios *save_tm)
{
	termios tm;

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

void run_filter(int fd_p2c[2], int fd_c2p[2])
{
	termios tm;
	pid_t pid;

	pipe(fd_p2c);
	pipe(fd_c2p);

	if ((pid = fork()) == 0) {	/* child */
		close(fd_p2c[1]);
		close(fd_c2p[0]);
		dup2(fd_p2c[0], STDIN_FILENO);
		dup2(fd_c2p[1], STDOUT_FILENO);
		dup2(fd_c2p[1], STDERR_FILENO);
		eexecl(filter_cmd);
	}

	/* parent */
	close(fd_p2c[0]);
	close(fd_c2p[1]);
	set_rawmode(fd_c2p[0], &tm);
}

int main()
{
	int size, fd_p2c[2], fd_c2p[2];
	fd_set fds;
	termios stdin_tm;
	timeval tv;
	pid_t pid;

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
	set_rawmode(STDIN_FILENO, &stdin_tm);

	/* input method */
	if (filter_cmd != NULL)
		run_filter(fd_p2c, fd_c2p);
	else
		fd_c2p[0] = -1;

	if (DUMP)
		setvbuf(stdout, NULL, _IONBF, 0);

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, term.fd, fd_c2p[0]);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0) {
				if (filter_cmd != NULL)
					ewrite(fd_p2c[1], buf, size);
				else
					ewrite(term.fd, buf, size);
			}
		}

		if (filter_cmd != NULL && FD_ISSET(fd_c2p[0], &fds)) {
			size = read(fd_c2p[0], buf, BUFSIZE);
			if (size > 0)
				ewrite(term.fd, buf, size);
		}

		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				parse(&term, buf, size);
				if (DUMP)
					ewrite(STDOUT_FILENO, buf, size);
				if (LAZYDRAW && size == BUFSIZE)
					continue;
				refresh(&fb, &term);
			}
		}
	}

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &stdin_tm);
	term_die(&term);
	fb_die(&fb);

	return 0;
}
