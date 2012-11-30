/* See LICENSE for licence details. */
void sigchld(int signal)
{
	loop_flag = 0;
}

void check_fds(fd_set * fds, timeval * tv, int stdin, int master_fd)
{
	FD_ZERO(fds);
	FD_SET(stdin, fds);
	FD_SET(master_fd, fds);
	tv->tv_sec = 0;
	tv->tv_usec = INTERVAL;
	eselect(fds, tv, master_fd + 1);
}

void set_rawmode(int fd, termios * save_tm, termios * tm)
{
	tcgetattr(fd, save_tm);
	*tm = *save_tm;
	tm->c_iflag = tm->c_oflag = RESET;
	tm->c_cflag &= ~CSIZE;
	tm->c_cflag |= CS8;
	tm->c_lflag &= ~(ECHO | ISIG | ICANON);
	tm->c_cc[VMIN] = 1;			/* min data size (byte) */
	tm->c_cc[VTIME] = 0;		/* time out */
	tcsetattr(fd, TCSAFLUSH, tm);
}

int main()
{
}
