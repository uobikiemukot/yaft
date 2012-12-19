enum {
	READ_OKURIARI = 1,
	READ_OKURINASI,
};

void load_map(struct map_t *mp)
{
	int num;
	char buf[BUFSIZE], key[KEYSIZE], hira[VALSIZE], kata[VALSIZE];
	FILE *fp;

	fp = efopen(map_file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0)
			continue;

		num = sscanf(buf, "%[^\t]\t%s\t%s", key, hira, kata);
		if (num != 3)
			continue;

		//fprintf(stderr, "key:%s hira:%s kata:%s\n", key, hira, kata);
		mp->triplets = (struct triplet_t *) realloc(mp->triplets, sizeof(struct triplet_t) * (mp->count + 1));
		mp->triplets[mp->count].key = (char *) emalloc(strlen(key) + 1);
		mp->triplets[mp->count].hira = (char *) emalloc(strlen(hira) + 1);
		mp->triplets[mp->count].kata = (char *) emalloc(strlen(kata) + 1);
		memcpy(mp->triplets[mp->count].key, key, strlen(key));
		memcpy(mp->triplets[mp->count].hira, hira, strlen(hira));
		memcpy(mp->triplets[mp->count].kata, kata, strlen(kata));
		mp->count++;
	}
	//fprintf(stderr, "map_count:%d\n", mp->count);
}

void load_dict(struct table_t *okuri_ari, struct table_t *okuri_nasi)
{
	char buf[BUFSIZE], key[BUFSIZE], entry[BUFSIZE];
	int  mode = RESET, num;
	long offset;
	extern FILE *dict_fp;
	struct table_t *tp;

	dict_fp = efopen(dict_file, "r");

	while (fgets(buf, BUFSIZE, dict_fp) != NULL) {
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

		offset = ftell(dict_fp) - strlen(buf);
		//printf("key:%s entry:%s offset:%ld\n", key, entry, offset);
		tp = (mode == READ_OKURIARI) ? okuri_ari: okuri_nasi;

		tp->entries = (struct entry_t *) realloc(tp->entries, sizeof(struct entry_t) * (tp->count + 1));
		tp->entries[tp->count].key = (char *) emalloc(strlen(key) + 1);

		strcpy(tp->entries[tp->count].key, key);
		tp->entries[tp->count].offset = offset;
		tp->count++;
	}
}
