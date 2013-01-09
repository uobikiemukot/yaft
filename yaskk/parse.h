bool mode_check(struct skk_t *skk, char *c)
{
	if (skk->mode & MODE_ASCII) {
		if (*c == LF) {
			change_mode(skk, MODE_HIRA);
			return true;
		}
	}
	else if (skk->mode & MODE_COOK) {
		if (*c == SPACE) {
			change_mode(skk, MODE_SELECT);
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
		}
		else if (isupper(*c)) {
			*c = tolower(*c);
			if (list_size(&skk->preedit) > 0
				&& (*c == 'a' || *c == 'i' || *c == 'u' || *c == 'e' || *c == 'o')) {
				change_mode(skk, MODE_APPEND);
				list_push_back(&skk->key, list_front(&skk->preedit));
			}
			else if (list_size(&skk->key) > 0) {
				change_mode(skk, MODE_APPEND);
				list_push_back(&skk->key, *c);
			}
		}
		else if (*c == LF) {
			change_mode(skk, ~MODE_COOK);
			write_list(skk->fd, &skk->key, list_size(&skk->key));
			return true;
		}
	}
	else if (skk->mode & MODE_SELECT) {
		if (*c == ESC) {
			reset_candidate(skk, false);
			change_mode(skk, ~MODE_SELECT);
			list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
			*c = '\0';
		}
		else if (*c == LF) {
			reset_candidate(skk, true);
			change_mode(skk, ~MODE_SELECT);
			return true;
		}
		else if (*c == 'l') {
			reset_candidate(skk, true);
			change_mode(skk, MODE_ASCII);
			return true;
		}
		else if (*c == 'q') {
			reset_candidate(skk, true);
			change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
			return true;
		}
		else if (*c != SPACE && *c != 'x') {
			reset_candidate(skk, true);
			change_mode(skk, ~MODE_SELECT);
			reset_buffer(skk);

			if (isupper(*c)) {
				change_mode(skk, MODE_COOK);
				*c = tolower(*c);
			}
		}
	}
	else if (skk->mode & MODE_APPEND) {
		if (isupper(*c)) {
			*c = tolower(*c);
		}
	}
	else if (skk->mode & MODE_HIRA || skk->mode & MODE_KATA) {
		if (*c == 'l' && list_back(&skk->preedit) != 'z') {
			change_mode(skk, MODE_ASCII);
			return true;
		}
		else if (*c == 'q') {
			change_mode(skk, (skk->mode & MODE_HIRA) ? MODE_KATA: MODE_HIRA);
			return true;
		}
		else if (isupper(*c)) {
			change_mode(skk, MODE_COOK);
			*c = tolower(*c);
		}
	}
	return false;
}

void parse_control(struct skk_t *sp, char c)
{
	fprintf(stderr, "\tparse control c:0x%.2X\n", c);

	if (c == BS)
		delete_char(sp, c);
	else
		write(sp->fd, &c, 1);
}

void parse_ascii(struct skk_t *sp, char c)
{
	write(sp->fd, &c, 1);
}

void parse_select(struct skk_t *sp, char c, int size)
{
	char str[size + 1];
	struct entry_t *ep;
	struct table_t *tp;

	memset(str, '\0', size + 1);
	list_front_n(&sp->key, str, size);

	fprintf(stderr, "\tparse select str:%s select:%d\n", str, sp->select);
	tp = (c == SPACE) ? &sp->okuri_nasi: &sp->okuri_ari;

	if (sp->select >= SELECT_LOADED) {
		if (c == SPACE) {
			sp->select++;
			if (sp->select >= sp->candidate.parm.argc)
				sp->select = SELECT_LOADED; /* dictionary mode: not implemented */
		}
		else if (c == 'x') {
			sp->select--;
			if (sp->select < 0)  {
				sp->select = sp->candidate.parm.argc - 1;
				/*
				change_mode(skk, MODE_COOK);
				reset_candidate(skk, false);
				list_erase_front_n(&skk->preedit, list_size(&skk->preedit));
				*/
			}
		}
	}
	else if ((ep = table_lookup(tp, str)) != NULL) {
		fprintf(stderr, "\tmatched!\n");
		if (get_candidate(&sp->candidate, ep)) {
			fprintf(stderr, "\tcandidate num:%d\n", sp->candidate.parm.argc);
			sp->select = SELECT_LOADED;
		}
		else {
			fprintf(stderr, "\tcandidate num:%d\n", sp->candidate.parm.argc);
			change_mode(sp, MODE_COOK);
		}
	}
	else {
		fprintf(stderr, "\tunmatched!\n");
		change_mode(sp, MODE_COOK);
	}
}

void parse_kana(struct skk_t *skk, int len)
{
	int size;
	char str[len + 1];
	bool is_double_consonant = false;
	struct triplet_t *tp;

	size = list_size(&skk->preedit);
	if (size < len || len <= 0) {
		fprintf(stderr, "\tstop! buffer size:%d compare length:%d\n", size, len);
		return;
	}

	memset(str, '\0', len + 1);
	list_front_n(&skk->preedit, str, len);
	fprintf(stderr, "\tparse kana compare length:%d list size:%d str:%s\n", len, size, str);

	if ((tp = map_lookup(&skk->rom2kana, str, KEYSIZE - 1)) != NULL) {
		fprintf(stderr, "\tmatched!\n");
		if (size >= 2 && (str[0] == str[1] && str[0] != 'n')) {
			is_double_consonant = true;
			list_push_front(&skk->preedit, str[0]);
		}
		write_buffer(skk, tp, is_double_consonant);
		list_erase_front_n(&skk->preedit, len);
	}
	else {
		if ((tp = map_lookup(&skk->rom2kana, str, len)) != NULL) {
			fprintf(stderr, "\tpartly matched!\n");
			parse_kana(skk, len + 1);
		}
		else {
			fprintf(stderr, "\tunmatched! ");
			if (str[0] == 'n') {
				fprintf(stderr, "add 'n'\n");
				list_push_front(&skk->preedit, 'n');
				parse_kana(skk, len);
			}
			else {
				fprintf(stderr, "erase front char\n");
				list_erase_front(&skk->preedit);
				parse_kana(skk, len - 1);
			}
		}
	}

}

void parse(struct skk_t *skk, char *buf, int size)
{
	int i;
	char c;

	buf[size] = '\0';
	fprintf(stderr, "start parse buf:%s size:%d mode:%s\n", buf, size, mode_str[skk->mode]);
	erase_buffer(skk);

	for (i = 0; i < size; i++) {
		c = buf[i];

		if (mode_check(skk, &c))
			reset_buffer(skk);
		else if (c < SPACE || c >= DEL)
			parse_control(skk, c);
		else if (skk->mode & MODE_ASCII)
			parse_ascii(skk, c);
		else if (skk->mode & MODE_SELECT)
			parse_select(skk, c, list_size(&skk->key));
		else if (skk->mode & MODE_HIRA || skk->mode & MODE_KATA) {
			list_push_back(&skk->preedit, c);
			parse_kana(skk, 1);

			if (skk->mode & MODE_SELECT)
				parse_select(skk, c, list_size(&skk->key));
		}
	}
	redraw_buffer(skk);
}
