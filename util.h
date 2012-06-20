int eopen(char *file, int flag)
{
	int fd;

	if ((fd = open(file, flag)) == -1) {
		fprintf(stderr, "%s\n", file);
		perror("open");
		exit(EXIT_FAILURE);
	}

	return fd;
}

FILE *efopen(char *file, char *mode)
{
	FILE *fp;

	if ((fp = fopen(file, mode)) == NULL) {
		fprintf(stderr, "%s\n", file);
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	return fp;
}

void eclose(int fd)
{
	if (close(fd) == -1) {
		perror("close");
		exit(EXIT_FAILURE);
	}
}

void eioctl(int fd, int req, void *arg)
{
	if (ioctl(fd, req, arg) == -1) {
		perror("ioctl");
		exit(EXIT_FAILURE);
	}
}

u32 *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	u32 *fp;

	if ((fp = (u32 *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	return fp;
}

void emunmap(u32 *fp, size_t len)
{
	if (munmap(fp, len) == -1) {
		perror("munmap");
		exit(EXIT_FAILURE);
	}
}

void *emalloc(size_t size)
{
	void *p;
	if ((p = malloc(size)) == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	return p;
}

pid_t eforkpty(int *fd, int lines, int cols)
{
	pid_t pid;
	winsize size;

	size.ws_col = cols;
	size.ws_row = lines;

	/* forkpty(int *amaster, char *name,
		const struct termios *termp, const struct winsize *winp) */
	if ((pid = forkpty(fd, NULL, NULL, &size)) == -1) {
		perror("forkpty");
		exit(EXIT_FAILURE);
	}

	return pid;
}

void eexecl(char *cmd)
{
	if (execl(cmd, cmd, NULL) == -1) {
		perror("execl");
		exit(EXIT_FAILURE);
	}
}

void eselect(fd_set *fds, timeval *tv, int max_fd)
{
	if (select(max_fd, fds, NULL, NULL, tv) == -1) {
		perror("select");
		exit(EXIT_FAILURE);
	}
}

void ewrite(int fd, char *buf, int size)
{
	int ret;

	if ((ret = write(fd, buf, size)) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}
}

int toaddr(terminal *term, int y, int x)
{
	return x + y * term->cols;
}
