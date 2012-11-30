int parse_ascii(int fd, struct list_t **list)
{
	char c;

	while (*list != NULL) {
		list_front(list, &c);
		list_erase_front(list);

		if (c == LF)
			return MODE_HIRAGANA;
		else
			write(fd, &c, 1);
	}

	return MODE_ASCII;
}

int parse_hiragana(int fd, struct list_t **list)
{
	extern struct map_t *r2h[];
	int n;
	char c[KEYSIZE];
	struct map_t *mp;

	memset(c, '\0', KEYSIZE);
	n = list_size(list);
	list_front_n(list, &c[0], n);

	if (n >= KEYSIZE) { /* too many input: erase all */
		list_erase_front_n(list, n);
		return MODE_HIRAGANA;
	}

	if (c[n - 1] == 'l') { /* check mode change */
		list_erase_front_n(list, n);
		return MODE_ASCII;
	}
	else if (isupper(c[n - 1])) {
		return MODE_HENKAN;
	}
	else if (c[n - 1] == 'q') {
		list_erase_front_n(list, n);
		return MODE_KATAKANA;
	}
	else if ((mp = hash_lookup(r2h, &c[0], NULL, false)) != NULL) { /* lookup hash */
		list_erase_front_n(list, n);
		write(fd, mp->val, strlen(mp->val));

		if (n == 2 && c[0] == c[1]) /* bb cc dd... */
			list_push_front(list, c[0]);
	}
	else if (!isalpha(c[n - 1])) {
		list_erase_front_n(list, n);
		write(fd, &c[n - 1], 1);
	}

	return MODE_HIRAGANA;
}

int parse_katakana(int fd, struct list_t **list)
{
	extern struct map_t *r2k[];
	int n;
	char c[KEYSIZE];
	struct map_t *mp;

	memset(c, '\0', KEYSIZE);
	n = list_size(list);
	list_front_n(list, &c[0], n);

	if (n >= KEYSIZE) { /* too many input: erase all */
		list_erase_front_n(list, n);
		return MODE_KATAKANA;
	}

	if (c[n - 1] == 'l') { /* check mode change */
		list_erase_front_n(list, n);
		return MODE_ASCII;
	}
	else if (isupper(c[n - 1])) {
		write(fd, mark_cook, strlen(mark_cook));
		return MODE_HENKAN;
	}
	else if (c[n - 1] == 'q') {
		list_erase_front_n(list, n);
		return MODE_HIRAGANA;
	}
	else if ((mp = hash_lookup(r2k, &c[0], NULL, false)) != NULL) { /* lookup hash */
		list_erase_front_n(list, n);
		write(fd, mp->val, strlen(mp->val));

		if (n == 2 && c[0] == c[1]) /* bb cc dd... */
			list_push_front(list, c[0]);
	}
	else if (!isalpha(c[n - 1])) {
		list_erase_front_n(list, n);
		write(fd, &c[n - 1], 1);
	}

	return MODE_KATAKANA;
}

int parse_zenei(int fd, struct list_t **list)
{
	/* not implemented */
	return MODE_ZENEI;
}

int parse_hankana(int fd, struct list_t **list)
{
	/* not implemented */
	return MODE_ZENEI;
}

int parse_henkan()
{
	return MODE_HENKAN;
}

int (*parse[])(int fd, struct list_t **cl) = {
	[MODE_ASCII]	= parse_ascii,
	[MODE_HIRAGANA] = parse_hiragana,
	[MODE_KATAKANA] = parse_katakana,
	[MODE_ZENEI]	= parse_zenei,
	[MODE_HANKANA]  = parse_hankana,
	[MODE_HENKAN]   = parse_henkan,
};
