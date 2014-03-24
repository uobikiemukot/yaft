/* See LICENSE for licence details. */
/* misc */
int sum(struct parm_t *pt)
{
	int i, sum = 0;

	for (i = 0; i < pt->argc; i++)
		sum += atoi(pt->argv[i]);

	return sum;
}

/* function for control character */
void bs(struct terminal *term, void *arg)
{
	move_cursor(term, 0, -1);
}

void tab(struct terminal *term, void *arg)
{
	int i;

	for (i = term->cursor.x + 1; i < term->cols; i++) {
		if (term->tabstop[i]) {
			set_cursor(term, term->cursor.y, i);
			return;
		}
	}
	set_cursor(term, term->cursor.y, term->cols - 1);
}

void nl(struct terminal *term, void *arg)
{
	move_cursor(term, 1, 0);
}

void cr(struct terminal *term, void *arg)
{
	set_cursor(term, term->cursor.y, 0);
}

void enter_esc(struct terminal *term, void *arg)
{
	term->esc.state = STATE_ESC;
}

/* function for escape sequence */
void save_state(struct terminal *term, void *arg)
{
	term->state.mode = term->mode & MODE_ORIGIN;
	term->state.cursor = term->cursor;
	term->state.attribute = term->attribute;
}

void restore_state(struct terminal *term, void *arg)
{
	if (term->state.mode & MODE_ORIGIN)
		term->mode |= MODE_ORIGIN;
	else
		term->mode &= ~MODE_ORIGIN;
	term->cursor = term->state.cursor;
	term->attribute = term->state.attribute;
}

void crnl(struct terminal *term, void *arg)
{
	cr(term, NULL);
	nl(term, NULL);
}

void set_tabstop(struct terminal *term, void *arg)
{
	term->tabstop[term->cursor.x] = true;
}

void reverse_nl(struct terminal *term, void *arg)
{
	move_cursor(term, -1, 0);
}

void identify(struct terminal *term, void *arg)
{
	ewrite(term->fd, "\033[?6c", 5); /* "I am a VT102" */
}

void enter_csi(struct terminal *term, void *arg)
{
	term->esc.state = STATE_CSI;
}

void enter_osc(struct terminal *term, void *arg)
{
	term->esc.state = STATE_OSC;
}

void enter_dcs(struct terminal *term, void *arg)
{
	term->esc.state = STATE_DCS;
}

void ris(struct terminal *term, void *arg)
{
	reset(term);
}

/* function for csi sequence */
void insert_blank(struct terminal *term, void *arg)
{
	int i, num = sum((struct parm_t *) arg);

	if (num <= 0)
		num = 1;

	for (i = term->cols - 1; term->cursor.x <= i; i--) {
		if (term->cursor.x <= (i - num))
			copy_cell(term, term->cursor.y, i, term->cursor.y, i - num);
		else
			erase_cell(term, term->cursor.y, i);
	}
}

void curs_up(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, -num, 0);
}

void curs_down(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, num, 0);
}

void curs_forward(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, 0, num);
}

void curs_back(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, 0, -num);
}

void curs_nl(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, num, 0);
	cr(term, NULL);
}

void curs_pl(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, -num, 0);
	cr(term, NULL);
}

void curs_col(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int num = sum(pt), argc = pt->argc;
	char **argv = pt->argv;

	if (argc == 0)
		num = 0;
	else
		num = atoi(argv[argc - 1]) - 1;

	set_cursor(term, term->cursor.y, num);
}

void curs_pos(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int argc = pt->argc, line, col;
	char **argv = pt->argv;

	if (argc == 0) {
		set_cursor(term, 0, 0);
		return;
	}

	if (argc != 2)
		return;

	line = atoi(argv[0]) - 1;
	col = atoi(argv[1]) - 1;
	set_cursor(term, line, col);
}

