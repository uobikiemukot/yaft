/* See LICENSE for licence details. */
/* function for dcs sequence */
enum {
	RGBMAX = 255,
	HUEMAX = 360,
	LSMAX  = 100,
};

inline int sixel_bitmap(struct sixel_canvas_t *sc, uint8_t bitmap)
{
	int i, offset;

	offset = sc->point.x * sizeof(uint32_t) + sc->point.y * sc->line_length;

	if (DEBUG)
		fprintf(stderr, "sixel_bitmap()\nbitmap:%.2X point(%d, %d)\n",
			bitmap, sc->point.x, sc->point.y);

	for (i = 0; i < BITS_PER_SIXEL; i++) {
		if (bitmap & (0x01 << i))
			memcpy(sc->data + offset, &sc->color_table[sc->color_index], sizeof(uint32_t));
		offset += sc->line_length;
	}
	sc->point.x++;

	if (sc->point.x > sc->width)
		sc->width = sc->point.x;

	return 1;
}

inline int sixel_repeat(struct sixel_canvas_t *sc, char *buf)
{
	int i, count;
	size_t length;
	char *cp, tmp[BUFSIZE];
	uint8_t bitmap;

	cp = buf + 1; /* skip '!' itself */
	while (isdigit(*cp)) /* skip non sixel bitmap character */
		cp++;

	length = (cp - buf);
	strncpy(tmp, buf + 1, length - 1);
	*(tmp + length - 1) = '\0';

	count = dec2num(tmp);

	if (DEBUG)
		fprintf(stderr, "sixel_repeat()\nbuf:%s length:%u\ncount:%d repeat:0x%.2X\n",
			tmp, (unsigned) length, count, *cp);

	if ('?' <= *cp && *cp <= '~') {
		bitmap = bit_mask[BITS_PER_SIXEL] & (*cp - '?');
		for (i = 0; i < count; i++)
			sixel_bitmap(sc, bitmap);
	}

	return length + 1;
}

inline int sixel_attr(struct sixel_canvas_t *sc, char *buf)
{
	char *cp, tmp[BUFSIZE];
	size_t length;
	struct parm_t parm;

	cp = buf + 1;
	while (isdigit(*cp) || *cp == ';') /* skip valid params */
		cp++;

	length = (cp - buf);
	strncpy(tmp, buf + 1, length - 1);
	*(tmp + length - 1) = '\0';

	reset_parm(&parm);
	parse_arg(tmp, &parm, ';', isdigit);

	if (DEBUG)
		fprintf(stderr, "sixel_attr()\nbuf:%s\nargc:%d\n", tmp, parm.argc);

	if (parm.argc >= 2)
		sc->ratio = dec2num(parm.argv[0]) / dec2num(parm.argv[1]);

	if (parm.argc >= 4) {
		sc->width  = dec2num(parm.argv[2]);
		sc->height = dec2num(parm.argv[3]);
	}

	if (DEBUG)
		fprintf(stderr, "ratio:%d width:%d height:%d\n", sc->ratio, sc->width, sc->height);

	return length;
}

inline uint32_t hue2rgb(int n1, int n2, int hue)
{
	if (hue < 0)
		hue += HUEMAX;

	if (hue > HUEMAX)
		hue -= HUEMAX;

	if (hue < (HUEMAX / 6))
		return (n1 + (((n2 - n1) * hue + (HUEMAX / 12)) / (HUEMAX / 6)));
	if (hue < (HUEMAX / 2))
		return n2;
	if (hue < ((HUEMAX * 2) / 3))
		return (n1 + (((n2 - n1) * (((HUEMAX * 2) / 3) - hue) + (HUEMAX / 12)) / (HUEMAX / 6)));
	else
		return n1;
}

