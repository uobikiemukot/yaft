enum hash_parm {
	NHASH = 512,
	MULTIPLIER = 37, // or 31
	KEYSIZE = 4,
	VALSIZE = 16,
};

struct hash_t {
	char key[KEYSIZE], val[VALSIZE];
	struct hash_t *next;
};

void safe_strncpy(char *dst, const char *src, size_t n)
{
	strncpy(dst, src, n);
	if (n > 0)
		dst[n - 1]= '\0';
}

void hash_init(struct hash_t *symtable[])
{
	int i;

	for (i = 0; i < NHASH; i++)
		symtable[i] = NULL;
}

int hash_key(char *cp)
{
	/*
	int ret = 0;

	while (*cp != 0)
		ret = MULTIPLIER * ret + *cp++;

	return ret % NHASH;
	*/
	return 0;
}

struct hash_t *hash_lookup(struct hash_t *symtable[], char *key, int len, char *val, bool create)
{
	int hkey;
	struct hash_t *hp;

	hkey = hash_key(key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strncmp(key, hp->key, len) == 0)
			return hp;
	}

	if (create) {
		hp = (struct hash_t *) emalloc(sizeof(struct hash_t));
		safe_strncpy(hp->key, key, KEYSIZE);
		safe_strncpy(hp->val, val, VALSIZE);
		hp->next = symtable[hkey];
		symtable[hkey] = hp;
		return hp;
	}

	return NULL;
}
