/* See LICENSE for licence details. */
/* function for osc sequence */
int32_t parse_color1(char *seq)
{
	/*
	format
		rgb:r/g/b
		rgb:rr/gg/bb
		rgb:rrr/ggg/bbb
		rgb:rrrr/gggg/bbbb
	*/
	int i, length, value;
	int32_t color;
	uint32_t rgb[3];
	struct parm_t parm;

	reset_parm(&parm);
	parse_arg(seq, &parm, '/', isalnum);

	for (i = 0; i < parm.argc; i++)
		logging(DEBUG, "parm.argv[%d]: %s\n", i, parm.argv[i]);

	if (parm.argc != 3)
		return -1;

	length = strlen(parm.argv[0]);

	for (i = 0; i < 3; i++) {
		value = hex2num(parm.argv[i]);
		logging(DEBUG, "value:%d\n", value);

		if (length == 1)      /* r/g/b/ */
			rgb[i] = bit_mask[8] & (value * 0xFF / 0x0F);
		else if (length == 2) /* rr/gg/bb */
			rgb[i] = bit_mask[8] & value;
		else if (length == 3) /* rrr/ggg/bbb */
			rgb[i] = bit_mask[8] & (value * 0xFF / 0xFFF);
		else if (length == 4) /* rrrr/gggg/bbbb */
			rgb[i] = bit_mask[8] & (value * 0xFF / 0xFFFF);
		else
			return -1;
	}

	color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
	logging(DEBUG, "color:0x%.6X\n", color);

	return color;
}

int32_t parse_color2(char *seq)
{
	/*
	format
		#rgb
		#rrggbb
		#rrrgggbbb
		#rrrrggggbbbb
	*/
	int i, length;
	uint32_t rgb[3];
	int32_t color;
	char buf[BUFSIZE];

	length = strlen(seq);
	memset(buf, '\0', BUFSIZE);

	if (length == 3) {       /* rgb */
		for (i = 0; i < 3; i++) {
			strncpy(buf, seq + i, 1);
			rgb[i] = bit_mask[8] & hex2num(buf) * 0xFF / 0x0F;
		}
	} else if (length == 6) {  /* rrggbb */
		for (i = 0; i < 3; i++) { /* rrggbb */
			strncpy(buf, seq + i * 2, 2);
			rgb[i] = bit_mask[8] & hex2num(buf);
		}
	} else if (length == 9) {  /* rrrgggbbb */
		for (i = 0; i < 3; i++) {
			strncpy(buf, seq + i * 3, 3);
			rgb[i] = bit_mask[8] & hex2num(buf) * 0xFF / 0xFFF;
		}
	} else if (length == 12) { /* rrrrggggbbbb */
		for (i = 0; i < 3; i++) {
			strncpy(buf, seq + i * 4, 4);
			rgb[i] = bit_mask[8] & hex2num(buf) * 0xFF / 0xFFFF;
		}
	} else {
		return -1;
	}

	color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
	logging(DEBUG, "color:0x%.6X\n", color);

	return color;
}

void set_palette(struct terminal_t *term, void *arg)
{
	/*
	OSC Ps ; Pt ST
	ref: http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
	ref: http://ttssh2.sourceforge.jp/manual/ja/about/ctrlseq.html#OSC

	only recognize change color palette:
		Ps: 4
		Pt: c ; spec
			c: color index (from 0 to 255)
			spec:
				rgb:r/g/b
				rgb:rr/gg/bb
				rgb:rrr/ggg/bbb
				rgb:rrrr/gggg/bbbb
				#rgb
				#rrggbb
				#rrrgggbbb
				#rrrrggggbbbb
					this rgb format is "RGB Device String Specification"
					see http://xjman.dsl.gr.jp/X11R6/X11/CH06.html
		Pt: c ; ?
			response rgb color
				OSC 4 ; c ; rgb:rr/gg/bb ST

	TODO: this function only works in 32bpp mode
	*/
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, index;
	int32_t color;
	uint8_t rgb[3];
	char **argv = pt->argv;
	char buf[BUFSIZE];

	if (argc != 3)
		return;

	index = dec2num(argv[1]);
	if (index < 0 || index >= COLORS)
		return;

	if (strncmp(argv[2], "rgb:", 4) == 0) {
		if ((color = parse_color1(argv[2] + 4)) != -1) { /* skip "rgb:" */
			term->virtual_palette[index] = (uint32_t) color;
			term->palette_modified = true;
		}
	} else if (strncmp(argv[2], "#", 1) == 0) {
		if ((color = parse_color2(argv[2] + 1)) != -1) { /* skip "#" */
			term->virtual_palette[index] = (uint32_t) color;
			term->palette_modified = true;
		}
	} else if (strncmp(argv[2], "?", 1) == 0) {
		for (i = 0; i < 3; i++)
			rgb[i] = bit_mask[8] & (term->virtual_palette[index] >> (8 * (2 - i)));

		snprintf(buf, BUFSIZE, "\033]4;%d;rgb:%.2X/%.2X/%.2X\033\\",
			index, rgb[0], rgb[1], rgb[2]);
		ewrite(term->fd, buf, strlen(buf));
	}
}