inline uint32_t hls2rgb(int hue, int lum, int sat)
{
	uint32_t r, g, b;
	int magic1, magic2;

	if (sat == 0) {
		r = g = b = (lum * RGBMAX) / LSMAX;
	}
	else {
		if (lum <= (LSMAX / 2) )
			magic2 = (lum * (LSMAX + sat) + (LSMAX / 2)) / LSMAX;
		else
			magic2 = lum + sat - ((lum * sat) + (LSMAX / 2)) / LSMAX;
		magic1 = 2 * lum - magic2;

		r = (hue2rgb(magic1, magic2, hue + (HUEMAX / 3)) * RGBMAX + (LSMAX / 2)) / LSMAX;
		g = (hue2rgb(magic1, magic2, hue) * RGBMAX + (LSMAX / 2)) / LSMAX;
		b = (hue2rgb(magic1, magic2, hue - (HUEMAX / 3)) * RGBMAX + (LSMAX/2)) / LSMAX;
	}
	return (r << 16) + (g << 8) + b;
}

inline int sixel_color(struct sixel_canvas_t *sc, char *buf)
{
	char *cp, tmp[BUFSIZE];
	int index, type;
	size_t length;
	uint16_t v1, v2, v3, r, g, b;
	uint32_t color;
	struct parm_t parm;

	cp = buf + 1;
	while (isdigit(*cp) || *cp == ';') /* skip valid params */
		cp++;

	length = (cp - buf);
	strncpy(tmp, buf + 1, length - 1); /* skip '#' */
	*(tmp + length - 1) = '\0';

	if (DEBUG)
		fprintf(stderr, "sixel_color()\nbuf:%s length:%u\n", tmp, (unsigned) length);

	reset_parm(&parm);
	parse_arg(tmp, &parm, ';', isdigit);

	if (DEBUG)
		fprintf(stderr, "argc:%d\n", parm.argc);

	if (parm.argc == 1) { /* select color */
		index = dec2num(parm.argv[0]);
		sc->color_index = index;

		if (DEBUG)
			fprintf(stderr, "select color:%d\n", index);

		return length;
	}

	if (parm.argc != 5)
		return length;

	index = dec2num(parm.argv[0]);
	type  = dec2num(parm.argv[1]);
	v1    = dec2num(parm.argv[2]);
	v2    = dec2num(parm.argv[3]);
	v3    = dec2num(parm.argv[4]);

	if (DEBUG)
		fprintf(stderr, "index:%d type:%d v1:%u v2:%u v3:%u\n",
			index, type, v1, v2, v3);

	if (type == 1) /* HLS */
		color = hls2rgb(v1, v2, v3);
	else {
		r = bit_mask[8] & (0xFF * v1 / 100);
		g = bit_mask[8] & (0xFF * v2 / 100);
		b = bit_mask[8] & (0xFF * v3 / 100);
		color = (r << 16) | (g << 8) | b;
	}

	if (DEBUG)
		fprintf(stderr, "color:0x%.8X\n", color);

	sc->color_table[index] = color;

	return length;
}

inline int sixel_cr(struct sixel_canvas_t *sc)
{
	if (DEBUG)
		fprintf(stderr, "sixel_cr()\n");

	sc->point.x = 0;

	return 1;
}

inline int sixel_nl(struct sixel_canvas_t *sc)
{
	if (DEBUG)
		fprintf(stderr, "sixel_nl()\n");

	/* DECGNL moves active position to left margin
		and down one line of sixels:
		http://odl.sysworks.biz/disk$vaxdocdec963/decw$book/d3qsaaa1.p67.decw$book */
	sc->point.y += BITS_PER_SIXEL;
	sc->point.x = 0;

	if (sc->point.y > sc->height)
		sc->height = sc->point.y;

	return 1;
}

