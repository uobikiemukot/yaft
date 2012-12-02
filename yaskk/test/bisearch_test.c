#include "../common.h"
#include "../util.h"
#include "../hash.h"
#include "../load.h"

struct entry_t *bisearch(const char *key)
{
	extern struct entry_t ari[], nasi[];
	extern int ari_count, nasi_count;

	int low, mid, high, ret;

	low = 0;
	high = ari_count - 1;

	printf("key:%s\n", key);
	printf("low:%d high:%d\n", low, high);

	while (low <= high) {
		mid = (low + high) / 2;

		ret = strcmp(key, ari[mid].key);
		printf("mid.key:%s ret:%d\n", ari[mid].key, ret);

		if (ret == 0) /* hit! */
			return &ari[mid];
		else if (ret < 0) /* s1 < s2 */
			low = mid;
		else
			high = mid;
	}

	return NULL;
}

int main()
{
	extern struct entry_t ari[], nasi[];
	extern int ari_count, nasi_count;

	int i;
	const char *key = "ゆきつk";
	struct entry_t *entry;
	struct parm_t *pt;

	load_dict(ari, &ari_count, nasi, &nasi_count);
	printf("ari:%d nasi:%d\n", ari_count, nasi_count);

	entry = bisearch(key);

	if (entry == NULL)
		printf("key not found\n");
	else {
		pt = &entry->parm;
		printf("argc:%d\n", pt->argc);
		for (i = 0; i < pt->argc; i++)
			printf("argv[%d]:%s\n", i, pt->argv[i]);
	}
}
