#include "common.h"
#include "misc.h"
#include "load.h"

void load(FILE *fp, struct table_t *okuri_ari, struct table_t *okuri_nasi)
{
	char buf[BUFSIZE], key[BUFSIZE], entry[BUFSIZE];
	int  mode = RESET, num;
	long offset;
	struct table_t *tp;

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strstr(buf, ";; okuri-ari entries") != NULL) {
			mode = READ_OKURIARI;
			continue;
		}
		else if (strstr(buf, ";; okuri-nasi entries") != NULL) {
			mode = READ_OKURINASI;
			continue;
		}

		if (mode == RESET)
			continue;

		num = sscanf(buf, "%s %s", key, entry);
		if (num != 2)
			continue;

		offset = ftell(fp) - strlen(buf);
		//printf("key:%s entry:%s offset:%ld\n", key, entry, offset);
		tp = (mode == READ_OKURIARI) ? okuri_ari: okuri_nasi;

		tp->entries = (struct entry_t *) realloc(tp->entries, sizeof(struct entry_t) * (tp->count + 1));
		tp->entries[tp->count].key = (char *) emalloc(strlen(key) + 1);

		strcpy(tp->entries[tp->count].key, key);
		tp->entries[tp->count].offset = offset;
		tp->count++;
	}
}

void swap(struct entry_t *e1, struct entry_t *e2)
{
	struct entry_t tmp;

	tmp = *e1;
	*e1 = *e2;
	*e2 = tmp;
}

void merge(struct entry_t *ep1, int size1, struct entry_t *ep2, int size2)
{
	int i, j, count;
	struct entry_t merged[size1 + size2];

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

void merge_sort(struct entry_t *entries, int size)
{
	int halfsize;

	if (size <= 1)
		return;
	else if (size == 2) {
		if (strcmp(entries[0].key, entries[1].key) > 0)
			swap(&entries[0], &entries[1]);
	}
	else {
		halfsize = size / 2;
		merge_sort(entries, halfsize);
		merge_sort(entries + halfsize, size - halfsize);
		merge(entries, halfsize, entries + halfsize, size - halfsize);
	}
}

void print_entry(FILE *fp, struct entry_t *ep)
{
	char buf[BUFSIZE];

	fseek(fp, ep->offset, SEEK_SET);
	fgets(buf, BUFSIZE, fp);
	printf("%s", buf);
}

int main(int argc, char *argv[])
{
	extern struct table_t okuri_ari, okuri_nasi;
	int i;
	FILE *fp;

	if (argc < 2) {
		fprintf(stderr, "usage: ./sort DICT\n");
		exit(EXIT_FAILURE);
	}

	fp = efopen(argv[1], "r");
	dict_init(&okuri_ari, &okuri_nasi);
	load(fp, &okuri_ari, &okuri_nasi);

	merge_sort(okuri_ari.entries, okuri_ari.count);
	merge_sort(okuri_nasi.entries, okuri_nasi.count);

	printf(";; okuri-ari entries.\n");
	for (i = 0; i < okuri_ari.count; i++)
		print_entry(fp, &okuri_ari.entries[i]);

	printf(";; okuri-nasi entries.\n");
	for (i = 0; i < okuri_nasi.count; i++)
		print_entry(fp, &okuri_nasi.entries[i]);
}
