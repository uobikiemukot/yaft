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

/* for yaft original font format: not used

CODE
WIDTH HEIGHT
BITMAP
BITMAP
BITMAP
CODE
WIDTH HEIGHT
....

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
*/

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
		if ((dst >= UCS2_CHARS) || (src >= UCS2_CHARS))
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

void check_fonts(struct glyph_t *fonts)
{
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
}

void dump_fonts(struct glyph_t *fonts)
{
	int i, j, width;
	uint8_t cell_width, cell_height;

	cell_width = fonts[DEFAULT_CHAR].width;
	cell_height = fonts[DEFAULT_CHAR].height;

	fprintf(stdout,
		"struct glyph_t {\n"
		"\tuint32_t code;\n"
		"\tuint8_t width;\n"
		"\tuint%d_t bitmap[%d];\n"
		"};\n\n", ((cell_width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE * 2, cell_height);
	
	fprintf(stdout, "enum {\n\tCELL_WIDTH = %d,\n\tCELL_HEIGHT = %d\n};\n\n",
		cell_width, cell_height);

	fprintf(stdout, "static const struct glyph_t glyphs[] = {\n");
	for (i = 0; i < UCS2_CHARS; i++) {
		width = wcwidth(i);

		if ((width <= 0)                                 /* not printable */
			|| (fonts[i].bitmap == NULL)                 /* glyph not found */
			|| (fonts[i].height != cell_height)          /* invalid font height */
			|| (fonts[i].width != (cell_width * width))) /* invalid font width */
			continue;

		fprintf(stdout, "\t{%d, %d, {", i, width);
		for (j = 0; j < cell_height; j++)
			fprintf(stdout, "0x%X%s", fonts[i].bitmap[j], (j == (cell_height - 1)) ? "": ", ");
		fprintf(stdout, "}},\n");
	}
	fprintf(stdout, "};\n");
}
