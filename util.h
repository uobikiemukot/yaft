/* See LICENSE for licence details. */
/* error functions */
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

void warn(char *str)
{
	fprintf(stderr, "%s\n", str);
}

/* wrapper of C functions */
int eopen(const char *path, int flag)
{
	int fd;
	errno = 0;

	if ((fd = open(path, flag)) < 0) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("open");
	}

	return fd;
}

void eclose(int fd)
{
	errno = 0;

	if (close(fd) < 0)
		error("close");
}

FILE *efopen(char *path, char *mode)
{
	FILE *fp;
	errno = 0;

	if ((fp = fopen(path, mode)) == NULL) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		error("fopen");
	}

	return fp;
}

void efclose(FILE *fp)
{
	errno = 0;

	if (fclose(fp) < 0)
		error("fclose");
}

void *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	uint32_t *fp;
	errno = 0;

	if ((fp = (uint32_t *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED)
		error("mmap");

	return fp;
}

void emunmap(void *ptr, size_t len)
{
	errno = 0;

	if (munmap(ptr, len) < 0)
		error("munmap");
}

void *ecalloc(size_t size)
{
	void *p;
	errno = 0;

	if ((p = calloc(1, size)) == NULL)
		error("calloc");

	return p;
}

void eselect(int max_fd, fd_set *fds, struct timeval *tv)
{
	errno = 0;

	if (select(max_fd, fds, NULL, NULL, tv) < 0) {
		if (errno == EINTR)
			eselect(max_fd, fds, tv);
		else
			error("select");
	}
}

void ewrite(int fd, void *buf, int size)
{
	int ret;
	errno = 0;

	if ((ret = write(fd, buf, size)) < 0)
		error("write");
	else if (ret < size)
		ewrite(fd, (char *) buf + ret, size - ret);
}

void esigaction(int signo, struct sigaction *act, struct sigaction *oact)
{
	errno = 0;

	if (sigaction(signo, act, oact) < 0)
		error("sigaction");
}

void etcgetattr(int fd, struct termios *tm)
{
	errno = 0;

	if (tcgetattr(fd, tm) < 0)
		error("tcgetattr");
}

void etcsetattr(int fd, int action, const struct termios *tm)
{
	errno = 0;

	if (tcsetattr(fd, action, tm) < 0)
		error("tcgetattr");
}


int eopenpty(int *amaster, int *aslave, char *aname,
	const struct termios *termp, const struct winsize *winsize)
{
	int master;
	char *name = NULL;
	errno = 0;

	if ((master = posix_openpt(O_RDWR | O_NOCTTY)) < 0
		|| grantpt(master) < 0 || unlockpt(master) < 0
		|| (name = ptsname(master)) == NULL)
		error("openpty");

	*amaster = master;
	*aslave = eopen(name,  O_RDWR | O_NOCTTY);

	if (aname)
		aname = NULL; /* we don't use this value */
	if (termp)
		etcsetattr(*aslave, TCSAFLUSH, termp);
	if (winsize)
		ioctl(*aslave, TIOCSWINSZ, winsize);

	return 0;
}

pid_t eforkpty(int *amaster, char *name,
	const struct termios *termp, const struct winsize *winsize)
{
	int master, slave;
	pid_t pid;

	if (eopenpty(&master, &slave, name, termp, winsize) < 0)
		return -1;

	errno = 0;
	pid = fork();

	if (pid < 0)
		error("fork");
	else if (pid == 0) { /* child */
		close(master);

		setsid();
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		/* this ioctl may fail in Mac OS X: so we don't call exit()
			ref http://www.opensource.apple.com/source/Libc/Libc-825.25/util/pty.c?txt */
		ioctl(slave, TIOCSCTTY, NULL);
		close(slave);

		return 0;
	}
	/* parent */
	close(slave);
	*amaster = master;

	return pid;
}

int esetenv(const char *name, const char *value, int overwrite)
{
	int ret;
	errno = 0;

	if ((ret = setenv(name, value, overwrite)) < 0)
		error("setenv");

	return ret;
}

int eexecvp(const char *file, const char *argv[])
{
	int ret;
	errno = 0;

	if ((ret = execvp(file, (char * const *) argv)) < 0)
		error("execvp");

	return ret;
}

/* parse_arg functions */
void reset_parm(struct parm_t *pt)
{
	int i;

	pt->argc = 0;
	for (i = 0; i < ESC_PARAMS; i++)
		pt->argv[i] = NULL;
}

void parse_arg(char *buf, struct parm_t *pt, int delim, int (is_valid)(int c))
{
	int length;
	char *cp;

	length = strlen(buf);
	cp = buf;

	while (cp < &buf[length]) {
		if (*cp == delim)
			*cp = '\0';
		cp++;
	}

	cp = buf;
	while (cp < &buf[length]) {
		if (pt->argc < ESC_PARAMS && is_valid(*cp)) {
			pt->argv[pt->argc] = cp;
			pt->argc++;
		}

		while (is_valid(*cp))
			cp++;

		while (!is_valid(*cp) && cp <= &buf[length - 1])
			cp++;
	}
}
