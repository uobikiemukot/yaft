/* See LICENSE for licence details. */
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
	term->esc.state = STATE_ESC;
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
	writeback(term->fd, "\033[?6c", 5);
}

void enter_csi(terminal *term, void *arg)
{
	term->esc.state = STATE_CSI;
}

void enter_osc(terminal *term, void *arg)
{
	term->esc.state = STATE_OSC;
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

	num = (num <= 0) ? 1 : num;
	move_cursor(term, -num, 0);
}

void curs_down(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, num, 0);
}

void curs_forward(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, 0, num);
}

void curs_back(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, 0, -num);
}

void curs_nl(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1 : num;
	move_cursor(term, num, 0);
	cr(term, NULL);
}

void curs_pl(terminal *term, void *arg)
{
	int num = sum((parm_t *) arg);

	num = (num <= 0) ? 1 : num;
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
	parm_t *pt = (parm_t *) arg;
	int i, j, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0 : atoi(argv[argc - 1]);

	if (mode < 0 || 2 < mode)
		return;

	if (mode == 0) {
		for (i = term->cursor.y; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				if (i > term->cursor.y
					|| (i == term->cursor.y && j >= term->cursor.x))
					set_cell(term, i, j, DEFAULT_CHAR);
	}
	else if (mode == 1) {
		for (i = 0; i <= term->cursor.y; i++)
			for (j = 0; j < term->cols; j++)
				if (i < term->cursor.y
					|| (i == term->cursor.y && j <= term->cursor.x))
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
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	mode = (argc == 0) ? 0 : atoi(argv[argc - 1]);

	if (mode < 0 || 2 < mode)
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

	num = (num <= 0) ? 1 : num;
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

	num = (num <= 0) ? 1 : num;
	scroll(term, term->cursor.y, term->scroll.bottom, num);
}

void delete_char(terminal *term, void *arg)
{
	int i, num = sum((parm_t *) arg);
	cell *cp, erase;

	num = (num <= 0) ? 1 : num;

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

		if (num == 0) {						/* reset all attribute and color */
			term->attribute = RESET;
			term->color.fg = DEFAULT_FG;
			term->color.bg = DEFAULT_BG;
		}
		else if (1 <= num && num <= 7)		/* set attribute */
			term->attribute |= attr_mask[num];
		else if (21 <= num && num <= 27)	/* reset attribute */
			term->attribute &= ~attr_mask[num - 20];
		else if (30 <= num && num <= 37)	/* set foreground */
			term->color.fg = (num - 30);
		else if (num == 38) {				/* set 256 color to foreground */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.fg = atoi(argv[i + 2]);
				i += 2;
			}
		}
		else if (num == 39)	/* reset foreground */
			term->color.fg = DEFAULT_FG;
		else if (40 <= num && num <= 47)	/* set background */
			term->color.bg = (num - 40);
		else if (num == 48) {				/* set 256 color to background */
			if ((i + 2) < argc && atoi(argv[i + 1]) == 5) {
				term->color.bg = atoi(argv[i + 2]);
				i += 2;
			}
		}
		else if (num == 49)	/* reset background */
			term->color.bg = DEFAULT_BG;
		else if (90 <= num && num <= 97) 	/* set bright foreground */
			term->color.fg = (num - 90) + BRIGHT_INC;
		else if (100 <= num && num <= 107)	/* set bright background */
			term->color.bg = (num - 100) + BRIGHT_INC;
	}
}

void status_report(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int i, num, argc = pt->argc;
	char **argv = pt->argv, buf[BUFSIZE];

	for (i = 0; i < argc; i++) {
		num = atoi(argv[i]);
		if (num == 5)			/* terminal response: ready */
			writeback(term->fd, "\033[0n", 4);
		else if (num == 6) {	/* cursor position report */
			memset(buf, '\0', BUFSIZE);
			snprintf(buf, BUFSIZE, "\033[%d;%dR", term->cursor.y + 1,
					 term->cursor.x + 1);
			writeback(term->fd, buf, strlen(buf));
		}
		else if (num == 15)	/* terminal response: printer not connected */
			writeback(term->fd, "\033[?13n", 6);
	}
}

void set_mode(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
	int i, argc = pt->argc, mode;
	char **argv = pt->argv;

	for (i = 0; i < argc; i++) {
		mode = atoi(argv[i]);
		if (term->esc.buf[0] != '?')
			continue;			/* ansi mode: not implemented */

		if (mode == 6) {		/* private mode */
			term->mode |= ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7)
			term->mode |= AMRIGHT;
		else if (mode == 25)
			term->mode |= CURSOR;
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
			continue;			/* ansi mode: not implemented */

		if (mode == 6) {		/* private mode */
			term->mode &= ~ORIGIN;
			set_cursor(term, 0, 0);
		}
		else if (mode == 7) {
			term->mode &= ~AMRIGHT;
			term->wrap = false;
		}
		else if (mode == 25)
			term->mode &= ~CURSOR;
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

	if (top >= bottom)			/* top margin must be less than bottom margin */
		return;

	top = (top < 0) ? 0 : (top >= term->lines) ? term->lines - 1 : top;

	bottom = (bottom < 0) ? 0 :
		(bottom >= term->lines) ? term->lines - 1 : bottom;

	term->scroll.top = top;
	term->scroll.bottom = bottom;

	set_cursor(term, 0, 0);		/* move cursor to home */
}


void clear_tabstop(terminal *term, void *arg)
{
	parm_t *pt = (parm_t *) arg;
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
/* not implemented */
