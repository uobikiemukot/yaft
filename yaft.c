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

void check_fds(fd_set *fds, timeval *tv, int stdin, int master_fd)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master_fd, fds);
	tv->tv_sec = 0;
	tv->tv_usec = INTERVAL;
	eselect(fds, tv, master_fd + 1);
}

void set_rawmode(int fd, termios *save_tm, termios *tm)
{
	tcgetattr(fd, save_tm);
	*tm = *save_tm;
	cfmakeraw(tm);
	tm->c_cc[VMIN] = 1; /* min data size (byte) */
	tm->c_cc[VTIME] = 0; /* time out */
	tcsetattr(fd, TCSAFLUSH, tm);
}

void load_func()
{
	load_ctrl_func(ctrl_func, CTRL_CHARS);
	load_esc_func(esc_func, ESC_CHARS);
	load_csi_func(csi_func, ESC_CHARS);
	load_osc_func(osc_func, ESC_CHARS);
}

int main()
{
	int size;
	fd_set fds;
	termios save_tm, tm;
	timeval tv;
	pid_t pid;
	FILE *logfile;

	u8 buf[BUFSIZE];
	framebuffer fb;
	terminal term;

	/* init */
	fb_init(&fb);
	term_init(&term);
	load_func();

	/* fork */
	pid = eforkpty(&term.fd, term.lines, term.cols);
	if (pid == 0) { /* child */
		setenv("TERM", term_name, true);
		eexecl(shell_cmd);
	}

	/* parent */
	signal(SIGCHLD, sigchld);
	set_rawmode(STDIN_FILENO, &save_tm, &tm);

	if (DUMP)
		setvbuf(stdout, NULL, _IONBF, 0);

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, term.fd);

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0)
				write(term.fd, buf, size);
		}

		if (FD_ISSET(term.fd, &fds)) {
			size = read(term.fd, buf, BUFSIZE);
			if (size > 0) {
				parse(&term, buf, size);
				if (DUMP)
					write(STDOUT_FILENO, buf, size);
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
