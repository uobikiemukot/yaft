/* See LICENSE for licence details. */
/* function for csi sequence */
void insert_blank(struct terminal_t *term, struct parm_t *parm)
{
	int i, num = sum(parm);

	if (num <= 0)
		num = 1;

	for (i = term->cols - 1; term->cursor.x <= i; i--) {
		if (term->cursor.x <= (i - num))
			copy_cell(term, term->cursor.y, i, term->cursor.y, i - num);
		else
			erase_cell(term, term->cursor.y, i);
	}
}

void curs_up(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, -num, 0);
}

void curs_down(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, num, 0);
}

void curs_forward(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, 0, num);
}

void curs_back(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, 0, -num);
}

void curs_nl(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, num, 0);
	cr(term);
}

void curs_pl(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (num <= 0)
		num = 1;

	move_cursor(term, -num, 0);
	cr(term);
}

void curs_col(struct terminal_t *term, struct parm_t *parm)
{
	int num;

	num = (parm->argc <= 0) ? 0: dec2num(parm->argv[parm->argc - 1]) - 1;
	set_cursor(term, term->cursor.y, num);
}

void curs_pos(struct terminal_t *term, struct parm_t *parm)
{
	int line, col;

	if (parm->argc <= 0) {
		line = col = 0;
	} else if (parm->argc == 2) {
		line = dec2num(parm->argv[0]) - 1;
		col  = dec2num(parm->argv[1]) - 1;
	} else {
		return;
	}

	if (line < 0)
		line = 0;
	if (col < 0)
		col = 0;

	set_cursor(term, line, col);
}

void curs_line(struct terminal_t *term, struct parm_t *parm)
{
	int num;

	num = (parm->argc <= 0) ? 0: dec2num(parm->argv[parm->argc - 1]) - 1;
	set_cursor(term, num, term->cursor.x);
}

