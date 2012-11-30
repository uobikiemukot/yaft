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

void reset_parm(struct parm_t *pt)
{
	int i;

	pt->argc = 0;
	for (i = 0; i < MAX_PARAMS; i++)
		pt->argv[i] = NULL;
}

void parse_entry(char *buf, struct parm_t *pt, int delim, int (is_valid)(int c))
{
	int length;
	char *cp;

	length = strlen((char *) buf);
	cp = buf;

	//printf("buf:%s length:%d delim:%c\n", buf, length, delim);

	while (cp <= &buf[length - 1]) {
		if (*cp == delim)
			*cp = '\0';
		cp++;
	}
	cp = buf;

start:
	if (pt->argc <= MAX_PARAMS && is_valid(*cp)) {
		pt->argv[pt->argc] = (char *) cp;
		pt->argc++;
	}

	while (is_valid(*cp))
		cp++;

	while (!is_valid(*cp) && cp <= &buf[length - 1])
		cp++;

	if (cp <= &buf[length - 1])
		goto start;
}

int not_slash(int c)
{
	if (c != 0x2F && !iscntrl(c))
		return 1; /* true */
	else
		return 0; /* false */
}

void load_dict(struct entry_t *ari, int *ari_count, struct entry_t *nasi, int *nasi_count)
{
	//int i, j;
	int num, mode = RESET;
	FILE *fp;
	char buf[BUFSIZE], key[BUFSIZE], entry[BUFSIZE];
	struct parm_t *pt;
	struct entry_t *ep;

	fp = efopen(dict_file, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0)
			continue;

		if (strstr(buf, ";; okuri-ari entries") != NULL) {
			mode = MODE_ARI;
			continue;
		}
		else if (strstr(buf, ";; okuri-nasi entries") != NULL) {
			mode = MODE_NASI;
			continue;
		}

		if (mode == RESET)
			continue;

		memset(key, 0, BUFSIZE);
		memset(entry, 0, BUFSIZE);

		num = sscanf(buf, "%s %s", key, entry);
		//printf("num:%d mode:%d key:%s entry:%s\n", num, mode, key, entry);

		if (num != 2)
			continue;

		ep = (mode == MODE_ARI) ? &ari[*ari_count]: &nasi[*nasi_count];
		pt = &ep->parm;

		ep->key = (char *) emalloc(strlen(key) + 1);
		ep->entry = (char *) emalloc(strlen(entry) + 1);

		strcpy(ep->key, key);
		strcpy(ep->entry, entry);

		reset_parm(pt);
		parse_entry(ep->entry, pt, '/', not_slash);

		if (mode == MODE_ARI)
			(*ari_count)++;
		else
			(*nasi_count)++;
	}

	/*
	printf("ari:%d nasi:%d\n", *ari_count, *nasi_count);
	for (i = 0; i < nasi_count; i++) {
		ep = &nasi[i];
		pt = &ep->parm;

		printf("key:%s entry:%s\n", ep->key, ep->entry);
		printf("argc:%d\n", pt->argc);
		for (j = 0; j < pt->argc; j++)
			printf("argv[%d]: %s\n", j, pt->argv[j]);
	}
	*/

	efclose(fp);
}
