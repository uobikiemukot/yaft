#include "common.h"
#include "util.h"
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

int main()
{
	extern struct map_t *r2h[], *r2k[];
	extern struct entry_t ari[], nasi[];
	extern int ari_count, nasi_count;
	int mode, master, status;
	char buf[BUFSIZE];
	ssize_t size;
	pid_t pid;
	fd_set fds;

	struct timeval tv;
	struct winsize wsize;
	struct list_t *lp;

	atexit(die);
	setupterm(NULL, STDOUT_FILENO, &status);
	init(&save_tm);

	hash_init(r2h);
	hash_init(r2k);
	list_init(&lp);
	load_map(r2h, r2k);
	load_dict(ari, &ari_count, nasi, &nasi_count);

	mode = MODE_ASCII;

	pid = forkpty(&master, NULL, NULL, NULL);
	if (pid == 0)
		execl("/bin/bash", "bash", NULL);

	ioctl(STDIN_FILENO, TIOCGWINSZ, &wsize);
	ioctl(master, TIOCSWINSZ, &wsize);

	while (loop_flag) {
		check_fds(&fds, &tv, STDIN_FILENO, master);
		if (FD_ISSET(STDIN_FILENO, &fds)) {
			size = read(STDIN_FILENO, buf, BUFSIZE);
			if (size > 0) {
				list_push_back_n(&lp, buf, size);
				mode = parse[mode](master, &lp);
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
