void hash_init(struct hash_t *symtable[])
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

struct hash_t *hash_lookup(struct hash_t *symtable[], char *key, char *val, bool create)
{
	int hkey;
	struct hash_t *hp;

	hkey = hash_key(key);
	for (hp = symtable[hkey]; hp != NULL; hp = hp->next) {
		if (strcmp(key, hp->key) == 0)
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