void reset_palette(struct terminal_t *term, void *arg)
{
	/*
	reset color c
		OSC 104 ; c ST
			c: index of color
			ST: BEL or ESC \
	reset all color
		OSC 104 ST
			ST: BEL or ESC \

	terminfo: oc=\E]104\E\\
	*/
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, c;
	char **argv = pt->argv;

	if (argc < 2) { /* reset all color palette */
		for (i = 0; i < COLORS; i++)
			term->virtual_palette[i] = color_list[i];
		term->palette_modified = true;
	} else if (argc == 2) { /* reset color_palette[c] */
		c = dec2num(argv[1]);
		if (0 <= c && c < COLORS) {
			term->virtual_palette[c] = color_list[c];
			term->palette_modified = true;
		}
	}
}

int isdigit_or_questionmark(int c)
{
	if (isdigit(c) || c == '?')
		return 1;
	else
		return 0;
}

void glyph_width_report(struct terminal_t *term, void *arg)
{
	/*
	glyph width report
		* request *
		OSC 8900 ; Ps ; Pw ; ? : Pf : Pt ST
			Ps: reserved
			Pw: width (0 or 1 or 2)
			Pfrom: beginning of unicode code point
			Pto: end of unicode code point
			ST: BEL(0x07) or ESC(0x1B) BACKSLASH(0x5C)
		* answer *
		OSC 8900 ; Ps ; Pv ; Pw ; Pf : Pt ; Pf : Pt ; ... ST
			Ps: responce code
				0: ok (default)
				1: recognized but not supported
				2: not recognized
			Pv: reserved (maybe East Asian Width Version)
			Pw: width (0 or 1 or 2)
			Pfrom: beginning of unicode code point
			Pto: end of unicode code point
			ST: BEL(0x07) or ESC(0x1B) BACKSLASH(0x5C)
	ref
		http://uobikiemukot.github.io/yaft/glyph_width_report.html
		https://gist.github.com/saitoha/8767268
	*/
	struct parm_t *pt = (struct parm_t *) arg, sub_parm;
	int i, argc = pt->argc, width, from, to, left, right, w, wcw; //reserved
	char **argv = pt->argv, buf[BUFSIZE];

	if (argc < 4)
		return;

	reset_parm(&sub_parm);
	parse_arg(argv[3], &sub_parm, ':', isdigit_or_questionmark);

	if (sub_parm.argc != 3 || *sub_parm.argv[0] != '?')
		return;

	//reserved = dec2num(argv[1]);
	width = dec2num(argv[2]);
	from  = dec2num(sub_parm.argv[1]);
	to    = dec2num(sub_parm.argv[2]);

	if ((width < 0) || (width > 2))
		return;

	/* unicode private area: plane 16 (DRCSMMv1) is always half */
	if ((from < 0) || (to >= UCS2_CHARS))
		return;

	snprintf(buf, BUFSIZE, "\033]8900;0;0;%d;", width); /* OSC 8900 ; Ps; Pv ; Pw ; */
	ewrite(term->fd, buf, strlen(buf));

	left = right = -1;
	for (i = from; i <= to; i++) {
		wcw = wcwidth(i);
		if (wcw <= 0) /* zero width */
			w = 0;
		else if (term->glyph[i] == NULL) /* missing glyph */
			w = wcw;
		else
			w = term->glyph[i]->width;

		if (w != width) {
			if (right != -1) {
				snprintf(buf, BUFSIZE, "%d:%d;", left, right);
				ewrite(term->fd, buf, strlen(buf));
			} else if (left != -1) {
				snprintf(buf, BUFSIZE, "%d:%d;", left, left);
				ewrite(term->fd, buf, strlen(buf));
			}
			left = right = -1;
		} else {
			if (left == -1)
				left = i;
			else
				right = i;
		}
	}

	if (right != -1) {
		snprintf(buf, BUFSIZE, "%d:%d;", left, right);
		ewrite(term->fd, buf, strlen(buf));
	} else if (left != -1) {
		snprintf(buf, BUFSIZE, "%d:%d;", left, left);
		ewrite(term->fd, buf, strlen(buf));
	}

	ewrite(term->fd, "\033\\", 2); /* ST (ESC BACKSLASH) */
}
