#include "common.h"
#include "misc.h"
#include "load.h"

struct map_buffer_t {
	char *key, *buf;
};

struct map2_t {
	struct map_buffer_t *map_buffers;
	int count;
};

void load_map2(FILE *fp, struct map2_t *mp)
{
	int num;
	char buf[BUFSIZE], key[KEYSIZE], hira[VALSIZE], kata[VALSIZE];

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0)
			continue;

		num = sscanf(buf, "%s\t%s\t%s", key, hira, kata);
		if (num != 3)
			continue;

		//fprintf(stderr, "key:%s hira:%s kata:%s\n", key, hira, kata);
		mp->map_buffers = (struct map_buffer_t *) realloc(mp->map_buffers, sizeof(struct map_buffer_t) * (mp->count + 1));
		mp->map_buffers[mp->count].key = (char *) emalloc(strlen(key) + 1);
		mp->map_buffers[mp->count].buf = (char *) emalloc(strlen(buf) + 1);
		strcpy(mp->map_buffers[mp->count].key, key);
		strcpy(mp->map_buffers[mp->count].buf, buf);
		mp->count++;
	}
	//fprintf(stderr, "map_count:%d\n", mp->count);
}

void swap(struct map_buffer_t *e1, struct map_buffer_t *e2)
{
	struct map_buffer_t tmp;

	tmp = *e1;
	*e1 = *e2;
	*e2 = tmp;
}

void merge(struct map_buffer_t *ep1, int size1, struct map_buffer_t *ep2, int size2)
{
	int i, j, count;
	struct map_buffer_t merged[size1 + size2];

	count = i = j = 0;
	while (i < size1 || j < size2) {
		if (j == size2)
			merged[count++] = ep1[i++];
		else if (i == size1)
			merged[count++] = ep2[j++];
		else if (strcmp(ep1[i].key, ep2[j].key) < 0)
			merged[count++] = ep1[i++];
		else
			merged[count++] = ep2[j++];
	}

	memcpy(ep1, merged, sizeof(struct entry_t) * (size1 + size2));
}

void merge_sort(struct map_buffer_t *map_buffers, int size)
{
	int halfsize;

	if (size <= 1)
		return;
	else if (size == 2) {
		if (strcmp(map_buffers[0].key, map_buffers[1].key) > 0)
			swap(&map_buffers[0], &map_buffers[1]);
	}
	else {
		halfsize = size / 2;
		merge_sort(map_buffers, halfsize);
		merge_sort(map_buffers + halfsize, size - halfsize);
		merge(map_buffers, halfsize, map_buffers + halfsize, size - halfsize);
	}
}

int main(int argc, char *argv[])
{
	int i;
	struct map2_t map;
	FILE *fp;

	if (argc < 2) {
		fprintf(stderr, "usage: ./sortmap MAP\n");
		exit(EXIT_FAILURE);
	}

	map.map_buffers = NULL;
	map.count = 0;
	fp = efopen(argv[1], "r");
	load_map2(fp, &map);

	merge_sort(map.map_buffers, map.count);

	for (i = 0; i < map.count; i++)
		printf("%s", map.map_buffers[i].buf);
}
