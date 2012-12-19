int not_slash(int c)
{
	if (c != 0x2F && !iscntrl(c))
		return 1; /* true */
	else
		return 0; /* false */
}

int utf8_strlen(const char c[], int n)
{
	int i, ret = 0;
	uint8_t code;

	for (i = 0; i < n; i++) {
		code = c[i];
		if (code <= 0x7F || (0xC2 <= code && code <= 0xFD)) 
			ret++;
	}

	return ret;
}

void dict_init(struct table_t *okuri_ari, struct table_t *okuri_nasi)
{
	okuri_ari->count = okuri_nasi->count = 0;
	okuri_ari->entries = okuri_nasi->entries = NULL;
}

void map_init(struct map_t *mp)
{
	mp->count = 0;
	mp->triplets = NULL;
}

struct triplet_t *map_lookup(struct map_t *mp, const char *key, int len)
{
	int low, mid, high, ret;

	low = 0;
	high = mp->count - 1;
	//fprintf(stderr, "\tmap lookup key:%s len:%d\n", key, len);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strncmp(key, mp->triplets[mid].key, len);
		//fprintf(stderr, "\tlow:%d mid:%d high:%d mid key:%s ret:%d\n",
			//low, mid, high, mp->triplets[mid].key, ret);

		if (ret == 0)
			return &mp->triplets[mid];
		else if (ret < 0) /* s1 < s2 */
			high = mid - 1;
		else
			low = mid + 1;
	}

	return NULL;
}

struct entry_t *table_lookup(struct table_t *tp, const char *key)
{
	int low, mid, high, ret;

	low = 0;
	high = tp->count - 1;
	//fprintf(stderr, "\ttable lookup key:%s\n", key);

	while (low <= high) {
		mid = (low + high) / 2;
		ret = strcmp(key, tp->entries[mid].key);
		//fprintf(stderr, "\tlow:%d mid:%d high:%d mid key:%s ret:%d\n",
			//low, mid, high, tp->entries[mid].key, ret);

		if (ret == 0)
			return &tp->entries[mid];
		else if (ret < 0) /* s1 < s2 */
			high = mid - 1;
		else
			low = mid + 1;
	}

	return NULL;
}
