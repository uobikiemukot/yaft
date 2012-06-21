void init_cell(cell * cp)
{
	cp->code = DEFAULT_CHAR;
	cp->color.fg = DEFAULT_FG;
	cp->color.bg = DEFAULT_BG;
	cp->attribute = RESET;
	cp->wide = HALF;
}

int set_cell(terminal * term, int y, int x, u16 code)
{
	cell new_cell, *cp;
	glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	gp = term->fonts[code];

	new_cell.code = code;
	new_cell.color.fg = term->color.fg;
	new_cell.color.bg = term->color.bg;
	new_cell.attribute = term->attribute;
	new_cell.wide = (gp->size.x > term->cell_size.x) ? WIDE : HALF;

	*cp = new_cell;
	term->line_dirty[y] = true;

	if (new_cell.wide == WIDE && x + 1 < term->cols) {
		cp = &term->cells[x + 1 + y * term->cols];
		*cp = new_cell;
		cp->wide = NEXT_TO_WIDE;
		return WIDE;
	}

	return HALF;
}

void scroll(terminal * term, int from, int to, int offset)
{
	int i, j, size, abs_offset;
	cell *dst, *src;

	if (offset == 0 || from >= to)
		return;

	if (DEBUG)
		fprintf(stderr, "scroll from:%d to:%d offset:%d\n", from, to,
				offset);

	for (i = from; i <= to; i++)
		term->line_dirty[i] = true;

	abs_offset = abs(offset);
	size = sizeof(cell) * ((to - from + 1) - abs_offset) * term->cols;
	dst = term->cells + from * term->cols;
	src = term->cells + (from + abs_offset) * term->cols;

	if (offset > 0) {
		memmove(dst, src, size);
		for (i = (to - offset + 1); i <= to; i++)
			for (j = 0; j < term->cols; j++)
				init_cell(&term->cells[j + i * term->cols]);
	} else {
		memmove(src, dst, size);
		for (i = from; i < from + abs_offset; i++)
			for (j = 0; j < term->cols; j++)
				init_cell(&term->cells[j + i * term->cols]);
	}
}

/* relative movement: causes scroll */
void move_cursor(terminal * term, int y_offset, int x_offset)
{
	int x, y, top, bottom;

	x = term->cursor.x + x_offset;

	if (x < 0)
		x = 0;
	else if (x >= term->cols) {
		if (term->mode & AMRIGHT)
			term->wrap = true;
		x = term->cols - 1;
	}

	term->cursor.x = x;

	y = term->cursor.y + y_offset;
	top = term->scroll.top;
	bottom = term->scroll.bottom;

	y = (y < 0) ? 0 : (y >= term->lines) ? term->lines - 1 : y;

	if (term->cursor.y == top && y_offset < 0) {
		y = top;
		scroll(term, top, bottom, y_offset);
	} else if (term->cursor.y == bottom && y_offset > 0) {
		y = bottom;
		scroll(term, top, bottom, y_offset);
	}

	term->cursor.y = y;
}

/* absolute movement: never scroll */
void set_cursor(terminal * term, int y, int x)
{
	int top, bottom;

	top = 0;
	bottom = term->lines - 1;

	if (term->mode & ORIGIN) {
		top = term->scroll.top;
		bottom = term->scroll.bottom;
		y += term->scroll.top;
	}

	x = (x < 0) ? 0 : (x >= term->cols) ? term->cols - 1 : x;

	y = (y < top) ? top : (y > bottom) ? bottom : y;

	term->cursor.x = x;
	term->cursor.y = y;
}

void addch(terminal * term, u32 code)
{
	glyph_t *gp;

	if (code == DEL)			/* ignore DEL */
		return;

	if (code >= UCS_CHARS		/* only handle UCS2 (<= 0xFFFF) */
		|| term->fonts[code] == NULL)	/* glyph not found */
		code = DEFAULT_CHAR;

	gp = term->fonts[code];		/* folding */
	if ((term->wrap && term->cursor.x == term->cols - 1)
		|| (gp->size.x > term->cell_size.x
			&& term->cursor.x == term->cols - 1)) {
		set_cursor(term, term->cursor.y, 0);
		move_cursor(term, 1, 0);
	}
	term->wrap = false;

	move_cursor(term, 0,
				set_cell(term, term->cursor.y, term->cursor.x, code));
}

void writeback(int fd, char *buf, int size)
{
	if (size > 0)
		ewrite(fd, (u8 *) buf, size);
}

void reset_esc(terminal * term)
{
	escape *ep;
	ep = &term->esc;

	memset(ep->buf, '\0', BUFSIZE);
	ep->bp = ep->buf;
	ep->state = RESET;
}

