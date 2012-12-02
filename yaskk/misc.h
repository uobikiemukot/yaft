void safe_strncpy(char *dst, const char *src, size_t n)
{
	strncpy(dst, src, n);
	if (n > 0)
		dst[n - 1]= '\0';
}

int not_slash(int c)
{
	if (c != 0x2F && !iscntrl(c))
		return 1; /* true */
	else
		return 0; /* false */
}

int utf8_length(int code)
{
	if (code <= 0x7F)
		return 1;
	else if (0xC2 <= code && code <= 0xDF)
		return 2;
	else if (0xE0 <= code && code <= 0xEF)
		return 3;
	else if (0xF0 <= code && code <= 0xF7)
		return 4;
	else if (0xF8 <= code && code <= 0xFB)
		return 5;
	else if (0xFC <= code && code <= 0xFD)
		return 6;
	else
		return 1;
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
