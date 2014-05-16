/* See LICENSE for licence details. */
/* function for osc sequence */
void set_palette(struct terminal *term, void *arg)
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
		PT: c ; ?
			response rgb color
				OSC 4 ; c ; rgb:rr/gg/bb ST
	*/
	struct parm_t *pt = (struct parm_t *) arg, sub_parm;
	int i, argc = pt->argc, c, length;
	char **argv = pt->argv;
	long val;
	uint8_t rgb[3];
	uint32_t color;
	char buf[BUFSIZE];

	if (argc != 3)
		return;

	if (strncmp(argv[2], "rgb:", 4) == 0) {
		/*
		rgb:r/g/b
		rgb:rr/gg/bb
		rgb:rrr/ggg/bbb
		rgb:rrrr/gggg/bbbb
		*/
		reset_parm(&sub_parm);
		parse_arg(argv[2] + 4, &sub_parm, '/', isalnum); /* skip "rgb:" */

		if (DEBUG)
			for (i = 0; i < sub_parm.argc; i++)
				fprintf(stderr, "sub_parm.argv[%d]: %s\n", i, sub_parm.argv[i]);

		if (sub_parm.argc != 3)
			return;

		length = strlen(sub_parm.argv[0]);
		c = atoi(argv[1]);

		for (i = 0; i < 3; i++) {
			val = strtol(sub_parm.argv[i], NULL, 16);
			if (DEBUG)
				fprintf(stderr, "val:%ld\n", val);

			if (length == 1)
				rgb[i] = (double) val * 0xFF / 0x0F;
			else if (length == 2)
				rgb[i] = val;
			else if (length == 3)
				rgb[i] = (double) val * 0xFF / 0xFFF;
			else if (length == 4)
				rgb[i] = (double) val * 0xFF / 0xFFFF;
			else
				return;
		}

		color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
		if (DEBUG)
			fprintf(stderr, "color:0x%.6X\n", color);

		if (0 <= c && c < COLORS)
			term->color_palette[c] = color;
	}
	else if (strncmp(argv[2], "#", 1) == 0) {
		/*
		#rgb
		#rrggbb
		#rrrgggbbb
		#rrrrggggbbbb
		*/
		c = atoi(argv[1]);
		length = strlen(argv[2] + 1); /* skip '#' */
		memset(buf, '\0', BUFSIZE);

		if (length == 3) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + 1 + i, 1);
				rgb[i] = (double) strtol(buf, NULL, 16) * 0xFF / 0x0F;
			}
		}
		else if (length == 6) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + 1 + i * 2, 2);
				rgb[i] = strtol(buf, NULL, 16);
			}
		}
		else if (length == 9) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + 1 + i * 3, 3);
				rgb[i] = (double) strtol(buf, NULL, 16) * 0xFF / 0xFFF;
			}
		}
		else if (length == 12) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + 1 + i * 4, 4);
				rgb[i] = (double) strtol(buf, NULL, 16) * 0xFF / 0xFFFF;
			}
		}
		else
			return;

		color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
		if (DEBUG)
			fprintf(stderr, "color:0x%.6X\n", color);

		if (0 <= c && c < COLORS)
			term->color_palette[c] = color;
	}
	else if (strncmp(argv[2], "?", 1) == 0) {
		/*
		?
		*/
		c = atoi(argv[1]);

		if (0 <= c && c < COLORS) {
			for (i = 2; i >= 0; i--)
				rgb[i] = (term->color_palette[c] >> (8 * i)) & 0xFF;
			snprintf(buf, BUFSIZE, "\033]4;%d;rgb:%.2X/%.2X/%.2X\033\\", c, rgb[0], rgb[1], rgb[2]);
			ewrite(term->fd, buf, strlen(buf));
		}
	}
}

void reset_palette(struct terminal *term, void *arg)
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
			term->color_palette[i] = color_list[i];
	}
	else if (argc == 2) { /* reset color_palette[c] */
		c = atoi(argv[1]);
		if (0 <= c && c < COLORS)
			term->color_palette[c] = color_list[c];
	}
}

int isdigit_or_questionmark(int c)
{
	if (isdigit(c) || c == '?')
		return 1;
	else
		return 0;
}

void glyph_width_report(struct terminal *term, void *arg)
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

	//reserved = atoi(argv[1]);
	width = atoi(argv[2]);
	from = atoi(sub_parm.argv[1]);
	to = atoi(sub_parm.argv[2]);

	if ((from < 0 || to >= UCS2_CHARS) /* TODO: change here when implement DRCS */
		|| (width < 0 || width > 2))
		return;

	snprintf(buf, BUFSIZE, "\033]8900;0;0;%d;", width); /* OSC 8900 ; Ps; Pv ; Pw ; */
    ewrite(term->fd, buf, strlen(buf));

	left = right = -1;
	for (i = from; i <= to; i++) {
		wcw = wcwidth(i);
		if (wcw <= 0) /* zero width */
			w = 0;
		else if (term->glyph_map[i] == NULL) /* missing glyph */
			w = wcw;
		else
			w = term->glyph_map[i]->width;

		if (w != width) {
			if (right != -1) {
    			snprintf(buf, BUFSIZE, "%d:%d;", left, right);
    			ewrite(term->fd, buf, strlen(buf));
			}
			else if (left != -1) {
    			snprintf(buf, BUFSIZE, "%d:%d;", left, left);
    			ewrite(term->fd, buf, strlen(buf));
			}

			left = right = -1;
			continue;
		}

		if (left == -1)
			left = i;
		else
			right = i;
	}

	if (right != -1) {
		snprintf(buf, BUFSIZE, "%d:%d;", left, right);
		ewrite(term->fd, buf, strlen(buf));
	}
	else if (left != -1) {
		snprintf(buf, BUFSIZE, "%d:%d;", left, left);
		ewrite(term->fd, buf, strlen(buf));
	}

    ewrite(term->fd, "\033\\", 2); /* ST (ESC BACKSLASH) */
}