void sixel_parse_data(struct sixel_canvas_t *sc, char *start_buf)
{
	/*
	DECDLD sixel data
		'$': carriage return
		'-': new line
		'#': color
			# Pc: select color
			# Pc; Pu; Px; Py; Pz
				Pc : color index (0 to 255)
				Pu : color coordinate system
					1: HLS (0 to 360 for Hue, 0 to 100 for others)
					2: RGB (0 to 100 percent) (default)
				Px : Hue        / Red
				Py : Lightness  / Green
				Pz : Saturation / Blue
		'"': attr
			" Pan; Pad; Ph; Pv
				Pan, Pad: defines aspect ratio (Pan / Pad) (ignored)
				Ph, Pv  : defines vertical/horizontal size of the image
		'!': repeat
			! Pn ch
				Pn : repeat count ([0-9]+)
				ch : character to repeat ('?' to '~')
		sixel bitmap:
			range of ? (hex 3F) to ~ (hex 7E)
			? (hex 3F) represents the binary value 00 0000.
			t (hex 74) represents the binary value 11 0101.
			~ (hex 7E) represents the binary value 11 1111.
	*/
	int size = 0;
	char *cp, *end_buf;
	uint8_t bitmap;

	if (DEBUG)
		fprintf(stderr, "sixel_parse_data()\n");

	cp = start_buf;
	end_buf = cp + strlen(start_buf);

	while (cp < end_buf) {
		if (*cp == '!')
			size = sixel_repeat(sc, cp);
		else if (*cp == '"')
			size = sixel_attr(sc, cp);
		else if (*cp == '#')
			size = sixel_color(sc, cp);
		else if (*cp == '$')
			size = sixel_cr(sc);
		else if (*cp == '-')
			size = sixel_nl(sc);
		else if ('?' <= *cp && *cp <= '~')  {
			bitmap =  bit_mask[BITS_PER_SIXEL] & (*cp - '?');
			size = sixel_bitmap(sc, bitmap);
		}
		else if (*cp == '\0') /* end of sixel data */
			break;
		else
			//fprintf(stderr, "unknown sixel data:0x%.2X\n", *cp);
			size = 1;
		cp += size;
	}
}

void sixel_parse_header(struct terminal *term, char *start_buf)
{
	/*
	sixel format
		DSC P1; P2; P3; q; s...s; ST
	parameters
		DCS: ESC(0x1B) P (0x50) (8bit C1 character not recognized)
		P1 : pixel aspect ratio (force 0, 2:1) (TODO: not implemented)
		P2 : background mode (TODO: not implemented)
			0 or 2: (default) 0 stdands for current background color
			1     : 0 stands for remaining current color
		P3 : horizontal grid parameter (ignored)
		q  : final character of sixel sequence
		s  : see parse_sixel_data()
		ST : ESC (0x1B) '\' (0x5C) or BEL (0x07)
	*/
	//int background_mode;
	char *cp;
	struct parm_t parm;
	struct sixel_canvas_t *sc;

	/* replace final char of sixel header by NUL '\0' */
	cp = strchr(start_buf, 'q');
	*cp = '\0';

	if (DEBUG)
		fprintf(stderr, "sixel_parse_header()\nbuf:%s\n", start_buf);

	/* split header by semicolon ';' */
	reset_parm(&parm);
	parse_arg(start_buf, &parm, ';', isdigit);

	/* set params */
	/*
	if (parm.argc != 3)
		background_mode = 0;
	else
		background_mode = dec2num(parm.argv[1]);
	*/

	/* remove previous canvas data */
	sc = &term->sixel_canvas[term->sixel_canvas_num];
	if (sc->data != NULL)
		free(sc->data);
	reset_sixel(sc);

	/* set canvas parameters */
	sc->data        = (unsigned char *) ecalloc(term->width * term->height, sizeof(uint32_t));
	sc->line_length = term->width * sizeof(uint32_t);
	sc->ref_cell    = term->cursor;

	if (DEBUG)
		fprintf(stderr, "ref_cell(%d, %d) sixel_canvas_num:%d\n",
			sc->ref_cell.x, sc->ref_cell.y, term->sixel_canvas_num);

	sixel_parse_data(sc, cp + 1); /* skip ';' after 'q' */
	/* TODO: shrink image buffer */
	if (term->mode & MODE_SIXSCR)
		move_cursor(term, my_ceil(sc->height + 1, CELL_HEIGHT), 0);
	term->sixel_canvas_num = (term->sixel_canvas_num + 1) % MAX_SIXEL_CANVAS;

	if (DEBUG)
		fprintf(stderr, "width:%d height:%d\n", sc->width, sc->height);
}

