/* See LICENSE for licence details. */
void fatal(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

int eopen(char *file, int flag)
{
	int fd;

	if ((fd = open(file, flag)) == -1) {
		fprintf(stderr, "%s\n", file);
		fatal("open");
	}
	return fd;
}

void eclose(int fd)
{
	if (close(fd) == -1)
		fatal("close");
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
	if (fclose(fp) == -1)
		fatal("fclose");
}

void eioctl(int fd, int req, void *arg)
{
	if (ioctl(fd, req, arg) == -1)
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
	if (munmap(ptr, len) == -1)
		fatal("munmap");
}

void *emalloc(size_t size)
{
	void *p;
	if ((p = malloc(size)) == NULL)
		fatal("malloc");
	return p;
}

pid_t eforkpty(int *fd, int lines, int cols)
{
	pid_t pid;
	struct winsize size;

	size.ws_col = cols;
	size.ws_row = lines;
	size.ws_xpixel = size.ws_ypixel = 0;

	if ((pid = forkpty(fd, NULL, NULL, &size)) == -1)
		fatal("forkpty");
	return pid;
}

void eexecl(char *path)
{
	if (execl(path, path, NULL) == -1)
		fatal("execl");
}

void eselect(int max_fd, fd_set *fds, struct timeval *tv)
{
	if (select(max_fd, fds, NULL, NULL, tv) == -1)
		fatal("select");
}

void ewrite(int fd, u8 *buf, int size)
{
	if (write(fd, buf, size) == -1)
		fatal("write");
}

void swap(int *a, int *b)
{
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}