void erase_display(struct terminal_t *term, struct parm_t *parm)
{
	int i, j, mode;

	mode = (parm->argc <= 0) ? 0: dec2num(parm->argv[parm->argc - 1]);

	if (mode < 0 || 2 < mode)
		return;

	if (mode == 0) {
		for (i = term->cursor.y; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				if (i > term->cursor.y || (i == term->cursor.y && j >= term->cursor.x))
					erase_cell(term, i, j);
	} else if (mode == 1) {
		for (i = 0; i <= term->cursor.y; i++)
			for (j = 0; j < term->cols; j++)
				if (i < term->cursor.y || (i == term->cursor.y && j <= term->cursor.x))
					erase_cell(term, i, j);
	} else if (mode == 2) {
		for (i = 0; i < term->lines; i++)
			for (j = 0; j < term->cols; j++)
				erase_cell(term, i, j);
	}
}

void erase_line(struct terminal_t *term, struct parm_t *parm)
{
	int i, mode;

	mode = (parm->argc <= 0) ? 0: dec2num(parm->argv[parm->argc - 1]);

	if (mode < 0 || 2 < mode)
		return;

	if (mode == 0) {
		for (i = term->cursor.x; i < term->cols; i++)
			erase_cell(term, term->cursor.y, i);
	} else if (mode == 1) {
		for (i = 0; i <= term->cursor.x; i++)
			erase_cell(term, term->cursor.y, i);
	} else if (mode == 2) {
		for (i = 0; i < term->cols; i++)
			erase_cell(term, term->cursor.y, i);
	}
}

void insert_line(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (term->mode & MODE_ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	if (num <= 0)
		num = 1;

	scroll(term, term->cursor.y, term->scroll.bottom, -num);
}

void delete_line(struct terminal_t *term, struct parm_t *parm)
{
	int num = sum(parm);

	if (term->mode & MODE_ORIGIN) {
		if (term->cursor.y < term->scroll.top
			|| term->cursor.y > term->scroll.bottom)
			return;
	}

	if (num <= 0)
		num = 1;

	scroll(term, term->cursor.y, term->scroll.bottom, num);
}

void delete_char(struct terminal_t *term, struct parm_t *parm)
{
	int i, num = sum(parm);

	if (num <= 0)
		num = 1;

	for (i = term->cursor.x; i < term->cols; i++) {
		if ((i + num) < term->cols)
			copy_cell(term, term->cursor.y, i, term->cursor.y, i + num);
		else
			erase_cell(term, term->cursor.y, i);
	}
}

void erase_char(struct terminal_t *term, struct parm_t *parm)
{
	int i, num = sum(parm);

	if (num <= 0)
		num = 1;
	else if (num + term->cursor.x > term->cols)
		num = term->cols - term->cursor.x;

	for (i = term->cursor.x; i < term->cursor.x + num; i++)
		erase_cell(term, term->cursor.y, i);
}

void set_attr(struct terminal_t *term, struct parm_t *parm)
{
	int i, num;

	if (parm->argc <= 0) {
		term->attribute     = ATTR_RESET;
		term->color_pair.fg = DEFAULT_FG;
		term->color_pair.bg = DEFAULT_BG;
		return;
	}

	for (i = 0; i < parm->argc; i++) {
		num = dec2num(parm->argv[i]);

		if (num == 0) {                        /* reset all attribute and color */
			term->attribute = ATTR_RESET;
			term->color_pair.fg = DEFAULT_FG;
			term->color_pair.bg = DEFAULT_BG;
		} else if (1 <= num && num <= 7) {     /* set attribute */
			term->attribute |= attr_mask[num];
		} else if (21 <= num && num <= 27) {   /* reset attribute */
			term->attribute &= ~attr_mask[num - 20];
		} else if (30 <= num && num <= 37) {   /* set foreground */
			term->color_pair.fg = (num - 30);
		} else if (num == 38) {                /* set 256 color to foreground */
			if ((i + 2) < parm->argc && dec2num(parm->argv[i + 1]) == 5) {
				term->color_pair.fg = dec2num(parm->argv[i + 2]);
				i += 2;
			}
		} else if (num == 39) {                /* reset foreground */
			term->color_pair.fg = DEFAULT_FG;
		} else if (40 <= num && num <= 47) {   /* set background */
			term->color_pair.bg = (num - 40);
		} else if (num == 48) {                /* set 256 color to background */
			if ((i + 2) < parm->argc && dec2num(parm->argv[i + 1]) == 5) {
				term->color_pair.bg = dec2num(parm->argv[i + 2]);
				i += 2;
			}
		} else if (num == 49) {                /* reset background */
			term->color_pair.bg = DEFAULT_BG;
		} else if (90 <= num && num <= 97) {   /* set bright foreground */
			term->color_pair.fg = (num - 90) + BRIGHT_INC;
		} else if (100 <= num && num <= 107) { /* set bright background */
			term->color_pair.bg = (num - 100) + BRIGHT_INC;
		}
	}
}

void status_report(struct terminal_t *term, struct parm_t *parm)
{
	int i, num;
	char buf[BUFSIZE];

	for (i = 0; i < parm->argc; i++) {
		num = dec2num(parm->argv[i]);
		if (num == 5) {         /* terminal response: ready */
			ewrite(term->fd, "\033[0n", 4);
		} else if (num == 6) {  /* cursor position report */
			snprintf(buf, BUFSIZE, "\033[%d;%dR", term->cursor.y + 1, term->cursor.x + 1);
			ewrite(term->fd, buf, strlen(buf));
		} else if (num == 15) { /* terminal response: printer not connected */
			ewrite(term->fd, "\033[?13n", 6);
		}
	}
}

void device_attribute(struct terminal_t *term, struct parm_t *parm)
{
	/* TODO: refer VT525 DA */
	(void) parm;
	ewrite(term->fd, "\033[?6c", 5); /* "I am a VT102" */
}

void set_mode(struct terminal_t *term, struct parm_t *parm)
{
	int i, mode;

	for (i = 0; i < parm->argc; i++) {
		mode = dec2num(parm->argv[i]);
		if (*(term->esc.buf + 1) != '?')
			continue; /* not supported */

		if (mode == 6) { /* private mode */
			term->mode |= MODE_ORIGIN;
			set_cursor(term, 0, 0);
		} else if (mode == 7) {
			term->mode |= MODE_AMRIGHT;
		} else if (mode == 25) {
			term->mode |= MODE_CURSOR;
		} else if (mode == 8901) {
			term->mode |= MODE_VWBS;
		}
	}

}

void reset_mode(struct terminal_t *term, struct parm_t *parm)
{
	int i, mode;

	for (i = 0; i < parm->argc; i++) {
		mode = dec2num(parm->argv[i]);
		if (*(term->esc.buf + 1) != '?')
			continue; /* not supported */

		if (mode == 6) { /* private mode */
			term->mode &= ~MODE_ORIGIN;
			set_cursor(term, 0, 0);
		} else if (mode == 7) {
			term->mode &= ~MODE_AMRIGHT;
			term->wrap_occured = false;
		} else if (mode == 25) {
			term->mode &= ~MODE_CURSOR;
		} else if (mode == 8901) {
			term->mode &= ~MODE_VWBS;
		}
	}

}

void set_margin(struct terminal_t *term, struct parm_t *parm)
{
	int top, bottom;

	if (parm->argc <= 0) {        /* CSI r */
		top    = 0;
		bottom = term->lines - 1;
	} else if (parm->argc == 2) { /* CSI ; r -> use default value */
		top    = (parm->argv[0] == NULL) ? 0: dec2num(parm->argv[0]) - 1;
		bottom = (parm->argv[1] == NULL) ? term->lines - 1: dec2num(parm->argv[1]) - 1;
	} else {
		return;
	}

	if (top < 0 || top >= term->lines)
		top = 0;
	if (bottom < 0 || bottom >= term->lines)
		bottom = term->lines - 1;

	if (top >= bottom)
		return;

	term->scroll.top = top;
	term->scroll.bottom = bottom;

	set_cursor(term, 0, 0); /* move cursor to home */
}

void clear_tabstop(struct terminal_t *term, struct parm_t *parm)
{
	int i, j, num;

	if (parm->argc <= 0) {
		term->tabstop[term->cursor.x] = false;
	} else {
		for (i = 0; i < parm->argc; i++) {
			num = dec2num(parm->argv[i]);
			if (num == 0) {
				term->tabstop[term->cursor.x] = false;
			} else if (num == 3) {
				for (j = 0; j < term->cols; j++)
					term->tabstop[j] = false;
				return;
			}
		}
	}
}
