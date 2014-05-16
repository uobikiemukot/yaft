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

/* wrapper of C functions */
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

FILE *efopen(char *path, char *mode)
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

void *ecalloc(size_t size)
{
	void *p;

	if ((p = calloc(1, size)) == NULL)
		error("calloc");
	return p;
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
	int ret;

	if ((ret = write(fd, buf, size)) < 0)
		error("write");
	else if (ret < size)
		ewrite(fd, (char *) buf + ret, size - ret);
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
