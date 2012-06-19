void cat_home(char *dst, char *src, int size)
{
	char *home;

	if (src == NULL) {
		fprintf(stderr, "font_path == NULL\n");
		exit(EXIT_FAILURE);
	}

	memset(dst, '\0', size);

	if (DEBUG)
		fprintf(stderr, "src:%s dst:%s size:%d\n", src, dst, size);

	if (src[0] == '~') {
		if ((home = getenv("HOME")) != NULL) {
			strncpy(dst, home, size - 1);
			strncat(dst, src + 1, size - strlen(dst) - 1);
			if (DEBUG)
				fprintf(stderr, "src:%s dst:%s size:%d\n", src, dst, size);
		}
		else
			strncpy(dst, src + 1, size - 1);
	}
	else
		strncpy(dst, src, size - 1);
}

void load_fonts(glyph_t **fonts, char *path)
{
	int i, count = 0, size, state = 0, width, height;
	char buf[BUFSIZE], home[BUFSIZE], *endp;
	FILE *fp;
	u16 code;
	glyph_t *gp;

	cat_home(home, path, BUFSIZE);
	fp = efopen(home, "r");

	for (i = 0; i < UCS_CHARS; i++)
		fonts[i] = NULL;

	while (fgets(buf, BUFSIZE, fp) != NULL) {
		switch (state) {
		case 0:
			code = atoi(buf);
			gp = (glyph_t *) emalloc(sizeof(glyph_t));
			state = 1;
			break;
		case 1:
			sscanf(buf, "%d %d", &gp->size.x, &gp->size.y);
			size = gp->size.x * gp->size.y / BITS_PER_BYTE;
			gp->bitmap = (u8 *) emalloc(size);
			state = 2;
			break;
		case 2:
			gp->bitmap[count++] = strtol(buf, &endp, 16);
			if (count >= size) {
				fonts[code] = gp;
				state = count = 0;
			}
			break;
		default:
			break;
		}
	}

	if (fonts[DEFAULT_CHAR] == NULL) {
		fprintf(stderr, "DEFAULT_CHAR(U+%.2X) not found: copy glyph of SPACE(U+20)\n", DEFAULT_CHAR);
		if (fonts[SPACE] == NULL) {
			fprintf(stderr, "fonts must have either SPACE(U+20) or DEFAULT_CHAR(U+%.2X)\n", DEFAULT_CHAR);
			exit(EXIT_FAILURE);
		}
		gp = (glyph_t *) emalloc(sizeof(glyph_t));
		gp->size.x = fonts[SPACE]->size.x;
		gp->size.y = fonts[SPACE]->size.y;
		size = gp->size.x * gp->size.y / BITS_PER_BYTE;
		gp->bitmap = (u8 *) emalloc(size);
		memcpy(gp->bitmap, fonts[SPACE]->bitmap, size);
		fonts[DEFAULT_CHAR] = gp;
	}

	fclose(fp);
}

u32 *load_wallpaper(int width, int height, char *path)
{
	int i, j, bits, count;
	unsigned char buf[BUFSIZE], home[BUFSIZE], type[3], rgb[3], *cp;
	pair size;
	u32 *ptr, color;
	FILE *fp;

	ptr = (u32 *) emalloc(width * height * sizeof(u32));

	cat_home(home, path, BUFSIZE);
	fp = efopen(home, "r");

	if (fgets(buf, BUFSIZE, fp) != NULL) {
		cp = strchr(buf, '\n');
		*cp = '\0';
		strncpy(type, buf, 3);
	}

	if (fgets(buf, BUFSIZE, fp) != NULL)
		sscanf(buf, "%d %d", &size.x, &size.y);

	if (fgets(buf, BUFSIZE, fp) != NULL)
		bits = atoi(buf);

	if (DEBUG)
		fprintf(stderr, "load_wallpaper type:%s width:%d height:%d bits:%d\n",
			type, size.x, size.y, bits);

	count = 0;
	for (i = 0; i < size.y; i++) {
		for (j = 0; j < size.x; j++) {
			fread(rgb, 1, 3, fp);
			color = (rgb[0] << 16) + (rgb[1] << 8) + (rgb[2]);
			if (i < height && j < width)
				*(ptr + count++) = color;
		}
	}

	fclose(fp);

	return ptr;
}
