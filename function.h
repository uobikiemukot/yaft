/* misc */
int sum(parm_t *pt)
{
	int i, sum = 0;

	for (i = 0; i < pt->argc; i++)
		sum += atoi(pt->argv[i]);

	return sum;
}

void ignore(terminal *term, void *arg)
{
	return;
}

/* depricated */
void screen_test(terminal *term, void *arg)
{
	int i, j;

	for (i = 0; i < term->lines; i++)
		for (j = 0; j < term->cols; j++)
			set_cell(term, i, j, 'E');
}

/* function for control character */
void bs(terminal *term, void *arg)
{
	move_cursor(term, 0, -1);
}

void tab(terminal *term, void *arg)
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

void nl(terminal *term, void *arg)
{
	move_cursor(term, 1, 0);
}

void cr(terminal *term, void *arg)
{
	set_cursor(term, term->cursor.y, 0);
}

void enter_esc(terminal *term, void *arg)
{
	term->esc.state = ESC;
}

/* function for escape sequence */
void save_state(terminal *term, void *arg)
{
	term->save_state.cursor = term->cursor;
	term->save_state.attribute = term->attribute;
	term->save_state.mode = term->mode & ORIGIN;
}

void restore_state(terminal *term, void *arg)
{
	term->cursor = term->save_state.cursor;
	term->attribute = term->save_state.attribute;

	if (term->save_state.mode & ORIGIN)
		term->mode |= ORIGIN;
	else
		term->mode &= ~ORIGIN;
}

void crnl(terminal *term, void *arg)
{
	cr(term, NULL);
	nl(term, NULL);
}

void set_tabstop(terminal *term, void *arg)
{
	term->tabstop[term->cursor.x] = true;
}

void reverse_nl(terminal *term, void *arg)
{
	move_cursor(term, -1, 0);
}

void identify(terminal *term, void *arg)
{
	writeback(term, "\e[?6c", 5);
}

void enter_csi(terminal *term, void *arg)
{
	term->esc.state = CSI;
}

void enter_osc(terminal *term, void *arg)
{
	term->esc.state = OSC;
}

void ris(terminal *term, void *arg)
{
	reset(term);
}

/* function for csi sequence */
void insert_blank(terminal *term, void *arg)
{
	int i, num = sum((parm_t *) arg);
	cell *cp, erase;

	if (num <= 0)
		num = 1;

	init_cell(&erase);

	for (i = term->cols - 1; term->cursor.x <= i; i--) {
		if (term->cursor.x <= (i - num))
			cp = &term->cells[(i - num) + term->cursor.y * term->cols];
		else
			cp = &erase;

		if (i == term->cols - 1 && cp->wide == WIDE)
			cp = &erase;

		if (cp->wide != NEXT_TO_WIDE)
			set_cell(term, term->cursor.y, i, cp->code);
	}
}

void curs_up(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, -num, 0);
}

void curs_down(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, num, 0);
}

void curs_forward(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, 0, num);
}

void curs_back(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, 0, -num);
}

void curs_nl(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, num, 0);
	cr(term, NULL);
}

void curs_pl(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1: num;
	move_cursor(term, -num, 0);
	cr(term, NULL);
}

void curs_col(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int num = sum(pt), argc = pt->argc;
	char **argv = pt->argv;

	if (argc == 0)
		num = 0;
	else
		num = atoi(argv[argc - 1]) - 1;

	set_cursor(term, term->cursor.y, num);
}

