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
		fprintf(stderr, "fonts must have DEFAULT_CHAR(U+%.2X)\n", DEFAULT_CHAR);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
}

u32 *load_wallpaper(framebuffer *fb, int width, int height)
{
	int i, j, count = 0;
	u32 *ptr;

	ptr = (u32 *) emalloc(width * height * sizeof(u32));

	for (i = 0; i < fb->res.y; i++) {
		for (j = 0; j < fb->res.x; j++) {
			if (i < height && j < width)
				*(ptr + count++) = *(fb->fp + j + i * fb->line_length);
		}
	}

	return ptr;
}
