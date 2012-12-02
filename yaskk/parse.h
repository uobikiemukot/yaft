int parse_ascii(int fd, struct list_t **list, int *wrote)
{
	char c;

	list_front(list, &c);
	list_erase_front(list);

	if (c == LF)
		return MODE_HIRAGANA;
	else
		write(fd, &c, 1);

	return MODE_ASCII;
}

void buffer_reset(int fd, struct list_t **list, int *wrote, int n)
{
	int i;

	list_erase_front_n(list, n);

	if (*wrote > 0) {
		write(STDOUT_FILENO, sgr_reset, strlen(sgr_reset));
		for (i = 0; i < *wrote; i++)
			write(fd, backspace, strlen(backspace));
		*wrote = 0;
	}
}

int parse_hiragana(int fd, struct list_t **list, int *wrote)
{
	extern struct hash_t *rom2hira[];
	int n;
	char c[KEYSIZE];
	struct hash_t *hp;

	memset(c, '\0', KEYSIZE);
	n = list_size(list);

	if (n >= KEYSIZE) { /* too many chars in list */
		list_erase_front_n(list, n);
		return MODE_HIRAGANA;
	}

	list_front_n(list, &c[0], n);

	if (c[n - 1] == 'l') {
		//list_erase_front_n(list, n);
		buffer_reset(fd, list, wrote, n);
		return MODE_ASCII;
	}
	else if (c[n - 1] == 'q') {
		//list_erase_front_n(list, n);
		buffer_reset(fd, list, wrote, n);
		return MODE_KATAKANA;
	}
	else if (isupper(c[n - 1])) {
		return MODE_HENKAN;
	}
	else if ((hp = hash_lookup(rom2hira, &c[0], NULL, false)) != NULL) {
		//list_erase_front_n(list, n);
		buffer_reset(fd, list, wrote, n);
		write(fd, hp->val, strlen(hp->val));

		if (n == 2 && c[0] == c[1])
			list_push_front(list, c[0]);
	}
	else if (iscntrl(c[n - 1])) {
		//list_erase_front_n(list, n);
		buffer_reset(fd, list, wrote, n);
		write(fd, &c[n - 1], 1);
	}
	else {
		if (*wrote == 0) {
			write(STDOUT_FILENO, save_cursor, strlen(save_cursor));
			write(STDOUT_FILENO, sgr_underline, strlen(sgr_underline));
		}
		write(fd, &c[n - 1], 1);
		*wrote += 1;
	}

	return MODE_HIRAGANA;
}

int parse_katakana()
{
	return MODE_KATAKANA;
}

int parse_zenei()
{
	/* not implemented */
	return MODE_ZENEI;
}

int parse_hankana()
{
	/* not implemented */
	return MODE_ZENEI;
}

int parse_henkan()
{
	return MODE_HENKAN;
}

int (*parse[])(int fd, struct list_t **list, int *wrote) = {
	[MODE_ASCII]	= parse_ascii,
	[MODE_HIRAGANA] = parse_hiragana,
	[MODE_KATAKANA] = parse_katakana,
	[MODE_ZENEI]	= parse_zenei,
	[MODE_HANKANA]  = parse_hankana,
	[MODE_HENKAN]   = parse_henkan,
};
