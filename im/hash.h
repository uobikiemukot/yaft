void hash_init(struct map_t *symtable[])
{
	int i;

	for (i = 0; i < NHASH; i++)
		symtable[i] = NULL;
}

int hash_key(char *cp)
{
	int ret = 0;

	while (*cp != 0)
		ret = MULTIPLIER * ret + *cp++;

	return ret % NHASH;
}

char *copy(char *dst, const char *src, size_t n)
{
	int i;

	for (i = 0; i < n - 1; i++)
		dst[i] = src[i];
	dst[n - 1] = '\0';

	return dst;
}

struct map_t *hash_lookup(struct map_t *map[], char *key, char *val, bool create)
{
	int hkey;
	struct map_t *mp;

	hkey = hash_key(key);
	for (mp = map[hkey]; mp != NULL; mp = mp->next) {
		if (strcmp(key, mp->key) == 0)
			return mp;
	}

	if (create) {
		mp = (struct map_t *) emalloc(sizeof(struct map_t));
		copy(mp->key, key, KEYSIZE);
		copy(mp->val, val, VALSIZE);
		mp->next = map[hkey];
		map[hkey] = mp;
		return mp;
	}

	return NULL;
}