inline void decdld_bitmap(struct glyph_t *glyph, uint8_t bitmap, uint8_t row, uint8_t column)
{
	/*
			  MSB        LSB (glyph_t bitmap order, padding at MSB side)
				  -> column
	sixel bit0 ->........
	sixel bit1 ->........
	sixel bit2 ->....@@..
	sixel bit3 ->...@..@.
	sixel bit4 ->...@....
	sixel bit5 ->...@....
				 .@@@@@..
				 ...@....
				|...@....
			row |...@....
				v...@....
				 ...@....
				 ...@....
				 ...@....
				 ........
				 ........
	*/
	int i, height_shift, width_shift;

	if (DEBUG)
		fprintf(stderr, "bit pattern:0x%.2X\n", bitmap);

	width_shift = CELL_WIDTH - 1 - column;
	if (width_shift < 0)
		return;

	for (i = 0; i < BITS_PER_SIXEL; i++) {
		if((bitmap >> i) & 0x01) {
			height_shift = row * BITS_PER_SIXEL + i;

			if (height_shift < CELL_HEIGHT) {
				if (DEBUG)
					fprintf(stderr, "height_shift:%d width_shift:%d\n", height_shift, width_shift);
				glyph->bitmap[height_shift] |= bit_mask[CELL_WIDTH] & (0x01 << width_shift);
			}
		}
	}
}

inline void init_glyph(struct glyph_t *glyph)
{
	int i;

	glyph->width = 1; /* drcs glyph must be HALF */
	glyph->code = 0;  /* this value not used: drcs call by DRCSMMv1 */

	for (i = 0; i < CELL_HEIGHT; i++)
		glyph->bitmap[i] = 0;
}

void decdld_parse_data(char *start_buf, int start_char, struct glyph_t *chars)
{
	/*
	DECDLD sixel data
		';': glyph separator
		'/': line feed
		sixel bitmap:
			range of ? (hex 3F) to ~ (hex 7E)
			? (hex 3F) represents the binary value 00 0000.
			t (hex 74) represents the binary value 11 0101.
			~ (hex 7E) represents the binary value 11 1111.
	*/
	char *cp, *end_buf;
	uint8_t char_num = start_char; /* start_char == 0 means SPACE(0x20) */
	uint8_t bitmap, row = 0, column = 0;
	//struct glyph_t *chars;

	/* TODO: must be selectable 94 or 96 here */
	//chars = (struct glyph_t *) ecalloc(sizeof(struct glyph_t), GLYPH_PER_CHARSET);
	init_glyph(&chars[char_num]);

	cp      = start_buf;
	end_buf = cp + strlen(cp);
	while (cp < end_buf) {
		if ('?' <= *cp && *cp <= '~') { /* sixel bitmap */
			if (DEBUG)
				fprintf(stderr, "char_num(ten):0x%.2X\n", char_num);
            /* remove offset '?' and use only 6bit */
			bitmap = bit_mask[BITS_PER_SIXEL] & (*cp - '?');
			decdld_bitmap(&chars[char_num], bitmap, row, column);
			column++;
		}
		else if (*cp == ';') {          /* next char */
			row = column = 0;
			char_num++;
			init_glyph(&chars[char_num]);
		}
		else if (*cp == '/') {          /* sixel nl+cr */
			row++;
			column = 0;
		}
		else if (*cp == '\0')           /* end of DECDLD sequence */
			break;
		/*
		else
			fprintf(stderr, "unknown decdld data '%.2X'\n", *cp);
		*/
		cp++;
	}
	//return chars;
}

