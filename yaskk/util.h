void error(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

void fatal(char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(EXIT_FAILURE);
}

int eopen(const char *path, int flag)
{
	int fd;

	if ((fd = open(path, flag)) < 0) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("open");
	}
	return fd;
}

void eclose(int fd)
{
	if (close(fd) < 0)
		error("close");
}

FILE *efopen(const char *path, char *mode)
{
	FILE *fp;

	if ((fp = fopen(path, mode)) == NULL) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("fopen");
	}
	return fp;
}

void efclose(FILE *fp)
{
	if (fclose(fp) < 0)
		error("fclose");
}

void *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	uint32_t *fp;

	if ((fp = (uint32_t *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED)
		error("mmap");
	return fp;
}

void emunmap(void *ptr, size_t len)
{
	if (munmap(ptr, len) < 0)
		error("munmap");
}

void *emalloc(size_t size)
{
	void *p;

	if ((p = calloc(1, size)) == NULL)
		error("calloc");
	return p;
}

void eexecl(const char *path)
{
	if (execl(path, path, NULL) < 0)
		error("execl");
}

int eposix_openpt(int flags)
{
	int fd;

	if ((fd = posix_openpt(O_RDWR)) < 0)
		error("posix_openpt");
	return fd;	
}

void egrantpt(int fd)
{
	if (grantpt(fd) < 0)
		error("grantpt");
}

void eunlockpt(int fd)
{
	if (unlockpt(fd) < 0)
		error("unlockpt");
}

void eforkpty(int *master, const char *cmd, int line, int col)
{
	int slave;
	pid_t pid;

	*master = eposix_openpt(O_RDWR);
	egrantpt(*master);
	eunlockpt(*master);
	if (ioctl(*master, TIOCSWINSZ,
		&(struct winsize){.ws_row = line, .ws_col = col, .ws_xpixel = 0, .ws_ypixel = 0}) < 0)
		fatal("ioctl: TIOCSWINSZ");
	slave = eopen(ptsname(*master), O_RDWR);

	pid = fork();
	if (pid < 0)
		error("fork");
	else if (pid == 0) { /* child */
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		setsid();
		ioctl(slave, TIOCSCTTY, NULL);
		close(slave);
		close(*master);
		//setenv("TERM", term, 1);
		eexecl(cmd);
	}
	else /* parent */
		close(slave);
}

void eselect(int max_fd, fd_set *fds, struct timeval *tv)
{
	if (select(max_fd, fds, NULL, NULL, tv) < 0) {
		if (errno == EINTR)
			eselect(max_fd, fds, tv);
		else
			error("select");
	}
}

void ewrite(int fd, void *buf, int size)
{
	if (write(fd, buf, size) < 0)
		error("write");
}

void esigaction(int signo, struct sigaction *act, struct sigaction *oact)
{
	if (sigaction(signo, act, oact) < 0)
		error("sigaction");
}

void etcgetattr(int fd, struct termios *tm)
{
	if (tcgetattr(fd, tm) < 0)
		error("tcgetattr");
}

void etcsetattr(int fd, int action, struct termios *tm)
{
	if (tcsetattr(fd, action, tm) < 0)
		error("tcgetattr");
}
