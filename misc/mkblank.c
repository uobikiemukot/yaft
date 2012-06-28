/*
	mkblank: create blank yaft font
	usage: ./mkblank
		or ./mkblank -cjk
*/
#include <stdio.h>
#include <math.h>
#include <wchar.h>
#include "wcwidth.h"

enum {
	DEFAULT_HALF_GLYPH = 0x20, /* half space */
	DEFAULT_WIDE_GLYPH = 0x3000, /* wide space */
	UCS2_MAX = 0x10000,
};

int main(int argc, char **argv)
{
	int i, width;
	int (*func)(wchar_t ucs);

	if (argc > 1)
		func = mk_wcwidth_cjk;
	else
		func = mk_wcwidth;

	/* wcha_t uses UCS4 in internal */
	for (i = 0; i < UCS2_MAX; i++) {
		width = func(i);
		if (width <= 0)
			continue;

		if (width == 1)
			printf("0x%.4X 0x%.4X\n", i, DEFAULT_HALF_GLYPH);
		else if (width == 2)
			printf("0x%.4X 0x%.4X\n", i, DEFAULT_WIDE_GLYPH);
	}
}
