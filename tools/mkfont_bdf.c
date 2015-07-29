#include "mkfont_bdf.h"
#include "../conf.h"
#include "util.h"
#include "bdf.h"

/* mkfont_bdf functions */
bool map_glyph(struct glyph_t *font[],
	struct glyph_list_t *glist_head, struct glyph_t *default_glyph)
{
	int width, cell_width = 0, cell_height = 0;
	struct glyph_t *glyph;
	struct glyph_list_t *listp;

	if (default_glyph->width == 0 || default_glyph->height == 0) {
		logging(ERROR, "default glyph(U+%.4X) not found\n", DEFAULT_CHAR);
		return false;
	}

	cell_width  = default_glyph->width;
	cell_height = default_glyph->height;
	logging(DEBUG, "default glyph width:%d height:%d\n",
		default_glyph->width, default_glyph->height);

	for (listp = glist_head; listp != NULL; listp = listp->next) {
		if (listp->code >= UCS2_CHARS)
			continue;

		width = wcwidth(listp->code);
		glyph = listp->glyph;
		if ((width <= 0)                                /* not printable */
			|| (glyph->height != cell_height)           /* invalid font height */
			|| (glyph->width  != (cell_width * width))) /* invalid font width */
			continue;

		font[listp->code] = glyph;
	}
	return true;
}

bool load_alias(struct glyph_t *font[], char *alias)
{
	unsigned int dst, src;
	char buf[BUFSIZE];
	FILE *fp;

	if ((fp = efopen(alias, "r")) == NULL)
		return false;

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		sscanf(buf, "%X %X", &dst, &src);
		if ((dst >= UCS2_CHARS) || (src >= UCS2_CHARS))
			continue;

		//if (font[src] != NULL && font[dst] != NULL) {
		if (font[src] != NULL) {
			logging(DEBUG, "swapped: use U+%.4X for U+%.4X\n", src, dst);
			font[dst] = font[src];

			//free(font[dst]->bitmap);
			//font[dst]->width  = font[src]->width;
			//font[dst]->height = font[src]->height;
			//font[dst]->bitmap = font[src]->bitmap;
			//memcpy(font[dst]->bitmap, font[src]->bitmap, sizeof(bitmap_width_t) * font[src]->height);
		}
	}
	efclose(fp);
	return true;
}

bool check_font(struct glyph_t **font, struct glyph_t *empty_half, struct glyph_t *empty_wide)
{
	empty_half->bitmap = (bitmap_width_t *) ecalloc(font[DEFAULT_CHAR]->height, sizeof(bitmap_width_t));
	empty_wide->bitmap = (bitmap_width_t *) ecalloc(font[DEFAULT_CHAR]->height, sizeof(bitmap_width_t));

	if (!empty_half->bitmap || !empty_wide->bitmap)
		return false;

	empty_half->width  = font[DEFAULT_CHAR]->width;
	empty_wide->width  = font[DEFAULT_CHAR]->width * WIDE;
	empty_half->height = font[DEFAULT_CHAR]->height;
	empty_wide->height = font[DEFAULT_CHAR]->height;

	if (font[SUBSTITUTE_HALF] == NULL) {
		logging(WARN, "half substitute glyph(U+%.4X) not found, use empty glyph\n", SUBSTITUTE_HALF);
		font[SUBSTITUTE_HALF] = empty_half;
	}
	if (font[SUBSTITUTE_WIDE] == NULL) {
		logging(WARN, "wide substitute glyph(U+%.4X) not found, use empty glyph\n", SUBSTITUTE_WIDE);
		font[SUBSTITUTE_WIDE] = empty_wide;
	}
	if (font[REPLACEMENT_CHAR] == NULL) {
		logging(WARN, "replacement glyph(U+%.4X) not found, use empty glyph\n", REPLACEMENT_CHAR);
		font[REPLACEMENT_CHAR] = empty_half;
	}
	return true;
}

bool dump_font(struct glyph_t *font[])
{
	int i, j, width, int_type;
	uint8_t cell_width, cell_height;

	cell_width  = font[DEFAULT_CHAR]->width;
	cell_height = font[DEFAULT_CHAR]->height;

	int_type = my_ceil(cell_width, BITS_PER_BYTE) /* minimum byte for containing half glyph */
		 * 2                                      /* minimum byte for containing wide glyph */
		 * BITS_PER_BYTE;                         /* minimum bits for containing wide glyph */

	/* int_type: 16, 32, 48, 64, 80... */
	if (int_type == 48) { /* uint48_t does not exist */
		int_type = 64;
	} else if (int_type >= 80) {
		logging(ERROR, "BDF width too large (uint%d_t does not exist)\n", int_type);
		return false;
	}

	fprintf(stdout,
		"struct glyph_t {\n"
		"\tuint32_t code;\n"
		"\tuint8_t width;\n"
		"\tuint%d_t bitmap[%d];\n"
		"};\n\n", int_type, cell_height);

	fprintf(stdout, "enum {\n\tCELL_WIDTH = %d,\n\tCELL_HEIGHT = %d\n};\n\n",
		cell_width, cell_height);

	fprintf(stdout, "static const struct glyph_t glyphs[] = {\n");
	for (i = 0; i < UCS2_CHARS; i++) {
		width = wcwidth(i);

		if (font[i] == NULL) /* glyph not found */
			continue;

		fprintf(stdout, "\t{%d, %d, {", i, width);
		for (j = 0; j < cell_height; j++)
			fprintf(stdout, "0x%X%s", (unsigned int) font[i]->bitmap[j], (j == (cell_height - 1)) ? "": ", ");
		fprintf(stdout, "}},\n");
	}
	fprintf(stdout, "};\n");
	return true;
}

void cleanup(struct glyph_list_t *glist_head, struct glyph_t *empty_half, struct glyph_t *empty_wide)
{
	struct glyph_list_t *listp, *next;

	for (listp = glist_head; listp != NULL; listp = next) {
		next = listp->next;

		free(listp->glyph->bitmap);
		free(listp->glyph);
		free(listp);
	}
	free(empty_half->bitmap);
	free(empty_wide->bitmap);
}

int main(int argc, char *argv[])
{
	struct glyph_list_t *glist_head = NULL;
	struct glyph_t *font[UCS2_CHARS], default_glyph, empty_wide, empty_half;

	if (!setlocale(LC_ALL, "")) /* set current locale for wcwidth() */
		logging(WARN, "setlocale() failed\n");

	if (argc < 3) {
		logging(FATAL, "usage: ./mkfont ALIAS BDF1 [BDF2] [BDF3] ...\n");
		return EXIT_FAILURE;
	}

	for (int i = 0; i < UCS2_CHARS; i++)
		font[i] = NULL;

	for (int i = 2; i < argc; i++)
		load_bdf_glyph(&glist_head, &default_glyph, argv[i]);

	if (!map_glyph(font, glist_head, &default_glyph)) {
		logging(FATAL, "map_glyph() failed\n");
		goto err_occured;
	}

	if (!load_alias(font, argv[1]))
		logging(WARN, "font alias does not work\n");

	if (!check_font(font, &empty_half, &empty_wide)) {
		logging(FATAL, "check_font() failed\n");
		goto err_occured;
	}

	if (!dump_font(font)) {
		logging(FATAL, "dump_font() failed\n");
		goto err_occured;
	}

	cleanup(glist_head, &empty_half, &empty_wide);
	return EXIT_SUCCESS;

err_occured:
	cleanup(glist_head, &empty_half, &empty_wide);
	return EXIT_FAILURE;
}
