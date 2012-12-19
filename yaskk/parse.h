bool mode_check(struct linebuf_t *lb, char *c)
{
	if (lb->mode & MODE_ASCII) {
		if (*c == LF) {
			lb->mode = MODE_HIRA;
			return true;
		}
	}
	else if (lb->mode & MODE_COOK) {
		if (*c == SPACE) {
			lb->mode &= ~MODE_COOK;
			lb->mode |= MODE_SELECT;
			list_erase_front_n(&lb->pre, list_size(&lb->pre));
		}
		else if (isupper(*c)) {
			lb->mode &= ~MODE_COOK;
			lb->mode |= MODE_APPEND;
			*c = tolower(*c);
			list_push_back(&lb->key, *c);
		}
		else if (*c == LF) {
			lb->mode &= ~MODE_COOK;
			linebuf_write_list(lb, &lb->key, list_size(&lb->key));
			return true;
		}
		/* not implemented: hira <--> kata convert
		else if (*c == 'q') {
		}
		*/
	}
	else if (lb->mode & MODE_SELECT) {
		if (*c == ESC) {
			lb->mode &= ~MODE_SELECT;
			lb->mode |= MODE_COOK;
			lb->select = SELECT_EMPTY;
			list_erase_front_n(&lb->pre, list_size(&lb->pre));
			if (lb->mode & MODE_APPEND) {
				list_erase_back(&lb->key);
				lb->mode &= ~MODE_APPEND;
			}
			*c = '\0';
		}
		else if (*c == LF) {
			lb->mode &= ~MODE_SELECT;
			write(lb->fd, lb->parm.argv[lb->select], strlen(lb->parm.argv[lb->select]));
			lb->select = SELECT_EMPTY;
			if (lb->mode & MODE_APPEND) {
				linebuf_write_list(lb, &lb->pre, list_size(&lb->pre));
				lb->mode &= ~MODE_APPEND;
			}
			return true;
		}
		else if (*c == 'l') {
			lb->mode = MODE_ASCII;
			write(lb->fd, lb->parm.argv[lb->select], strlen(lb->parm.argv[lb->select]));
			lb->select = SELECT_EMPTY;
			if (lb->mode & MODE_APPEND) {
				linebuf_write_list(lb, &lb->pre, list_size(&lb->pre));
				lb->mode &= ~MODE_APPEND;
			}
			return true;
		}
		else if (*c == 'q') {
			lb->mode = (lb->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA;
			write(lb->fd, lb->parm.argv[lb->select], strlen(lb->parm.argv[lb->select]));
			lb->select = SELECT_EMPTY;
			if (lb->mode & MODE_APPEND) {
				linebuf_write_list(lb, &lb->pre, list_size(&lb->pre));
				lb->mode &= ~MODE_APPEND;
			}
			return true;
		}
		else if (*c != SPACE && *c != 'x') {
			lb->mode &= ~MODE_SELECT;
			write(lb->fd, lb->parm.argv[lb->select], strlen(lb->parm.argv[lb->select]));
			lb->select = SELECT_EMPTY;
			if (lb->mode & MODE_APPEND) {
				linebuf_write_list(lb, &lb->pre, list_size(&lb->pre));
				lb->mode &= ~MODE_APPEND;
			}
			list_erase_front_n(&lb->pre, list_size(&lb->pre));
			list_erase_front_n(&lb->key, list_size(&lb->key));

			if (isupper(*c)) {
				lb->mode |= MODE_COOK;
				*c = tolower(*c);
			}
		}
	}
	else if (lb->mode & MODE_HIRA || lb->mode & MODE_KATA) {
		if (*c == 'l' && list_back(&lb->pre) != 'z') {
			lb->mode = MODE_ASCII;
			return true;
		}
		else if (*c == 'q') {
			lb->mode = (lb->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA;
			return true;
		}
		else if (isupper(*c)) {
			lb->mode |= MODE_COOK;
			*c = tolower(*c);
		}
	}
	return false;
}

void parse_control(struct linebuf_t *lb, char c)
{
	char code;

	fprintf(stderr, "\tparse control c:0x%.2X\n", c);

	if (c == BS) {
		if (list_size(&lb->pre) > 0)
			list_erase_back(&lb->pre);
		else if (list_size(&lb->key) > 0) {
			while (lb->key != NULL) {
				code = list_back(&lb->key);
				list_erase_back(&lb->key);
				if (utf8_strlen(&code, 1) > 0)
					break;
			}
		}
		else if (lb->mode & MODE_COOK)
			lb->mode &= ~MODE_COOK;
		else
			write(lb->fd, &c, 1);
	}
	else
		write(lb->fd, &c, 1);
}

void parse_ascii(struct linebuf_t *lb, char c)
{
	write(lb->fd, &c, 1);
}

void get_candidate(struct entry_t *ep, struct parm_t *parm, char *entry)
{
	extern FILE *dict_fp;
	int i;
	char buf[BUFSIZE], key[BUFSIZE];

	fseek(dict_fp, ep->offset, SEEK_SET);
	fgets(buf, BUFSIZE, dict_fp);
	sscanf(buf, "%s %s", key, entry);

	reset_parm(parm);
	parse_entry(entry, parm, '/', not_slash);

	fprintf(stderr, "argc:%d\n", parm->argc);
	for (i = 0; i < parm->argc; i++)
		fprintf(stderr, "argv[%d]:%s\n", i, parm->argv[i]);
}

void parse_select(struct linebuf_t *lb, char c, int size)
{
	extern struct table_t okuri_ari, okuri_nasi;
	char str[size + 1];
	struct entry_t *ep;
	struct table_t *tp;

	memset(str, '\0', size + 1);
	list_front_n(&lb->key, str, size);

	fprintf(stderr, "\tparse select str:%s select:%d\n", str, lb->select);
	tp = (c == SPACE) ? &okuri_nasi: &okuri_ari;

	if (lb->select >= SELECT_LOADED) {
		if (c == SPACE) {
			lb->select++;
			if (lb->select >= lb->parm.argc)
				lb->select = lb->parm.argc - 1; /* dictionary mode: not implemented */
		}
		else if (c == 'x') {
			lb->select--;
			if (lb->select < 0) {
				lb->mode &= ~MODE_SELECT;
				lb->mode |= MODE_COOK;
				lb->select = SELECT_EMPTY;
				list_erase_front_n(&lb->pre, list_size(&lb->pre));
				if (lb->mode & MODE_APPEND) {
					list_erase_back(&lb->key);
					lb->mode &= ~MODE_APPEND;
				}
			}
		}
	}
	else if ((ep = table_lookup(tp, str)) != NULL) {
		fprintf(stderr, "\tmatched!\n");
		get_candidate(ep, &lb->parm, lb->entry);
		if (lb->parm.argc > 0)
			lb->select = SELECT_LOADED;
		else {
			lb->mode &= ~MODE_SELECT;
			lb->mode |= MODE_COOK;
		}
	}
	else {
		fprintf(stderr, "\tunmatched!\n");
 		/* dictionary mode: not implemented */
		lb->mode &= ~MODE_SELECT;
		lb->mode |= MODE_COOK;
	}
}

void parse_kana(struct linebuf_t *lb, int len)
{
	extern struct map_t rom2kana;
	int size;
	char str[len + 1], *val;
	struct triplet_t *tp;

	size = list_size(&lb->pre);
	if (size < len) {
		fprintf(stderr, "\tstop! buffer size:%d compare length:%d\n", size, len);
		return;
	}

	memset(str, '\0', len + 1);
	list_front_n(&lb->pre, str, len);
	fprintf(stderr, "\tparse kana compare length:%d list size:%d str:%s\n", len, size, str);

	if ((tp = map_lookup(&rom2kana, str, KEYSIZE - 1)) != NULL) {
		fprintf(stderr, "\tmatched!\n");
		val = (lb->mode & MODE_HIRA) ? tp->hira: tp->kata;
		if (lb->mode & MODE_COOK)
			list_push_back_n(&lb->key, tp->hira, strlen(tp->hira));
		else if (lb->mode & MODE_APPEND) {
			list_push_back_n(&lb->pre, tp->hira, strlen(tp->hira));
			lb->mode |= MODE_SELECT;
			parse_select(lb, list_back(&lb->pre), list_size(&lb->key));
		}
		else
			write(lb->fd, val, strlen(val));

		list_erase_front_n(&lb->pre, len);

		if (str[0] == str[1] && str[0] != 'n') {
			list_push_front(&lb->pre, str[0]);
		}
	}
	else {
		if ((tp = map_lookup(&rom2kana, str, len)) != NULL) {
			fprintf(stderr, "\tpartly matched!\n");
			parse_kana(lb, len + 1);
		}
		else {
			fprintf(stderr, "\tunmatched! ");
			if (str[0] == 'n') {
				fprintf(stderr, "add 'n'\n");
				list_push_front(&lb->pre, 'n');
				parse_kana(lb, len);
			}
			else {
				fprintf(stderr, "erase front char\n");
				list_erase_front(&lb->pre);
				parse_kana(lb, len - 1);
			}
		}
	}

}

void parse(struct linebuf_t *lb, char *buf, int size)
{
	int i;
	char c;

	buf[size] = '\0';
	fprintf(stderr, "start parse buf:%s size:%d mode:%d\n", buf, size, lb->mode);
	linebuf_erase_preedit(lb);

	for (i = 0; i < size; i++) {
		c = buf[i];

		if (mode_check(lb, &c)) {
			/* mode changed */
			list_erase_front_n(&lb->pre, list_size(&lb->pre));
			list_erase_front_n(&lb->key, list_size(&lb->key));
		}
		else if (iscntrl(c)) {
			parse_control(lb, c);
		}
		else if (lb->mode & MODE_SELECT) {
			parse_select(lb, c, list_size(&lb->key));
		}
		else if (lb->mode & MODE_ASCII) {
			parse_ascii(lb, c);
		}
		else if (lb->mode & MODE_HIRA || lb->mode & MODE_KATA) {
			list_push_back(&lb->pre, c);
			parse_kana(lb, 1);
		}
	}
	linebuf_write_preedit(lb, list_size(&lb->pre), list_size(&lb->key));
}
