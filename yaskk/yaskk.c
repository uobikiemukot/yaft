#include "common.h"
#include "util.h"
#include "misc.h"
#include "list.h"
#include "hash.h"
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
	extern struct hash_t *rom2hira[NHASH], *rom2kata[NHASH];
	extern struct table_t okuri_ari, okuri_nasi;
	int mode, master, wrote;
	char buf[BUFSIZE];
	const char *cmd;
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct winsize wsize;
	struct list_t *list;

	/* init */
	atexit(die);
	init(&save_tm);

	list_init(&list);
	hash_init(rom2hira);
	hash_init(rom2kata);

	load_map(rom2hira, rom2kata);
	load_dict(&okuri_ari, &okuri_nasi);

	mode = MODE_ASCII;
	wrote = 0;

	/* fork */
	ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
	cmd = (argc < 2) ? exec_cmd: argv[1];
	eforkpty(&master, cmd, wsize.ws_row, wsize.ws_col);

	/* main loop */
	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, master);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0) {
				list_push_back_n(&list, buf, size);
				mode = parse[mode](master, &list, &wrote);
			}
		}
		if (FD_ISSET(master, &fds)) {
			size = read(master, buf, BUFSIZE);
			if (size > 0)
				write(STDOUT_FILENO, buf, size);
		}
	}

	return EXIT_SUCCESS;
}
