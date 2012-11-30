#include "util.h"
#include "hash.h"

enum {
	BUFSIZE = 1024,
};

const char *map_file = "convert.map";

void load_map(struct map_t *r2h[], struct map_t *r2k[])
{
	int num;
	char buf[BUFSIZE], key[BUFSIZE];
	char hira[BUFSIZE], kata[BUFSIZE];
	FILE *fp;

	fp = efopen(map_file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		memset(key, '\0', BUFSIZE);
		memset(hira, '\0', BUFSIZE);
		memset(kata, '\0', BUFSIZE);

		num = sscanf(buf, "%s\t%s\t%s", key, hira, kata);
		if (num == 3) {
			hash_lookup(r2h, key, hira, true);
			hash_lookup(r2k, key, kata, true);
		}
	}

	efclose(fp);
}

int main()
{
	struct map_t *r2h[NHASH], *r2k[NHASH], *mp;

	hash_init(r2h);
	hash_init(r2k);
	load_map(r2h, r2k);

	if ((mp = hash_lookup(r2k, "kaki", NULL, false)) != NULL) {
		printf("%s\n", mp->val);
	}

	printf("size:%d\n", strlen("昔"));
}
