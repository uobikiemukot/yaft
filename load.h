void load_fonts(glyph_t **fonts, char *path)
{
	int i, count = 0, size, state = 0, width, height;
	char buf[BUFSIZE], *endp;
	FILE *fp;
	u16 code;
	glyph_t *gp;

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

	fclose(fp);
}

u32 *load_wallpaper(int width, int height)
{
	int i, j, bits, count;
	unsigned char buf[BUFSIZE], type[3], rgb[3], *cp;
	pair size;
	u32 *ptr, color;
	FILE *fp;

	ptr = (u32 *) emalloc(width * height * sizeof(u32));
	fp = efopen(wall_path, "r");

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
		fprintf(stderr, "load_wallpaper type:%s width:%d height:%d bits:%d\n", type, size.x, size.y, bits);

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
