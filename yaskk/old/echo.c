/* See LICENSE for licence details. */
#include <stdio.h>
#include <unistd.h>

enum {
	BUFSIZE = 1024,
};

int main()
{
	int size;
	unsigned char buf[BUFSIZE];

	while (1) {
		size = read(STDIN_FILENO, buf, BUFSIZE);

		if (size > 0)
			write(STDOUT_FILENO, buf, size);
	}
}
