/*
	mkblank: create blank yaft font
	usage: ./mkblank WIDTH HEIGHT
		or ./mkblank WIDTH HEIGHT -cjk
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <wchar.h>
#include "wcwidth.h"

enum {
	UCS_MAX = 0x10000,
};

int main(int argc, char **argv)
{
	int i, j, tmp, width, height, ambiguous;
	int (*func)(wchar_t ucs);

	if (argc < 3) {
		printf("usage: ./mkblank WIDTH HEIGHT [-cjk]\n");
		return 1;
	}
	else {
		width = atoi(argv[1]);
		height = atoi(argv[2]);
	}

	if (argc >= 4)
		func = mk_wcwidth_cjk;
	else
		func = mk_wcwidth;

	/* wcha_t uses UCS4 in internal */
	for (i = 0; i < UCS_MAX; i++) {
		tmp = func(i);
		if (tmp <= 0)
			continue;

		tmp *= width;
		printf("%d\n%d %d\n", i, tmp, height);
		for (j = 0; j < height; j++)
			printf("%.2X\n", 0);
	}
}
