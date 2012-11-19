#include "font.h"
#include "bdf.h"

int main(int argc, char *argv[])
{
	int i;
	struct glyph_t fonts[UCS2_CHARS];

	setlocale(LC_ALL, "");

	if (argc < 3) {
		fprintf(stderr, "usage: ./mkfont ALIAS BDF1 [BDF2] [BDF3] ...\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < UCS2_CHARS; i++)
		fonts[i].bitmap = NULL;

	for (i = 2; i < argc; i++)
		load_bdf_glyph(fonts, argv[i]);

	load_alias(fonts, argv[1]);

	if (fonts[DEFAULT_CHAR].bitmap == NULL) {
		fprintf(stderr, "default glyph(U+%.4X) not found\n", DEFAULT_CHAR);
		exit(EXIT_FAILURE);
	}

	if (fonts[SUBSTITUTE_HALF].bitmap == NULL) {
		fprintf(stderr, "half substitute glyph(U+%.4X) not found\n", SUBSTITUTE_HALF);
		exit(EXIT_FAILURE);
	}

	if (fonts[SUBSTITUTE_WIDE].bitmap == NULL) {
		fprintf(stderr, "wide substitute glyph(U+%.4X) not found\n", SUBSTITUTE_WIDE);
		exit(EXIT_FAILURE);
	}

	dump_fonts(fonts);
}