int push_esc(terminal * term, u8 c)
{
	escape *ep;
	ep = &term->esc;

	if (ep->bp == &ep->buf[BUFSIZE]	/* buffer limit */
		||c == 0x18 || c == 0x1A) {	/* CAN or SUB */
		reset_esc(term);
		return false;
	}

	*ep->bp++ = c;
	if (ep->state == CSI && ('@' <= c && c <= '~'))
		return true;
	else if (ep->state == OSC && c == 0x07)
		return true;
	else if (ep->state == OSC && strlen((char *) ep->buf) > 1
			 && *(ep->bp - 2) == 0x1B && c == 0x5C)
		return true;
	else
		return false;
}

void reset_ucs(terminal * term)
{
	term->ucs.code = term->ucs.count = term->ucs.length = 0;
}

void reset_state(terminal * term)
{
	term->save_state.cursor.x = term->save_state.cursor.y = 0;
	term->save_state.attribute = RESET;
	term->save_state.mode = RESET;
}

void reset(terminal * term)
{
	int i, j;
	cell *cp;

	term->mode = RESET;
	term->mode |= (CURSOR | AMRIGHT);
	term->wrap = false;

	term->scroll.top = 0;
	term->scroll.bottom = term->lines - 1;

	term->cursor.x = term->cursor.y = 0;

	term->color.fg = DEFAULT_FG;
	term->color.bg = DEFAULT_BG;

	term->attribute = RESET;

	for (i = 0; i < term->lines; i++) {
		for (j = 0; j < term->cols; j++) {
			cp = &term->cells[j + i * term->cols];
			init_cell(cp);
			if ((j % TABSTOP) == 0)
				term->tabstop[j] = true;
			else
				term->tabstop[j] = false;
		}
		term->line_dirty[i] = true;
	}

	reset_state(term);
	reset_esc(term);
	reset_ucs(term);
}

void resize(terminal * term, int lines, int cols)
{
	winsize size;

	free(term->cells);

	term->lines = lines;
	term->cols = cols;
	term->width = term->cols * term->cell_size.x;
	term->height = term->lines * term->cell_size.y;

	term->cells = (cell *)
		emalloc(sizeof(cell) * term->cols * term->lines);

	reset(term);

	size.ws_row = lines;
	size.ws_col = cols;
	size.ws_xpixel = size.ws_ypixel = 0;
	eioctl(term->fd, TIOCSWINSZ, &size);
}

void term_init(terminal * term, framebuffer * fb)
{
	glyph_t *gp;

	load_fonts(term->fonts, font_path);

	gp = term->fonts[DEFAULT_CHAR];
	term->cell_size.x = gp->size.x;
	term->cell_size.y = gp->size.y;

	term->width = TERM_WIDTH;
	term->height = TERM_HEIGHT;

	term->offset.x = OFFSET_X;
	term->offset.y = OFFSET_Y;

	if (term->width + term->offset.x > fb->res.x
		|| term->height + term->offset.y > fb->res.y
		|| term->width < term->cell_size.x
		|| term->height < term->cell_size.y) {
		if (DEBUG)
			fprintf(stderr,
					"invalid termnal size %dx%d: use screen size %dx%d\n",
					term->width, term->height, fb->res.x, fb->res.y);
		term->width = fb->res.x;
		term->height = fb->res.y;
		term->offset.x = 0;
		term->offset.y = 0;
	}

	term->cols = term->width / term->cell_size.x;
	term->lines = term->height / term->cell_size.y;

	if (DEBUG)
		fprintf(stderr, "width:%d height:%d cols:%d lines:%d\n",
				term->width, term->height, term->cols, term->lines);

	term->wall =
		(WALLPAPER) ? load_wallpaper(fb, term->width, term->height) : NULL;

	term->line_dirty = (bool *) emalloc(sizeof(bool) * term->lines);

	term->tabstop = (bool *) emalloc(sizeof(bool) * term->cols);

	term->cells = (cell *)
		emalloc(sizeof(cell) * term->cols * term->lines);

	reset(term);

	writeback(STDIN_FILENO, "[?25l", 6);	/* cusor hide */
}

void term_die(terminal * term)
{
	int i;

	for (i = 0; i < UCS_CHARS; i++) {
		if (term->fonts[i] != NULL)
			free(term->fonts[i]->bitmap);
		free(term->fonts[i]);
	}

	free(term->wall);

	free(term->line_dirty);

	free(term->cells);

	writeback(STDIN_FILENO, "[?25h", 6);	/* cursor visible */
}