void curs_pos(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
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

void erase_display(terminal *term, void *arg)
{
	/*
		mode 0: erase from cursor to the end of display (default)
		mode 1: erase from the beginning of display to cursor
		mode 2: erase whole display
	*/
	parm_t *pt = (parm_t *) arg;
	int i, j, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0: atoi(argv[argc - 1]);

	if (mode < 0 || mode > 2)
		return;

	if (mode == 0) {
		for (i = term->cursor.y; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				if (i > term->cursor.y || (i == term->cursor.y && j >= term->cursor.x))
					set_cell(term, i, j, DEFAULT_CHAR);
	}
	else if (mode == 1) {
		for (i = 0; i <= term->cursor.y; i++)
			for (j = 0; j < term->cols; j++)
				if (i < term->cursor.y || (i == term->cursor.y && j <= term->cursor.x))
					set_cell(term, i, j, DEFAULT_CHAR);
	}
	else if (mode == 2) {
		for (i = 0; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				set_cell(term, i, j, DEFAULT_CHAR);
	}
}

void erase_line(terminal *term, void *arg)
{
	/*
		mode 0: erase from cursor to the end of the line (default)
		mode 1: erase from the beginning of the line to cursor
		mode 2: erase whole line
	*/
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0: atoi(argv[argc - 1]);

	if (mode < 0 || mode > 2)
		return;

	if (mode == 0) {
		for (i = term->cursor.x; i < term->cols; i++)
			set_cell(term, term->cursor.y, i, DEFAULT_CHAR);
	}
	else if (mode == 1) {
		for (i = 0; i <= term->cursor.x; i++)
			set_cell(term, term->cursor.y, i, DEFAULT_CHAR);
	}
	else if (mode == 2) {
		for (i = 0; i < term->cols; i++)
			set_cell(term, term->cursor.y, i, DEFAULT_CHAR);
	}
}

void insert_line(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	if (term->mode & ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	num = (num <= 0) ? 1: num;
	scroll(term, term->cursor.y, term->scroll.bottom, -num);
}

void delete_line(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	if (term->mode & ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	num = (num <= 0) ? 1: num;
	scroll(term, term->cursor.y, term->scroll.bottom, num);
}

void delete_char(terminal *term, void *arg)
{
	int i, num = sum((parm_t *) arg);
	cell *cp, erase;

	num = (num <= 0) ? 1: num;

	init_cell(&erase);

	for (i = term->cursor.x; i < term->cols; i++) {
		if ((i + num) < term->cols)
			cp = &term->cells[(i + num) + term->cursor.y * term->cols];
		else
			cp = &erase;

		if (i == term->cols - 1 && cp->wide == WIDE)
			cp = &erase;

		if (cp->wide != NEXT_TO_WIDE)
			set_cell(term, term->cursor.y, i, cp->code);
	}
}

void erase_char(terminal *term, void *arg)
{
	int i, num = sum((parm_t *) arg);

	if (num <= 0)
		num = 1;
	else if (num + term->cursor.x > term->cols)
		num = term->cols - term->cursor.x;

	for (i = term->cursor.x; i < term->cursor.x + num; i++)
		set_cell(term, term->cursor.y, i, DEFAULT_CHAR);
}

void curs_line(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int num, argc = pt->argc;
	char **argv = pt->argv;

	if (argc == 0)
		num = 0;
	else
		num = atoi(argv[argc - 1]) - 1;

	set_cursor(term, num, term->cursor.x);
}

void set_attr(terminal *term, void *arg)
{
	/*
	ESC [ Pn m
	0       reset all attribute

	1       bold
	4       underline
	7       reverse

	21      reset bold
	24      reset underline
	27      reset reverse

	30      set black foreground
	31      set red foreground
	32      set green foreground
	33      set brown foreground
	34      set blue foreground
	35      set magenta foreground
	36      set cyan foreground
	37      set white foreground
	38      xterm extension (set foreground by using 256colors)
	39      set default foreground color

	40      set black background
	41      set red background
	42      set green background
	43      set brown background
	44      set blue background
	45      set magenta background
	46      set cyan background
	47      set white background
	48      xterm extension (set background by using 256colors)
	49      set default background color
	*/
	parm_t *pt = (parm_t *) arg;
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

		if (num == 0) { /* reset all attribute and color */
			term->attribute = RESET;
			term->color.fg = DEFAULT_FG;
			term->color.bg = DEFAULT_BG;
		}
		else if (num == 1) /* set attribute */
			term->attribute |= BOLD;
		else if (num == 4)
			term->attribute |= UNDERLINE;
		else if (num == 7)
			term->attribute |= REVERSE;
		else if (num == 21) /* reset attribute */
			term->attribute &= ~BOLD;
		else if (num == 24)
			term->attribute &= ~UNDERLINE;
		else if (num == 27)
			term->attribute &= ~REVERSE;
		else if (num == 30) /* set foreground */
			term->color.fg =
				(term->attribute & BOLD) ? DARKGRAY: BLACK;
		else if (num == 31)
			term->color.fg =
				(term->attribute & BOLD) ? LIGHTRED: RED;
		else if (num == 32)
			term->color.fg =
				(term->attribute & BOLD) ? LIGHTGREEN: GREEN;
		else if (num == 33)
			term->color.fg =
				(term->attribute & BOLD) ? YELLOW: BROWN;
		else if (num == 34)
			term->color.fg =
				(term->attribute & BOLD) ? LIGHTBLUE: BLUE;
		else if (num == 35)
			term->color.fg =
				(term->attribute & BOLD) ? LIGHTMAGENTA: MAGENTA;
		else if (num == 36)
			term->color.fg =
				(term->attribute & BOLD) ? LIGHTCYAN: CYAN;
		else if (num == 37)
			term->color.fg =
				(term->attribute & BOLD) ? WHITE: GRAY;
		else if (num ==  38) {
			/* set 256 color to foreground */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.fg = term->color_palette[atoi(argv[i + 2])];
				i += 2;
			}
		}
		else if (num == 39)
			term->color.fg = DEFAULT_FG;
		else if (num == 40) /* set background */
			term->color.bg = BLACK;
		else if (num == 41)
			term->color.bg = RED;
		else if (num == 42)
			term->color.bg = GREEN;
		else if (num == 43)
			term->color.bg = BROWN;
		else if (num == 44)
			term->color.bg = BLUE;
		else if (num == 45)
			term->color.bg = MAGENTA;
		else if (num == 46)
			term->color.bg = CYAN;
		else if (num == 47)
			term->color.bg = GRAY;
		else if (num == 48) {
			/* set 256 color to background */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.bg = term->color_palette[atoi(argv[i + 2])];
				i += 2;
			}
		}
		else if (num ==  49)
			term->color.bg = DEFAULT_BG;
	}
}

void status_report(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int i, num, argc = pt->argc;
	char **argv = pt->argv, buf[BUFSIZE];

	for (i = 0; i < argc; i++) {
		num = atoi(argv[i]);
		if (num == 5) /* terminal response: ready */
			writeback(term, "\e[0n", 4);
		else if (num == 6) { /* cursor position report */
			memset(buf, '\0', BUFSIZE);
			snprintf(buf, BUFSIZE, "\e[%d;%dR", term->cursor.y + 1, term->cursor.x + 1);
			writeback(term, buf, strlen(buf));
		}
		else if(num == 15) /* terminal response: printer not connected */
			writeback(term, "\e[?13n", 6);
	}
}

void set_mode(terminal *term, void *arg)
{
	/*
		modes
			COLUMN: 80/132 colmun mode switch (DECCOLM)
			ORIGIN: origin mode (DECOM)
			AMRIGHT: auto margin right (DECAWM)
			AMLEFT: auto margin left
			CURSOR: cursor visible (DECTECM)
	*/
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	for (i = 0; i < argc; i++) {
		mode = atoi(argv[i]);
		if (term->esc.buf[0] != '?')
			continue; /* ansi mode: not implemented */

		/* private mode */
		if (mode == 3) {
			term->mode |= COLUMN;
			resize(term, term->lines, 132);
		}
		else if (mode == 6) {
			term->mode |= ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7)
			term->mode |= AMRIGHT;
		else if (mode == 25)
			term->mode |= CURSOR;
		else if (mode == 45)
			term->mode |= AMLEFT;
	}
}

void reset_mode(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	for (i = 0; i < argc; i++) {
		mode = atoi(argv[i]);
		if (term->esc.buf[0] != '?')
			continue; /* ansi mode: not implemented */

		/* private mode */
		if (mode == 3) {
			term->mode &= ~COLUMN;
			resize(term, term->lines, 80);
		}
		else if (mode == 6) {
			term->mode &= ~ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7)
			term->mode &= ~AMRIGHT;
		else if (mode == 25)
			term->mode &= ~CURSOR;
		else if (mode == 45)
			term->mode &= ~AMLEFT;
	}
}

void set_margin(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int argc = pt->argc, top, bottom;
	char **argv = pt->argv;

	if (argc != 2)
		return;

	top = atoi(argv[0]) - 1;
	bottom = atoi(argv[1]) - 1;

	if (top >= bottom) /* top margin must be less than bottom margin */
		return;

	top = (top < 0) ? 0:
		(top >= term->lines) ? term->lines - 1: top;

	bottom = (bottom < 0) ? 0:
		(bottom >= term->lines) ? term->lines - 1: bottom;

	term->scroll.top = top;
	term->scroll.bottom = bottom;

	set_cursor(term, 0, 0); /* move cursor to home */
}


void clear_tabstop(terminal *term, void *arg)
{
	/*
		ESC [ Ps g
			0: Tabulation clear at current column (default)
			3: all character tab stops are cleared
		yaft doesn't hav TSM
	*/
	parm_t *pt = (parm_t *) arg;
	int i, j, k, argc = pt->argc, num;
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
void set_palette(terminal *term, void *arg)
{
	/*
	OSC Ps ; Pt ST
	OSC Ps ; Pt BEL

	only recognize change color palette:
		Ps = 4 ; c ; spec
		spec:
			rgb:r/g/b
			rgb:rr/gg/bb
			rgb:rrr/ggg/bbb
			rgb:rrrr/gggg/bbbb
			#rgb
			#rrggbb
			#rrrgggbbb
			#rrrrggggbbbb
			? (not implemented)
		this rgb format is "RGB Device String Specification"
		see http://xjman.dsl.gr.jp/X11R6/X11/CH06.html
	*/
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, num, length;
	char **argv = pt->argv, *endptr, buf[BUFSIZE];
	double val;
	u8 rgb[3];
	u32 color;

	if (argc == 6) {
		/*
		rgb:r/g/b
		rgb:rr/gg/bb
		rgb:rrr/ggg/bbb
		rgb:rrrr/gggg/bbbb
		*/
		num = atoi(argv[1]);

		for (i = 0; i < 3; i++) {
			length = strlen(argv[3 + i]);
			val = strtol(argv[3 + i], &endptr, 16);

			if (length == 1)
				rgb[i] = val * (double) (0xFF / 0x0F);
			else if (length == 2)
				rgb[i] = val;
			else if (length == 3)
				rgb[i] = val * (double) (0xFF / 0xFFF);
			else if (length == 4)
				rgb[i] = val * (double) (0xFF / 0xFFFF);
			else
				return;
		}

		color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
		if (0 <= num && num < COLORS)
			term->color_palette[num] = color;
	}
	else if (argc == 3) {
		/*
		#rgb
		#rrggbb
		#rrrgggbbb
		#rrrrggggbbbb
		*/
		num = atoi(argv[1]);
		length = strlen(argv[2]);
		memset(buf, '\0', BUFSIZE);

		if (length == 3) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + i, 1);
				rgb[i] = strtol(buf, &endptr, 16) << 4;
			}
		}
		else if (length == 6) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + i * 2, 2);
				rgb[i] = strtol(buf, &endptr, 16);
			}
		}
		else if (length == 9) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + i * 3, 3);
				rgb[i] = strtol(buf, &endptr, 16) >> 4;
			}
		}
		else if (length == 12) {
			for (i = 0; i < 3; i++) {
				strncpy(buf, argv[2] + i * 4, 4);
				rgb[i] = strtol(buf, &endptr, 16) >> 8;
			}
		}
		else
			return;

		color = (rgb[0] << 16) + (rgb[1] << 8) + rgb[2];
		if (0 <= num && num < COLORS)
			term->color_palette[num] = color;
	}
	else if (argc == 2) {
		/*
		?
		*/
		num = atoi(argv[1]);
		length = strlen(term->esc.buf);
		memset(buf, '\0', BUFSIZE);

		if (term->esc.buf[length - 1] == '?'
			&& 0 <= num && num < COLORS) {
			for (i = 0; i < 3; i++)
				rgb[i] = (term->color_palette[num] >> (8 * (2 - i))) & 0xFF;
			snprintf(buf, BUFSIZE, "\e]4;%d;rgb:%2X/%2X/2X\e\\", num, rgb[0], rgb[1], rgb[2]);
			writeback(term, buf, strlen(buf));
		}
	}
}

void reset_palette(terminal *term, void *arg)
{
	/*
		ESC ] 104 ; c ESC \ : reset color c 
		or ESC ] 104 ; c BEL

		ESC ] 104 ESC \ : reset all color

		terminfo: oc=\E]104\E\\
	*/
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, num;
	char **argv = pt->argv;

	if (argc < 2) {
		for (i = 0; i < COLORS; i++)
			term->color_palette[i] = default_color_palette[i];
	}
	else if (argc == 2) {
		num = atoi(argv[1]);
		if (0 <= num && num < COLORS)
			term->color_palette[num] = default_color_palette[num];
	}
}
