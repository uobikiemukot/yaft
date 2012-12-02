void load_map(struct hash_t *r2h[], struct hash_t *r2k[])
{
	int num;
	char buf[BUFSIZE], key[KEYSIZE], hira[VALSIZE], kata[VALSIZE];
	FILE *fp;

	fp = efopen(map_file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		num = sscanf(buf, "%s\t%s\t%s", key, hira, kata);
		if (num == 3) {
			hash_lookup(r2h, key, hira, true);
			hash_lookup(r2k, key, kata, true);
		}
	}

	efclose(fp);
}

void load_dict(struct table_t *okuri_ari, struct table_t *okuri_nasi)
{
}
