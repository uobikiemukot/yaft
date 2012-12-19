enum {
	MAX_PARAMS = 256,
};

struct parm_t {
	int argc;
	char *argv[MAX_PARAMS];
};

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
