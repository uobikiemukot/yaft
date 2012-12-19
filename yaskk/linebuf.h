void linebuf_init(struct linebuf_t *lb)
{
	lb->pre = lb->key = NULL;
	lb->pwrote = lb->kwrote = 0;
	lb->select = SELECT_EMPTY;
	lb->mode = MODE_ASCII;
}

void linebuf_write_list(struct linebuf_t *lb, struct list_t **list, int size)
{
	char str[size + 1];

	memset(str, '\0', size + 1);
	list_front_n(list, str, size);
	write(lb->fd, str, size);
}

void linebuf_erase_preedit(struct linebuf_t *lb)
{
	int i;

	for (i = 0; i < lb->pwrote; i++)
		write(lb->fd, backspace, strlen(backspace));

	for (i = 0; i < lb->kwrote; i++)
		write(lb->fd, backspace, strlen(backspace));

	lb->pwrote = lb->kwrote = 0;
}

void linebuf_write_preedit(struct linebuf_t *lb, int psize, int ksize)
{
	char pstr[psize + 1], kstr[ksize + 1];
	const char *str;

	if (lb->mode & MODE_SELECT) {
		write(lb->fd, mark_select, strlen(mark_select));
		lb->kwrote = utf8_strlen(mark_select, strlen(mark_select));

		str = lb->parm.argv[lb->select];
		write(lb->fd, str, strlen(str));
		lb->kwrote += utf8_strlen(str, strlen(str));
	}
	else if (lb->mode & MODE_COOK || lb->mode & MODE_APPEND) {
		write(lb->fd, mark_cook, strlen(mark_cook));
		lb->kwrote = utf8_strlen(mark_cook, strlen(mark_cook));

		memset(kstr, '\0', ksize + 1);
		if (lb->mode & MODE_APPEND)
			ksize--;
		list_front_n(&lb->key, kstr, ksize);
		write(lb->fd, kstr, ksize);
		lb->kwrote += utf8_strlen(kstr, ksize);
	}

	memset(pstr, '\0', psize + 1);
	list_front_n(&lb->pre, pstr, psize);
	write(lb->fd, pstr, psize);
	lb->pwrote = utf8_strlen(pstr, psize);

	fprintf(stderr, "\trefresh preedit psize:%d pwrote:%d ksize:%d kwrote:%d\n",
		psize, lb->pwrote, ksize, lb->kwrote);
}