void erase_display(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, j, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0 : atoi(argv[argc - 1]);

	if (mode < 0 || 2 < mode)
		return;

	if (mode == 0) {
		for (i = term->cursor.y; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				if (i > term->cursor.y || (i == term->cursor.y && j >= term->cursor.x))
					erase_cell(term, i, j);
	}
	else if (mode == 1) {
		for (i = 0; i <= term->cursor.y; i++)
			for (j = 0; j < term->cols; j++)
				if (i < term->cursor.y || (i == term->cursor.y && j <= term->cursor.x))
					erase_cell(term, i, j);
	}
	else if (mode == 2) {
		for (i = 0; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				erase_cell(term, i, j);
	}
}

void erase_line(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0 : atoi(argv[argc - 1]);

	if (mode < 0 || 2 < mode)
		return;

	if (mode == 0) {
		for (i = term->cursor.x; i < term->cols; i++)
			erase_cell(term, term->cursor.y, i);
	}
	else if (mode == 1) {
		for (i = 0; i <= term->cursor.x; i++)
			erase_cell(term, term->cursor.y, i);
	}
	else if (mode == 2) {
		for (i = 0; i < term->cols; i++)
			erase_cell(term, term->cursor.y, i);
	}
}

void insert_line(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	if (term->mode & MODE_ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	num = (num <= 0) ? 1 : num;
	scroll(term, term->cursor.y, term->scroll.bottom, -num);
}

void delete_line(struct terminal *term, void *arg)
{
	int num = sum((struct parm_t *) arg);

	if (term->mode & MODE_ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	num = (num <= 0) ? 1 : num;
	scroll(term, term->cursor.y, term->scroll.bottom, num);
}

void delete_char(struct terminal *term, void *arg)
{
	int i, num = sum((struct parm_t *) arg);

	num = (num <= 0) ? 1 : num;

	for (i = term->cursor.x; i < term->cols; i++) {
		if ((i + num) < term->cols)
			copy_cell(term, term->cursor.y, i, term->cursor.y, i + num);
		else
			erase_cell(term, term->cursor.y, i);
	}
}

void erase_char(struct terminal *term, void *arg)
{
	int i, num = sum((struct parm_t *) arg);

	if (num <= 0)
		num = 1;
	else if (num + term->cursor.x > term->cols)
		num = term->cols - term->cursor.x;

	for (i = term->cursor.x; i < term->cursor.x + num; i++)
		erase_cell(term, term->cursor.y, i);
}

void curs_line(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int num, argc = pt->argc;
	char **argv = pt->argv;

	if (argc == 0)
		num = 0;
	else
		num = atoi(argv[argc - 1]) - 1;

	set_cursor(term, num, term->cursor.x);
}

void set_attr(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, num;
	char **argv = pt->argv;

	if (argc == 0) {
		term->attribute = RESET;
		term->color.fg = DEFAULT_FG;
		term->color.bg = DEFAULT_BG;
		return;
	}

	for (i = 0; i < argc; i++) {
		num = atoi(argv[i]);

		if (num == 0) {                    /* reset all attribute and color */
			term->attribute = RESET;
			term->color.fg = DEFAULT_FG;
			term->color.bg = DEFAULT_BG;
		}
		else if (1 <= num && num <= 7)     /* set attribute */
			term->attribute |= attr_mask[num];
		else if (21 <= num && num <= 27)   /* reset attribute */
			term->attribute &= ~attr_mask[num - 20];
		else if (30 <= num && num <= 37)   /* set foreground */
			term->color.fg = (num - 30);
		else if (num == 38) {              /* set 256 color to foreground */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.fg = atoi(argv[i + 2]);
				i += 2;
			}
		}
		else if (num == 39)                /* reset foreground */
			term->color.fg = DEFAULT_FG;
		else if (40 <= num && num <= 47)   /* set background */
			term->color.bg = (num - 40);
		else if (num == 48) {              /* set 256 color to background */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.bg = atoi(argv[i + 2]);
				i += 2;
			}
		}
		else if (num == 49)                /* reset background */
			term->color.bg = DEFAULT_BG;
		else if (90 <= num && num <= 97)   /* set bright foreground */
			term->color.fg = (num - 90) + BRIGHT_INC;
		else if (100 <= num && num <= 107) /* set bright background */
			term->color.bg = (num - 100) + BRIGHT_INC;
	}
}

void status_report(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, num, argc = pt->argc;
	char **argv = pt->argv, buf[BUFSIZE];

	for (i = 0; i < argc; i++) {
		num = atoi(argv[i]);
		if (num == 5)        /* terminal response: ready */
			ewrite(term->fd, "\033[0n", 4);
		else if (num == 6) { /* cursor position report */
			snprintf(buf, BUFSIZE, "\033[%d;%dR", term->cursor.y + 1, term->cursor.x + 1);
			ewrite(term->fd, buf, strlen(buf));
		}
		else if (num == 15)  /* terminal response: printer not connected */
			ewrite(term->fd, "\033[?13n", 6);
	}
}

void set_mode(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	for (i = 0; i < argc; i++) {
		mode = atoi(argv[i]);
		if (term->esc.buf[1] != '?')
			continue;    /* ansi mode: not implemented */

		if (mode == 6) { /* private mode */
			term->mode |= MODE_ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7)
			term->mode |= MODE_AMRIGHT;
		else if (mode == 25)
			term->mode |= MODE_CURSOR;
	}

}

void reset_mode(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	for (i = 0; i < argc; i++) {
		mode = atoi(argv[i]);
		if (term->esc.buf[1] != '?')
			continue;    /* ansi mode: not implemented */

		if (mode == 6) { /* private mode */
			term->mode &= ~MODE_ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7) {
			term->mode &= ~MODE_AMRIGHT;
			term->wrap = false;
		}
		else if (mode == 25)
			term->mode &= ~MODE_CURSOR;
	}

}

void set_margin(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int argc = pt->argc, top, bottom;
	char **argv = pt->argv;

	if (argc != 2)
		return;

	top = atoi(argv[0]) - 1;
	bottom = atoi(argv[1]) - 1;

	if (top >= bottom)
		return;

	top = (top < 0) ? 0 : (top >= term->lines) ? term->lines - 1 : top;

	bottom = (bottom < 0) ? 0 :
		(bottom >= term->lines) ? term->lines - 1 : bottom;

	term->scroll.top = top;
	term->scroll.bottom = bottom;

	set_cursor(term, 0, 0); /* move cursor to home */
}

void clear_tabstop(struct terminal *term, void *arg)
{
	struct parm_t *pt = (struct parm_t *) arg;
	int i, j, argc = pt->argc, num;
	char **argv = pt->argv;

	if (argc == 0)
		term->tabstop[term->cursor.x] = false;
	else {
		for (i = 0; i < argc; i++) {
			num = atoi(argv[i]);
			if (num == 0)
				term->tabstop[term->cursor.x] = false;
			else if (num == 3) {
				for (j = 0; j < term->cols; j++)
					term->tabstop[j] = false;
				return;
			}
		}
	}
}

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

void glyph_width_report(struct terminal *term, void *arg)
{
	/*
		glyph width report
			* request *
			OSC 8900 ; Ps ; ? : Pw : Pf : Pt ST
				Ps: reserved
				Pw: width (0 or 1 or 2)
				Pf: beginning of unicode code point
				Pt: end of unicode code point
				ST: BEL(0x07) or ESC(0x1B) BACKSLASH(0x5C)
        	* answer *
			OSC 8900 ; Ps ; Pt ; Pf : Pt ; Pf : Pt ; ... ST
				Ps: responce code
					0: ok (default)
					1: recognized but not supported
					2: not recognized
				Pt: reserved (maybe East Asian Width Version)
				<!-- Pw: width (0 or 1 or 2) depricated -->
				Pf: beginning of unicode code point
				Pt: end of unicode code point
				ST: BEL(0x07) or ESC(0x1B) BACKSLASH(0x5C)
		ref
			http://uobikiemukot.github.io/yaft/glyph_width_report.html
			https://gist.github.com/saitoha/8767268
	*/
	struct parm_t *pt = (struct parm_t *) arg, sub_parm;
	int i, width, from, to, left, right, w, wcw; //reserved
	char **argv = pt->argv, buf[BUFSIZE];

	reset_parm(&sub_parm);
	parse_arg(argv[2], &sub_parm, ':', isdigit_or_questionmark);

	if (sub_parm.argc != 4 || *sub_parm.argv[0] != '?')
		return;

	//reserved = atoi(argv[1]);
	width = atoi(sub_parm.argv[1]);
	from = atoi(sub_parm.argv[2]);
	to = atoi(sub_parm.argv[3]);

	if ((from < 0 || to >= UCS2_CHARS) /* change here when implement DRCS */
		|| (width < 0 || width > 2))
		return;

    ewrite(term->fd, "\033]8900;0;0;", 11); /* OSC 8900 ; Ps; Pt ; */

	left = right = -1;
	for (i = from; i <= to; i++) {
		wcw = wcwidth(i);
		if (wcw <= 0) /* zero width */
			w = 0;
		else if (fonts[i].width == 0) /* missing glyph */
			w = wcw;
		else
			w = fonts[i].width;

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

/* function for dcs sequence */
void decdld_header(struct terminal *term, void *arg)
{
	/*
		DECDLD header:
		(http://vt100.net/docs/vt510-rm/DECDLD#T5-2)
		DCS Pfn ; Pcn; Pe; Pcmw; Pss; Pt; Pcmh; Pcss {

			Pfn: Font number (ignored)
			Pcn: Starting character (from 0 to 95 (0x20 - 0x7F))
			Pe: Erase control (0, 1, 2)
			Pcmw: Character matrix width (ignored)
			Pss: Font set size (ignored)
			Pt: Text or full cell (force "2": full cell mode)
			Pcmh: Character matrix height (ignored)
			Pcss: Character set size (force "1": 96 character set)
		
		in yaft, DRCS font size must be the same as cell size.
		So some parameters (Pcmw, Pss, Pcmh, Pcss) are ignored.
		Pt force "2" (full cell mode) or "3" for sixel...(not implemented)

		DRCS are mapped on UCS Private Area.

		~~~
		DRCSMMv1
		(https://github.com/saitoha/drcsterm)

		U+10XXYY (0x40 <= 0xXX <=0x7E, 0x20 <= 0xYY <= 0x7F)

		Dcsc: from '@' (0x40) to '~' (0x7E)
			('0' (0x30) - '?' (0x3F) are not available)
		
		each character set has 96 glyph (0x20 - 0x7F)

		~~~
		ISO-2022-JP-MS
		~~~

	*/
	struct parm_t *pt = (struct parm_t *) arg;
	int argc = pt->argc;
	char **argv = pt->argv;

	if (argc != 7) /* invalid DECDLD header */
		return;

	if (argv[0])
		;

	term->esc.state = STATE_DSCS;
}
