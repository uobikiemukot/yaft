/* See LICENSE for licence details. */
/* error functions */
enum loglevel_t {
	DEBUG = 0,
	WARN,
	ERROR,
	FATAL,
};

void logging(enum loglevel_t loglevel, char *format, ...)
{
	va_list arg;
	static const char *loglevel2str[] = {
		[DEBUG] = "DEBUG",
		[WARN]  = "WARN",
		[ERROR] = "ERROR",
		[FATAL] = "FATAL",
	};

	/* debug message is available on verbose mode */
	if ((loglevel == DEBUG) && (VERBOSE == false))
		return;

	fprintf(stderr, ">>%s<<\t", loglevel2str[loglevel]);

	va_start(arg, format);
	vfprintf(stderr, format, arg);
	va_end(arg);
}

/* wrapper of C functions */
int eopen(const char *path, int flag)
{
	int fd;
	errno = 0;

	if ((fd = open(path, flag)) < 0) {
		logging(ERROR, "couldn't open \"%s\"\n", path);
		logging(ERROR, "open: %s\n", strerror(errno));
	}
	return fd;
}

int eclose(int fd)
{
	int ret;
	errno = 0;

	if ((ret = close(fd)) < 0)
		logging(ERROR, "close: %s\n", strerror(errno));

	return ret;
}

FILE *efopen(const char *path, char *mode)
{
	FILE *fp;
	errno = 0;

	if ((fp = fopen(path, mode)) == NULL) {
		logging(ERROR, "couldn't open \"%s\"\n", path);
		logging(ERROR, "fopen: %s\n", strerror(errno));
	}
	return fp;
}

int efclose(FILE *fp)
{
	int ret;
	errno = 0;

	if ((ret = fclose(fp)) < 0)
		logging(ERROR, "fclose: %s\n", strerror(errno));

	return ret;
}

void *emmap(void *addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	void *fp;
	errno = 0;

	if ((fp = mmap(addr, len, prot, flag, fd, offset)) == MAP_FAILED)
		logging(ERROR, "mmap: %s\n", strerror(errno));

	return fp;
}

int emunmap(void *ptr, size_t len)
{
	int ret;
	errno = 0;

	if ((ret = munmap(ptr, len)) < 0)
		logging(ERROR, "munmap: %s\n", strerror(errno));

	return ret;
}

void *ecalloc(size_t nmemb, size_t size)
{
	void *ptr;
	errno = 0;

	if ((ptr = calloc(nmemb, size)) == NULL)
		logging(ERROR, "calloc: %s\n", strerror(errno));

	return ptr;
}

void *erealloc(void *ptr, size_t size)
{
	void *new;
	errno = 0;

	if ((new = realloc(ptr, size)) == NULL)
		logging(ERROR, "realloc: %s\n", strerror(errno));

	return new;
}

int eselect(int maxfd, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *tv)
{
	int ret;
	errno = 0;

	if ((ret = select(maxfd, readfds, writefds, errorfds, tv)) < 0) {
		if (errno == EINTR)
			return eselect(maxfd, readfds, writefds, errorfds, tv);
		else
			logging(ERROR, "select: %s\n", strerror(errno));
	}
	return ret;
}

ssize_t ewrite(int fd, const void *buf, size_t size)
{
	ssize_t ret;
	errno = 0;

	if ((ret = write(fd, buf, size)) < 0) {
		if (errno == EINTR) {
			logging(ERROR, "write: EINTR occurred\n");
			return ewrite(fd, buf, size);
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			logging(ERROR, "write: EAGAIN or EWOULDBLOCK occurred, sleep %d usec\n", SLEEP_TIME);
			usleep(SLEEP_TIME);
			return ewrite(fd, buf, size);
		} else {
			logging(ERROR, "write: %s\n", strerror(errno));
			return ret;
		}
	} else if (ret < (ssize_t) size) {
		logging(ERROR, "data size:%zu write size:%zd\n", size, ret);
		return ewrite(fd, (char *) buf + ret, size - ret);
	}
	return ret;
}

int esigaction(int signo, struct sigaction *act, struct sigaction *oact)
{
	int ret;
	errno = 0;

	if ((ret = sigaction(signo, act, oact)) < 0)
		logging(ERROR, "sigaction: %s\n", strerror(errno));

	return ret;
}

int etcgetattr(int fd, struct termios *tm)
{
	int ret;
	errno = 0;

	if ((ret = tcgetattr(fd, tm)) < 0)
		logging(ERROR, "tcgetattr: %s\n", strerror(errno));

	return ret;
}

