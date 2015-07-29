int pre_match(const char *buf, const char *str) {
	return !strncmp(buf, str, strlen(str));
}

int bit2byte(int bits)
{
	return (bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
}

void shift_glyph(struct bdf_header_t *bdf_header, struct bdf_char_t *bdf_char)
{
	int i, byte, shift;
	bitmap_width_t bitmap;

	for (i = 0; i < bdf_char->bbh; i++) {
		bitmap  = bdf_char->bitmap[i];
		byte    = bit2byte(bdf_char->bbw);
		bitmap <<= (bit2byte(bdf_char->dwidth) - byte) * BITS_PER_BYTE;
		if (bdf_char->bbx >= 0)
			bitmap >>= bdf_char->bbx;
		else {
			logging(ERROR, "maybe overlapping (glpyh:%X bbx:%d)\n", bdf_char->encoding, bdf_char->bbx);
			bitmap <<= abs(bdf_char->bbx);
		}
		bdf_char->bitmap[i] = bitmap;
	}

	shift = bdf_header->ascent - (bdf_char->bbh + bdf_char->bby);
	if (shift >= 0) {
		memmove(bdf_char->bitmap + shift, bdf_char->bitmap, sizeof(bitmap_width_t) * bdf_char->bbh);
		memset(bdf_char->bitmap, 0, sizeof(bitmap_width_t) * shift);
	}
	else {
		logging(ERROR, "maybe overlapping (glpyh:%X vertical shift:%d)\n", bdf_char->encoding, shift);
		memmove(bdf_char->bitmap, bdf_char->bitmap + abs(shift), sizeof(bitmap_width_t) * bdf_char->bbh);
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

		sscanf(buf, "%X %X", (unsigned int *) &from, (unsigned int *) &to);
		convert_table[from] = to;
	}
}

int read_header(char *buf, struct bdf_header_t *bdf_header)
{
	char *cp;

	if (pre_match(buf, "FONTBOUNDINGBOX "))
		sscanf(buf + strlen("FONTBOUNDINGBOX "), "%d %d %d %d",
			&bdf_header->bbw, &bdf_header->bbh, &bdf_header->bbx, &bdf_header->bby);
	else if (pre_match(buf, "FONT_ASCENT "))
		bdf_header->ascent = atoi(buf + strlen("FONT_ASCENT "));
	else if (pre_match(buf, "FONT_DESCENT "))
		bdf_header->descent = atoi(buf + strlen("FONT_DESCENT "));
	else if (pre_match(buf, "DEFAULT_CHAR "))
		bdf_header->default_char = atoi(buf + strlen("DEFAULT_CHAR "));
	else if (pre_match(buf, "PIXEL_SIZE "))
		bdf_header->pixel_size = atoi(buf + strlen("PIXEL_SIZE "));
	else if (pre_match(buf, "CHARSET_REGISTRY ")) {
		strncpy(bdf_header->charset, buf + strlen("CHARSET_REGISTRY "), BUFSIZE - 1);
		logging(DEBUG, "%s\n", bdf_header->charset);

		for (cp = bdf_header->charset; *cp != '\0'; cp++)
			*cp = (char) toupper((int) *cp);

		if (strstr(bdf_header->charset, "X68000") != NULL
			|| strstr(bdf_header->charset, "x68000") != NULL)
			load_table("./table/X68000", X68000);
		else if (strstr(bdf_header->charset, "JISX0201") != NULL
			|| strstr(bdf_header->charset, "jisx0201") != NULL)
			load_table("./table/JISX0201", JISX0201);
		else if (strstr(bdf_header->charset, "JISX0208") != NULL
			|| strstr(bdf_header->charset, "jisx0208") != NULL)
			load_table("./table/JISX0208", JISX0208);
		else if (strstr(bdf_header->charset, "ISO8859") != NULL
			|| strstr(bdf_header->charset, "iso8859") != NULL)
			load_table("./table/ISO8859", ISO8859);
		else /* assume "ISO10646" */
			load_table("./table/ISO10646", ISO10646);
	}
	else if (pre_match(buf, "CHARS ")) {
		bdf_header->chars =  atoi(buf + strlen("CHARS "));
		return BDF_CHAR;
	}

	return BDF_HEADER;
}

int read_char(char *buf, struct bdf_char_t *bdf_char)
{
	if (pre_match(buf, "BBX "))
		sscanf(buf + strlen("BBX "), "%d %d %d %d",
			&bdf_char->bbw, &bdf_char->bbh, &bdf_char->bbx, &bdf_char->bby);
	else if (pre_match(buf, "DWIDTH "))
		bdf_char->dwidth = atoi(buf + strlen("DWIDTH "));
	else if (pre_match(buf, "ENCODING ")) {
		bdf_char->encoding = atoi(buf + strlen("ENCODING "));
		//logging(DEBUG, "reading char:%d\n", bdf_char->encoding);
	}
	else if (pre_match(buf, "BITMAP")) {
		return BDF_BITMAP;
	}

	return BDF_CHAR;
}

int read_bitmap(struct glyph_list_t **glist_head, struct glyph_t *default_glyph,
	char *buf, struct bdf_header_t *bdf_header, struct bdf_char_t *bdf_char)
{
	static int count = 0;
	uint32_t code;
	uint8_t width, height;
	struct glyph_list_t *listp;
	struct glyph_t *glyph;

	if (pre_match(buf, "ENDCHAR")) {
		count = 0;

		shift_glyph(bdf_header, bdf_char);

		if (bdf_char->encoding < 0 || bdf_char->encoding >= UCS2_CHARS)
			return BDF_CHAR;

		code   = convert_table[bdf_char->encoding];
		width  = bdf_char->dwidth;
		height = bdf_header->ascent + bdf_header->descent;

		//logging(DEBUG, "code:%d width:%d height:%d\n", code, width, height);

		if (code < UCS2_CHARS && width <= 64) {
			listp = ecalloc(1, sizeof(struct glyph_list_t));
			glyph = ecalloc(1, sizeof(struct glyph_t));

			glyph->width  = width;
			glyph->height = height;
			/* XXX: max width 64 pixels (wide char)  */
			glyph->bitmap = (bitmap_width_t *) ecalloc(glyph->height, sizeof(bitmap_width_t));

			for (int i = 0; i < glyph->height; i++)
				glyph->bitmap[i] = bdf_char->bitmap[i];

			/* add new element to glyph list */
			listp->code  = code;
			listp->glyph = glyph;
			listp->next  = *glist_head;
			*glist_head  = listp;

			if (code == DEFAULT_CHAR)
				*default_glyph = *glyph;
		}
		memset(bdf_char, 0, sizeof(struct bdf_char_t));
		return BDF_CHAR;
	}
	else
		sscanf(buf, "%X", (unsigned int *) &bdf_char->bitmap[count++]);

	return BDF_BITMAP;
}

bool load_bdf_glyph(struct glyph_list_t **glist_head, struct glyph_t *default_glyph, char *path)
{
	int mode = BDF_HEADER;
	char lbuf[BUFSIZE], *cp;
	FILE *fp;
	struct bdf_header_t bdf_header;
	struct bdf_char_t bdf_char;

	if ((fp = efopen(path, "r")) == NULL)
		return false;

	memset(&bdf_header, 0, sizeof(struct bdf_header_t));
	memset(&bdf_char, 0, sizeof(struct bdf_char_t));

	logging(DEBUG, "read bdf: %s\n", path);

	while (fgets(lbuf, BUFSIZE, fp) != NULL) {
		if ((cp = strchr(lbuf, '\n')) != NULL)
			*cp = '\0';

		//fprintf(stderr, "%s\n", lbuf);

		if (mode == BDF_HEADER)
			mode = read_header(lbuf, &bdf_header);
		else if (mode == BDF_CHAR)
			mode = read_char(lbuf, &bdf_char);
		else if (mode == BDF_BITMAP)
			mode = read_bitmap(glist_head, default_glyph, lbuf, &bdf_header, &bdf_char);
	}
	efclose(fp);
	return true;
}
