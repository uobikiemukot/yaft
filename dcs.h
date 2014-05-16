/* See LICENSE for licence details. */
/* function for dcs sequence */
void parse_sixel_header(struct terminal *term)
{
	struct esc_t *esc = &term->esc;
	char *cp;

	/* replace final char of sixel header by NUL '\0' */
	cp = strchr(esc->buf, 'q');
	*cp = '\0';
}

void set_bitmap(struct static_glyph_t *glyph, uint8_t bitmap, uint8_t row, uint8_t column)
{
	/*
		          MSB        LSB (static_glyph_t bitmap order)
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

void parse_sixel_data(struct terminal *term, struct drcs_t *drcs, char *start_buf)
{
	/* sixel data
		';' glyph separator
		'/' line feed
		sixel bitmap:
			range of ? (hex 3F) to ~ (hex 7E)
			? (hex 3F) represents the binary value 00 0000.
			t (hex 74) represents the binary value 11 0101.
			~ (hex 7E) represents the binary value 11 1111.
	*/
	int i, j;
	uint8_t char_num = drcs->start_char, /* start_char == 0 means SPACE(0x20) */
		bitmap, row = 0, column = 0;
	char *cp, *end_buf;

	/* allocate memory */
	if (drcs->chars == NULL)
		drcs->chars = (struct static_glyph_t *)
			ecalloc(sizeof(struct static_glyph_t) * GLYPH_PER_CHARSET); /* TODO: must be 94 or 96 here */

	/* initialize glyph */
	for (i = 0; i < GLYPH_PER_CHARSET; i++) {
		drcs->chars[i].width = 1; /* drcs glyph must be HALF */
		drcs->chars[i].code = 0; /* this value not used: drcs call by DRCSMMv1 */

		for (j = 0; j < CELL_HEIGHT; j++)
			drcs->chars[i].bitmap[j] = 0;
	}

	cp = start_buf;
	end_buf = cp + strlen(cp);
	while (cp < end_buf) {
		if (*cp == ';') {
			char_num++;
			row = column = 0;
		}
		else if (*cp == '/') {
			row++;
			column = 0;
		}
		else if ('?' <= *cp && *cp <= '~'){ /* load bitmap data to glyph */
			if (DEBUG)
				fprintf(stderr, "char_num(ten):0x%.2X\n", char_num);

			bitmap = bit_mask[BITS_PER_SIXEL] & (*cp - '?'); /* remove offset '?' and use only 6bit */
			set_bitmap(&drcs->chars[char_num], bitmap, row, column);
			column++;
		}
		else if (*cp == ESC || *cp == BEL) { /* end of DECDLD sequence */
			break;
		}
		else {
			fprintf(stderr, "unknown sixel data '%c'\n", *cp);
		}
		cp++;
	}

}

void parse_decdld_header(struct terminal *term)
{
	/*
		DECDLD
		format
			DCS Pfn; Pcn; Pe; Pcmw; Pss; Pt; Pcmh; Pcss; f Dscs Sxbp1 ; Sxbp2 ; .. .; Sxbpn ST
		parameters
			DCS : ESC (0x1B) 'P' (0x50) (DCS(8bit C1 code) is not supported)
			Pfn : fontset (ignored)
			Pcn : start char (0 means SPACE 0x20)
			Pe  : erase mode (force 0)
					0: clear selectet charset
					1: clear only redefined glyph (TODO: not implemented)
					2: clear all drcs charset (TODO: not implemented)
			Pcmw: max cellwidth (force CELL_WEDTH defined in glyph.h)
			Pss : screen size (ignored)
			Pt  : defines the glyph as text or full cell or sixel (force full cell mode)
					TODO implement sixel/text mode
			Pcmh: max cellheight (force CELL_HEIGHT defined in glyph.h)
			Pcss: character set size (force: 96)
					0: 94 gylphs charset
					1: 96 gylphs charset
			f   : '{' (0x7B)
			Dscs: define character set
					Intermediate char: from SPACE (0x20) to '/' (0x2F)
					final char       : from '0' (0x30) to '~' (0x7E)
										but yaft uses from '@' (0x40) to '~' (0x7E) for DRCSMMv1
										(ref: https://github.com/saitoha/drcsterm/blob/master/README.rst)
			ST  : ESC (0x1B) '\' (0x5C) or BEL (0x07)
	*/
	char *cp;
	int Pcn, Pe; /* Pfn, Pcmw, Pss, Pt, Pcmh, Pcss are ignored */
	int charset;
	struct esc_t *esc = &term->esc;
	struct parm_t parm;

	/* replace final char of DECDLD header by NUL '\0' */
	cp = strchr(esc->buf, '{');
	*cp = '\0';

	/* split header by semicolon ';' */
	reset_parm(&parm);
	parse_arg(esc->buf, &parm, ';', isdigit);

	if (parm.argc != 8) /* DECDLD header must have 8 params */
		return;

	/* set params */
	//Pfn  = atoi(parm.argv[0]);
	Pcn  = atoi(parm.argv[1]);
	Pe   = atoi(parm.argv[2]);
	//Pcmw = atoi(parm.argv[3]);
	//Pss  = atoi(parm.argv[4]);
	//Pt   = atoi(parm.argv[5]);
	//Pcmh = atoi(parm.argv[6]);
	//Pcss = atoi(parm.argv[7]);

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
			charset, Pcn, Pe);

	/* set drcs param */
	term->drcs[charset].start_char  = Pcn;
	term->drcs[charset].erase_mode  = Pe; /* TODO: erase mode not implemented */
	//term->drcs[charset].charset_num = (Pcss == 0) ? 94: 96;

	/* read sixel */
	parse_sixel_data(term, &term->drcs[charset], cp + 1); /* skil final char */
}

void check_dcs_header(struct terminal *term)
{
	struct esc_t *esc = &term->esc;
	char *cp;

	/* check DCS header */
	cp = esc->buf;
	while (cp < esc->bp) {
		if (*cp == '{' /* DECDLD */
			|| *cp == 'q') /* sixel */
			break;
		else if (('0' <= *cp && *cp <= '9') || *cp == ';')
			; /* valid DCS header */
		else
			return; /* invalid sequence! */
			
		cp++;
	}
	if (cp == (esc->bp - 1)) /* header only or cannot find final char */
		return;

	/* parse DCS header */
	if (*cp == 'q')
		parse_sixel_header(term);
	else if (*cp == '{')
		parse_decdld_header(term);
}