void decdld_parse_header(struct terminal *term, char *start_buf)
{
	/*
	DECDLD format
		DCS Pfn; Pcn; Pe; Pcmw; Pss; Pt; Pcmh; Pcss; f Dscs Sxbp1 ; Sxbp2 ; .. .; Sxbpn ST
	parameters
		DCS : ESC (0x1B) 'P' (0x50) (DCS(8bit C1 code) is not supported)
		Pfn : fontset (ignored)
		Pcn : start char (0 means SPACE 0x20)
		Pe  : erase mode (force 0)
				0: clear selectet charset
				1: clear only redefined glyph
				2: clear all drcs charset
		Pcmw: max cellwidth (force CELL_WEDTH defined in glyph.h)
		Pss : screen size (ignored)
		Pt  : defines the glyph as text or full cell or sixel (force full cell mode)
		      (TODO: implement sixel/text mode)
		Pcmh: max cellheight (force CELL_HEIGHT defined in glyph.h)
		Pcss: character set size (force: 96)
				0: 94 gylphs charset
				1: 96 gylphs charset
		f   : '{' (0x7B)
		Dscs: define character set
				Intermediate char: SPACE (0x20) to '/' (0x2F)
				final char       : '0' (0x30) to '~' (0x7E)
									but allow chars between '@' (0x40) and '~' (0x7E) for DRCSMMv1
									(ref: https://github.com/saitoha/drcsterm/blob/master/README.rst)
		Sxbp: see parse_decdld_sixel()
		ST  : ESC (0x1B) '\' (0x5C) or BEL (0x07)
	*/
	char *cp;
	int i, start_char, erase_mode, charset;
	struct parm_t parm;

	/* replace final char of DECDLD header by NUL '\0' */
	cp = strchr(start_buf, '{');
	*cp = '\0';

	if (DEBUG)
		fprintf(stderr, "decdld_parse_header()\nbuf:%s\n", start_buf);

	/* split header by semicolon ';' */
	reset_parm(&parm);
	parse_arg(start_buf, &parm, ';', isdigit);

	if (parm.argc != 8) /* DECDLD header must have 8 params */
		return;

	/* set params */
	start_char = dec2num(parm.argv[1]);
	erase_mode = dec2num(parm.argv[2]);

	/* parse Dscs */
	cp++; /* skip final char (NUL) of DECDLD header */
	while (SPACE <= *cp && *cp <= '/') /* skip intermediate char */
		cp++;

	if (0x40 <= *cp && *cp <= 0x7E) /* final char of Dscs must be between 0x40 to 0x7E (DRCSMMv1) */
		charset = *cp - 0x40; 
	else
		charset = 0;

	if (DEBUG)
		fprintf(stderr, "charset(ku):0x%.2X start_char:%d erase_mode:%d\n",
			charset, start_char, erase_mode);

	/* reset previous glyph data */
	if (erase_mode < 0 || erase_mode > 2)
		erase_mode = 0;

	if (erase_mode == 2) {      /* reset all drcs charset */
		for (i = 0; i < DRCS_CHARSETS; i++) {
			if (term->drcs[i] != NULL) {
				free(term->drcs[i]);
				term->drcs[i] = NULL;
			}
		}
	}
	else if (erase_mode == 0) { /* reset selected drcs charset */
		if (term->drcs[charset] != NULL) { /* TODO: check erase mode */
			free(term->drcs[charset]);
			term->drcs[charset] = NULL;
		}
	}

	if (term->drcs[charset] == NULL)
		term->drcs[charset] = ecalloc(GLYPH_PER_CHARSET, sizeof(struct glyph_t));

	decdld_parse_data(cp + 1, start_char, term->drcs[charset]); /* skil final char */
}
