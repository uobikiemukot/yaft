#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <wchar.h>

enum {
	SUBSTITUTE_HALF = 0x20, /* SPACE */
	//SUBSTITUTE_HALF = 0xFFFD, /* REPLACEMENT CHARACTER: ambiguous width */
	SUBSTITUTE_WIDE = 0x3000, /* IDEOGRAPHIC SPACE */
	UCS2_CHARS = 0x10000,
	DEFAULT_CHAR = 0x20,
	BUFSIZE = 1024,
	BITS_PER_BYTE = 8,
};

struct glyph_t {
	uint8_t width, height;
	uint32_t *bitmap;
};

void fatal(char *str)
{
	perror(str);
	exit(EXIT_FAILURE);
}

FILE *efopen(char *path, char *mode)
{
	FILE *fp;

	if ((fp = fopen(path, mode)) == NULL) {
		fprintf(stderr, "cannot open \"%s\"\n", path);
		fatal("fopen");
	}
	return fp;
}

void efclose(FILE *fp)
{
	if (fclose(fp) < 0)
		fatal("fclose");
}

void *emalloc(size_t size)
{
	void *p;

	if ((p = calloc(1, size)) == NULL)
		fatal("calloc");
	return p;
}

void load_glyph(struct glyph_t *fonts, char *path)
{
	int count = 0, state = 0;
	char buf[BUFSIZE], *endp;
	FILE *fp;
	uint16_t code = DEFAULT_CHAR;

	fp = efopen(path, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		switch (state) {
		case 0:
			code = atoi(buf);
			if (fonts[code].bitmap != NULL) {
				free(fonts[code].bitmap);
				fonts[code].bitmap = NULL;
			}
			state = 1;
			break;
		case 1:
			sscanf(buf, "%hhu %hhu", &fonts[code].width, &fonts[code].height);
			fonts[code].bitmap = (uint32_t *) emalloc(fonts[code].height * sizeof(uint32_t));
			state = 2;
			break;
		case 2:
			fonts[code].bitmap[count++] = strtol(buf, &endp, 16);
			if (count >= fonts[code].height)
				state = count = 0;
			break;
		default:
			break;
		}
	}

	efclose(fp);
}

void load_alias(struct glyph_t *fonts, char *alias)
{
	unsigned int dst, src;
	char buf[BUFSIZE];
	FILE *fp;

	fp = efopen(alias, "r");

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		sscanf(buf, "%X %X", &dst, &src);
		if (dst < 0 || dst >= UCS2_CHARS
			|| src < 0 || src >= UCS2_CHARS)
			continue;

		if (fonts[src].bitmap != NULL) {
			fprintf(stderr, "swapped: use U+%.4X for U+%.4X\n", src, dst);

			free(fonts[dst].bitmap);
			fonts[dst].width = fonts[src].width;
			fonts[dst].height = fonts[src].height;
			fonts[dst].bitmap = fonts[src].bitmap;
		}
	}

	efclose(fp);
}

void dump_fonts(struct glyph_t *fonts)
{
	int i, j, width;
	uint8_t cell_width, cell_height;

	cell_width = fonts[DEFAULT_CHAR].width;
	cell_height = fonts[DEFAULT_CHAR].height;

	fprintf(stdout,
		"struct static_glyph_t {\n"
		"\tuint8_t width;\n"
		"\tuint%d_t bitmap[%d];\n"
		"};\n\n", ((cell_width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE * 2, cell_height);
	
	fprintf(stdout, "static const uint8_t cell_width = %d, cell_height = %d;\n",
		cell_width, cell_height);

	fprintf(stdout, "static const struct static_glyph_t fonts[UCS2_CHARS] = {\n");
	for (i = 0; i < UCS2_CHARS; i++) {
		width = wcwidth(i);

		if (width <= 0) /* not printable */
			continue;

		if ((fonts[i].bitmap == NULL) /* glyph not found */
			|| (fonts[i].height != cell_height) /* invalid font height */
			|| (fonts[i].width != (cell_width * width))) /* invalid font width */
			continue;

		fprintf(stdout, "[%d] = {%d, {", i, width);
		for (j = 0; j < cell_height; j++)
			fprintf(stdout, "0x%X%s", fonts[i].bitmap[j], (j == (cell_height - 1)) ? "": ", ");
		fprintf(stdout, "}},\n");
	}
	fprintf(stdout, "};\n");
}
