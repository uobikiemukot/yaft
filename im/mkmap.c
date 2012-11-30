#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "perf.h"

enum {
	BUFSIZE = 64,
	KEYLEN = 4,
};

struct r2k_map_t {
	char key[BUFSIZE], hira[BUFSIZE], kata[BUFSIZE];
};

void fatal(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

FILE *efopen(char *path, char *mode)
{
	FILE *fp;

	if ((fp = fopen(path, mode)) == NULL)
		fatal("fopen");

	return fp;
}

int main(int argc, char *argv[])
{
	int num;
	unsigned int key;
	char buf[BUFSIZE];
	FILE *fp;
	struct r2k_map_t map;

	fp = (argc < 2) ? stdin: efopen(argv[1], "r");

	printf("const char *r2h_map[] = {\n");
	fprintf(stderr, "const char *r2k_map[] = {\n");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;
		
		memset(&map, 0, sizeof(struct r2k_map_t));
		num = sscanf(buf, "%s\t%s\t%s", map.key, map.hira, map.kata);
		if (num == 3) {
			key = hash(map.key, strlen(map.key));
			printf("[%u] = \"%s\",\n", key, map.hira);
			fprintf(stderr, "[%u] = \"%s\",\n", key, map.kata);
		}
	}

	printf("};\n");
	fprintf(stderr, "};\n");

	return EXIT_SUCCESS;
}
