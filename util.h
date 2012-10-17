/* See LICENSE for licence details. */
void fatal(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

int eopen(char *path, int flag)
{
	int fd;

	if ((fd = open(path, flag)) < 0) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		fatal("open");
	}
	return fd;
}

void eclose(int fd)
{
	if (close(fd) < 0) {
		if (errno == EINTR)
			eclose(fd);
		else
			fatal("close");
	}
}

FILE *efopen(char *path, char *mode)
{
	FILE *fp;

	if ((fp = fopen(path, mode)) == NULL) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		fatal("fopen");
	}
	return fp;
}

void efclose(FILE *fp)
{
	if (fclose(fp) < 0)
		fatal("fclose");
}

void eioctl(int fd, int req, void *arg)
{
	if (ioctl(fd, req, arg) < 0)
		fatal("ioctl");
}

void *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	u32 *fp;

	if ((fp = (u32 *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED)
		fatal("mmap");
	return fp;
}

void emunmap(void *ptr, size_t len)
{
	if (munmap(ptr, len) < 0)
		fatal("munmap");
}

void *emalloc(size_t size)
{
	void *p;
	if ((p = calloc(1, size)) == NULL)
		fatal("malloc");
	return p;
}

void eexecl(char *path)
{
	if (execl(path, path, NULL) < 0)
		fatal("execl");
}

void eforkpty(int *master, int lines, int cols)
{
	int slave;
	pid_t pid;
	winsize size;

	size.ws_col = cols;
	size.ws_row = lines;
	size.ws_xpixel = size.ws_ypixel = 0;

	if (openpty(master, &slave, NULL, NULL, &size) < 0)
		fatal("openpty");

	pid = fork();
	if (pid < 0)
		fatal("fork");
	else if (pid == 0) { /* child */
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		setsid();
		eioctl(slave, TIOCSCTTY, NULL);
		close(slave);
		close(*master);
		putenv(term_name);
		eexecl(shell_cmd);
	}
	else /* parent */
		close(slave);
}

void eselect(int max_fd, fd_set *fds, timeval *tv)
{
	if (select(max_fd, fds, NULL, NULL, tv) < 0) {
		if (errno == EINTR)
			eselect(max_fd, fds, tv);
		else
			fatal("select");
	}
}

void ewrite(int fd, u8 *buf, int size)
{
	if (write(fd, buf, size) < 0)
		fatal("write");
}

/* non system call function */
u32 bit_reverse(u32 v, int bits)
{
	u32 r = v;
	int s = bits - 1;

	for (v >>= 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}

	return r <<= s;
}

void swap_color(color_pair *cp)
{
	int tmp;

	tmp = cp->fg;
	cp->fg = cp->bg;
	cp->bg = tmp;
}

void reset_parm(parm_t *pt)
{
	int i;

	pt->argc = 0;
	for (i = 0; i < ESC_PARAMS; i++)
		pt->argv[i] = NULL;
}

void parse_arg(u8 *buf, parm_t *pt, int delim, int (is_valid)(int c))
{
	int length;
	u8 *cp;

	length = strlen((char *) buf);
	cp = buf;

	while (cp < &buf[length - 1]) {
		if (*cp == delim)
			*cp = '\0';
		cp++;
	}

	cp = buf;
  start:
	if (pt->argc < ESC_PARAMS && is_valid(*cp)) {
		pt->argv[pt->argc] = (char *) cp;
		pt->argc++;
	}

	while (is_valid(*cp))
		cp++;

	while (!is_valid(*cp) && cp < &buf[length - 1])
		cp++;

	if (cp < &buf[length - 1])
		goto start;
}
