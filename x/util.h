/* See LICENSE for licence details. */
void fatal(char *str)
{
	perror(str);
	//fprintf(stderr, "fatal error: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
}

int eopen(char *file, int flag)
{
	int fd;

	if ((fd = open(file, flag)) < 0) {
		fprintf(stderr, "%s\n", file);
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

FILE *efopen(char *file, char *mode)
{
	FILE *fp;

	if ((fp = fopen(file, mode)) == NULL) {
		fprintf(stderr, "%s\n", file);
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

u32 *emmap(int addr, size_t len, int prot, int flag, int fd, off_t offset)
{
	u32 *fp;

	if ((fp = (u32 *) mmap(0, len, prot, flag, fd, offset)) == MAP_FAILED)
		fatal("mmap");
	return fp;
}

void emunmap(u32 *ptr, size_t len)
{
	if (munmap(ptr, len) < 0)
		fatal("munmap");
}

void *emalloc(size_t size)
{
	void *p;
	if ((p = malloc(size)) == NULL)
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
	struct winsize size;

	size.ws_col = cols;
	size.ws_row = lines;
	size.ws_xpixel = size.ws_ypixel = 0;

	if (openpty(master, &slave, NULL, NULL, &size) < 0)
		fatal("openpty");

	pid = fork();
	if (pid < 0)
		fatal("fork");
	else if (pid == 0) { /* child */
		setsid();
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		if(ioctl(slave, TIOCSCTTY, NULL) < 0)
			fatal("ioctl TIOSCTTY");
		close(slave);
		close(*master);
		putenv(term_name);
		eexecl(shell_cmd);
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
			fatal("select");
	}
}

void ewrite(int fd, u8 *buf, int size)
{
	if (write(fd, buf, size) < 0)
		fatal("write");
}

void swap(int *a, int *b)
{
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}
