#include "common.h"
#include "misc.h"
#include "linebuf.h"
#include "parse.h"
#include "load.h"

bool loop_flag = true;
struct termios save_tm;

void handler(int signo)
{
	extern bool loop_flag;

	if (signo == SIGCHLD)
		loop_flag = false;
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
	tm.c_cc[VMIN] = 1;  /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	tcsetattr(fd, TCSAFLUSH, &tm);
}

void init()
{
	extern struct termios save_tm;
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = handler;
	sigact.sa_flags = SA_RESTART;
	sigaction(SIGCHLD, &sigact, NULL);

	set_rawmode(STDIN_FILENO, &save_tm);
}

void die()
{
	extern struct termios save_tm;
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_tm);
	fflush(stdout);
}

void check_fds(fd_set *fds, struct timeval *tv, int stdin, int master)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master, fds);
	tv->tv_sec = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	select(master + 1, fds, NULL, NULL, tv);
}

int main(int argc, char *argv[])
{
	extern struct map_t rom2kana;
	extern struct table_t okuri_ari, okuri_nasi;
	struct linebuf_t linebuf;
	char buf[BUFSIZE];
	const char *cmd;
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct winsize wsize;

	/* init */
	atexit(die);
	init(&save_tm);

	map_init(&rom2kana);
	dict_init(&okuri_ari, &okuri_nasi);
	linebuf_init(&linebuf);

	load_map(&rom2kana);
	load_dict(&okuri_ari, &okuri_nasi);

	/* fork */
	ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
	cmd = (argc < 2) ? exec_cmd: argv[1];
	eforkpty(&linebuf.fd, cmd, &wsize, &save_tm);

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, linebuf.fd);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0) {
				parse(&linebuf, buf, size);
			}
		}
		if (FD_ISSET(linebuf.fd, &fds)) {
			size = read(linebuf.fd, buf, BUFSIZE);
			if (size > 0)
				write(STDOUT_FILENO, buf, size);
		}
	}

	return EXIT_SUCCESS;
}
