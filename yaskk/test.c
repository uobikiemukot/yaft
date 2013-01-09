#include "common.h"
#include "util.h"
#include "misc.h"
#include "list.h"
#include "hash.h"
#include "load.h"

int main()
{
	const char test[] = "▽あいうえお";

	printf("%s: len:%d\n", test, utf8_len(test, strlen(test)));
}
