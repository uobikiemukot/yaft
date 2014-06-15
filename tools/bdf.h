int pre_match(const char *buf, const char *str) {
	return !strncmp(buf, str, strlen(str));
}

void print_bitmap(uint32_t *bitmap)
{
	unsigned int i, j;

	for (i = 0; i < MAX_HEIGHT; i++) {
		for (j = 1; j <= sizeof(uint32_t) * BITS_PER_BYTE; j++) {
			if (bitmap[i] & (1 << (sizeof(uint32_t) * BITS_PER_BYTE - j)))
				fprintf(stderr, "@");
			else
				fprintf(stderr, ".");
		}
		fprintf(stderr, "\n");
	}
}

int bit2byte(int bits)
{
	return (bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
}

void shift_glyph(struct bdf_t *bdf, struct bdf_glyph_t *glyph)
{
	int i, byte, shift;
	uint32_t tmp;

	for (i = 0; i < glyph->bbh; i++) {
		tmp = glyph->bitmap[i];
		byte = bit2byte(glyph->bbw);
		tmp <<= (bit2byte(glyph->dwidth) - byte) * BITS_PER_BYTE;
		if (glyph->bbx >= 0)
			tmp >>= glyph->bbx;
		else {
			fprintf(stderr, "maybe overlapping (glpyh:%X bbx:%d)\n", glyph->encoding, glyph->bbx);
			tmp <<= abs(glyph->bbx);
		}
		glyph->bitmap[i] = tmp;
	}

	shift = bdf->ascent - (glyph->bbh + glyph->bby);
	if (shift >= 0) {
		memmove(glyph->bitmap + shift, glyph->bitmap, sizeof(uint32_t) * glyph->bbh);
		memset(glyph->bitmap, 0, sizeof(uint32_t) * shift);
	}
	else {
		fprintf(stderr, "maybe overlapping (glpyh:%X vertical shift:%d)\n", glyph->encoding, shift);
		memmove(glyph->bitmap, glyph->bitmap + abs(shift), sizeof(uint32_t) * glyph->bbh);
	}
}

void load_table(char *path, enum encode_t encode)
{
	int i;
	char buf[BUFSIZE], *cp;
	FILE *fp;
	uint32_t from, to;

	fp = efopen(path, "r");

	for (i = 0; i < UCS2_CHARS; i++) {
		if (encode == ISO10646)
			convert_table[i] = i;
		else
			convert_table[i] = -1;
	}

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if (strlen(buf) == 0 || buf[0] == '#')
			continue;

		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';

		sscanf(buf, "%X %X", &from, &to);
		convert_table[from] = to;
	}
}

int read_header(char *buf, struct bdf_t *bdf)
{
	char *cp;

	if (pre_match(buf, "FONTBOUNDINGBOX "))
		sscanf(buf + strlen("FONTBOUNDINGBOX "), "%d %d %d %d",
			&bdf->bbw, &bdf->bbh, &bdf->bbx, &bdf->bby);
	else if (pre_match(buf, "FONT_ASCENT "))
		bdf->ascent = atoi(buf + strlen("FONT_ASCENT "));
	else if (pre_match(buf, "FONT_DESCENT "))
		bdf->descent = atoi(buf + strlen("FONT_DESCENT "));
	else if (pre_match(buf, "DEFAULT_CHAR "))
		bdf->default_char = atoi(buf + strlen("DEFAULT_CHAR "));
	else if (pre_match(buf, "PIXEL_SIZE "))
		bdf->pixel_size = atoi(buf + strlen("PIXEL_SIZE "));
	else if (pre_match(buf, "CHARSET_REGISTRY ")) {
		strncpy(bdf->charset, buf + strlen("CHARSET_REGISTRY "), BUFSIZE - 1);
		fprintf(stderr, "%s\n", bdf->charset);
		for (cp = bdf->charset; *cp != '\0'; cp++)
			*cp = toupper(*cp);

		if (strstr(bdf->charset, "X68000") != NULL || strstr(bdf->charset, "x68000") != NULL)
			load_table("./table/X68000", X68000);
		else if (strstr(bdf->charset, "JISX0201") != NULL || strstr(bdf->charset, "jisx0201") != NULL)
			load_table("./table/JISX0201", JISX0201);
		else if (strstr(bdf->charset, "JISX0208") != NULL || strstr(bdf->charset, "jisx0208") != NULL)
			load_table("./table/JISX0208", JISX0208);
		else if (strstr(bdf->charset, "ISO8859") != NULL || strstr(bdf->charset, "iso8859") != NULL)
			load_table("./table/ISO8859", ISO8859);
		else /* assume "ISO10646" */
			load_table("./table/ISO10646", ISO10646);
	}
	else if (pre_match(buf, "CHARS ")) {
		bdf->chars =  atoi(buf + strlen("CHARS "));
		return BDF_CHAR;
	}

	return BDF_HEADER;
}

int read_char(char *buf, struct bdf_glyph_t *glyph)
{
	if (pre_match(buf, "BBX "))
		sscanf(buf + strlen("BBX "), "%d %d %d %d", &glyph->bbw, &glyph->bbh, &glyph->bbx, &glyph->bby);
	else if (pre_match(buf, "DWIDTH "))
		glyph->dwidth = atoi(buf + strlen("DWIDTH "));
	else if (pre_match(buf, "ENCODING ")) {
		glyph->encoding = atoi(buf + strlen("ENCODING "));
		//fprintf(stderr, "reading char:%d\n", glyph->encoding);
	}
	else if (pre_match(buf, "BITMAP")) {
		return BDF_BITMAP;
	}

	return BDF_CHAR;
}

int read_bitmap(struct glyph_t *fonts, char *buf, struct bdf_t *bdf, struct bdf_glyph_t *glyph)
{
	int i;
	uint32_t code;
	uint8_t width, height;

	static int count = 0;

	if (pre_match(buf, "ENDCHAR")) {
		shift_glyph(bdf, glyph);
		count = 0;

		code = convert_table[glyph->encoding];
		width = glyph->dwidth;
		height = bdf->ascent + bdf->descent;

		//fprintf(stderr, "code:%d width:%d height:%d\n", code, width, height);

		if (code < UCS2_CHARS) {
			fonts[code].width = width;
			fonts[code].height = height;
			fonts[code].bitmap = (uint32_t *) emalloc(sizeof(uint32_t) * fonts[code].height);

			for (i = 0; i < fonts[code].height; i++)
				fonts[code].bitmap[i] = glyph->bitmap[i];
		}
		memset(glyph, 0, sizeof(struct bdf_glyph_t));
		return BDF_CHAR;
	}
	else
		sscanf(buf, "%X", &glyph->bitmap[count++]);

	return BDF_BITMAP;
}

void load_bdf_glyph(struct glyph_t *fonts, char *path)
{
	int mode = BDF_HEADER;
	char buf[BUFSIZE], *cp;
	FILE *fp;
	struct bdf_t bdf;
	struct bdf_glyph_t glyph;

	fp = efopen(path, "r");

	memset(&bdf, 0, sizeof(struct bdf_t));
	memset(&glyph, 0, sizeof(struct bdf_glyph_t));

	fprintf(stderr, "read bdf: %s\n", path);

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		if ((cp = strchr(buf, '\n')) != NULL)
			*cp = '\0';

		//fprintf(stderr, "%s\n", buf);

		if (mode == BDF_HEADER)
			mode = read_header(buf, &bdf);
		else if (mode == BDF_CHAR)
			mode = read_char(buf, &glyph);
		else if (mode == BDF_BITMAP)
			mode = read_bitmap(fonts, buf, &bdf, &glyph);
	}
}
