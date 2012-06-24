/* See LICENSE for licence details. */
void load_fonts(glyph_t **fonts, char *path)
{
	int i, count = 0, state = 0;
	char buf[BUFSIZE], *endp;
	FILE *fp;
	u16 code = DEFAULT_CHAR;
	glyph_t *gp = NULL;

	fp = efopen(path, "r");

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
			gp->bitmap = (u32 *) emalloc(gp->size.y * sizeof(u32));
			state = 2;
			break;
		case 2:
			gp->bitmap[count++] = strtol(buf, &endp, 16);
			if (count >= gp->size.y) {
				fonts[code] = gp;
				state = count = 0;
			}
			break;
		default:
			break;
		}
	}

	gp = fonts[DEFAULT_CHAR];
	if (gp == NULL || gp->size.x == 0 || gp->size.y == 0) {
		fprintf(stderr, "DEFAULT_CHAR(U+%.2X) not found or invalid cell size x:%d y:%d\n",
				DEFAULT_CHAR, gp->size.x, gp->size.y);
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