int etcsetattr(int fd, int action, const struct termios *tm)
{
	int ret;
	errno = 0;

	if ((ret = tcsetattr(fd, action, tm)) < 0)
		logging(ERROR, "tcgetattr: %s\n", strerror(errno));

	return ret;
}

int eopenpty(int *amaster, int *aslave, char *aname,
	const struct termios *termp, const struct winsize *winsize)
{
	int master;
	char *name = NULL;
	errno = 0;

	if ((master = posix_openpt(O_RDWR | O_NOCTTY)) < 0
		|| grantpt(master) < 0
		|| unlockpt(master) < 0
		|| (name = ptsname(master)) == NULL) {
		logging(ERROR, "openpty: %s\n", strerror(errno));
		return -1;
	}
	*amaster = master;
	*aslave  = eopen(name, O_RDWR | O_NOCTTY);

	if (aname)
		/* XXX: we don't use the slave's name, do nothing */
		(void) aname;
		//strncpy(aname, name, _POSIX_TTY_NAME_MAX - 1);
		//snprintf(aname, _POSIX_TTY_NAME_MAX, "%s", name);
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
	pid   = fork();
	if (pid < 0) {
		logging(ERROR, "fork: %s\n", strerror(errno));
		return pid;
	} else if (pid == 0) { /* child */
		close(master);
		setsid();

		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);

		/* XXX: this ioctl may fail in Mac OS X
			ref http://www.opensource.apple.com/source/Libc/Libc-825.25/util/pty.c?txt */
		if (ioctl(slave, TIOCSCTTY, NULL))
			logging(WARN, "ioctl: TIOCSCTTY faild\n");
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
		logging(ERROR, "setenv: %s\n", strerror(errno));

	return ret;
}

int eexecvp(const char *file, const char *argv[])
{
	int ret;
	errno = 0;

	if ((ret = execvp(file, (char * const *) argv)) < 0)
		logging(ERROR, "execvp: %s\n", strerror(errno));

	return ret;
}

int eexecl(const char *path)
{
	int ret;
	errno = 0;

	/* XXX: assume only one argument is given */
	if ((ret = execl(path, path, NULL)) < 0)
		logging(ERROR, "execl: %s\n", strerror(errno));

	return ret;
}

long estrtol(const char *nptr, char **endptr, int base)
{
	long int ret;
	errno = 0;

	ret = strtol(nptr, endptr, base);
	if (ret == LONG_MIN || ret == LONG_MAX) {
		logging(ERROR, "strtol: %s\n", strerror(errno));
		return 0;
	}

	return ret;
}

/* parse_arg functions */
void reset_parm(struct parm_t *pt)
{
	pt->argc = 0;
	for (int i = 0; i < MAX_ARGS; i++)
		pt->argv[i] = NULL;
}

void add_parm(struct parm_t *pt, char *cp)
{
	if (pt->argc >= MAX_ARGS)
		return;

	logging(DEBUG, "argv[%d]: %s\n", pt->argc, (cp == NULL) ? "NULL": cp);

	pt->argv[pt->argc] = cp;
	pt->argc++;
}

void parse_arg(char *buf, struct parm_t *pt, int delim, int (is_valid)(int c))
{
	/*
		v..........v d           v.....v d v.....v ... d
		(valid char) (delimiter)
		argv[0]                  argv[1]   argv[2] ...   argv[argc - 1]
	*/
	size_t length;
	char *cp, *vp;

	if (buf == NULL)
		return;

	length = strlen(buf);
	logging(DEBUG, "parse_arg() length:%u\n", (unsigned) length);

	vp = NULL;
	for (size_t i = 0; i < length; i++) {
		cp = buf + i;

		if (vp == NULL && is_valid(*cp))
			vp = cp;

		if (*cp == delim) {
			*cp = '\0';
			add_parm(pt, vp);
			vp = NULL;
		}

		if (i == (length - 1) && (vp != NULL || *cp == '\0'))
			add_parm(pt, vp);
	}

	logging(DEBUG, "argc:%d\n", pt->argc);
}

/* other functions */
int my_ceil(int val, int div)
{
	if (div == 0)
		return 0;
	else
		return (val + div - 1) / div;
}

int dec2num(char *str)
{
	if (str == NULL)
		return 0;

	return estrtol(str, NULL, 10);
}

int hex2num(char *str)
{
	if (str == NULL)
		return 0;

	return estrtol(str, NULL, 16);
}

int sum(struct parm_t *parm)
{
	int sum = 0;

	for (int i = 0; i < parm->argc; i++)
		sum += dec2num(parm->argv[i]);

	return sum;
}
