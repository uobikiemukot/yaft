#include "../common.h"
#include "../util.h"
#include "../hash.h"
#include "../load.h"

int main()
{
	//extern struct entry_t ari[], nasi[];
	int i, n, len;
	char test[] = "わたt /渡/亘/亙/渉/航/",
		key[BUFSIZE], entry[BUFSIZE];
	struct parm_t parm;

	memset(key, 0, BUFSIZE);
	memset(entry, 0, BUFSIZE);

	n = sscanf(test, "%s %s", key, entry);
	printf("n:%d\n", n);

	len = strlen(entry);
	entry[len - 1] = '\0';

	reset_parm(&parm);
	parse_entry(entry, &parm, '/', not_slash);

	printf("argc:%d\n", parm.argc);
	for (i = 0; i < parm.argc; i++)
		printf("argv[%d]: %s\n", i, parm.argv[i]);
}
