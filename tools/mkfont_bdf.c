#include "mkfont_bdf.h"
#include "../conf.h"
#include "font.h"
#include "bdf.h"

int main(int argc, char *argv[])
{
	int i;
	struct glyph_t fonts[UCS2_CHARS];

	setlocale(LC_ALL, ""); /* set current locale for wcwidth() */

	if (argc < 3) {
		fprintf(stderr, "usage: ./mkfont ALIAS BDF1 [BDF2] [BDF3] ...\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < UCS2_CHARS; i++)
		fonts[i].bitmap = NULL;

	for (i = 2; i < argc; i++)
		load_bdf_glyph(fonts, argv[i]);

	load_alias(fonts, argv[1]);

	check_fonts(fonts);

	dump_fonts(fonts);

	return EXIT_SUCCESS;
}
