/* See LICENSE for licence details. */
void erase_cell(struct terminal *term, int y, int x)
{
	struct cell *cp;

	cp = &term->cells[x + y * term->cols];
	cp->gp = term->glyph_map[DEFAULT_CHAR];
	cp->color = term->color; /* bce */
	cp->attribute = RESET;
	cp->width = HALF;

	term->line_dirty[y] = true;
}

void copy_cell(struct terminal *term, int dst_y, int dst_x, int src_y, int src_x)
{
	struct cell *dst, *src;

	dst = &term->cells[dst_x + dst_y * term->cols];
	src = &term->cells[src_x + src_y * term->cols];

	if (src->width == NEXT_TO_WIDE)
		return;
	else if (src->width == WIDE && dst_x == (term->cols - 1))
		erase_cell(term, dst_y, dst_x);
	else {
		*dst = *src;
		if (src->width == WIDE) {
			*(dst + 1) = *src;
			(dst + 1)->width = NEXT_TO_WIDE;
		}
		term->line_dirty[dst_y] = true;
	}
}

void swap_color(struct color_pair *cp)
{
	int tmp;

	tmp = cp->fg;
	cp->fg = cp->bg;
	cp->bg = tmp;
}

int set_cell(struct terminal *term, int y, int x, const struct static_glyph_t *gp)
{
	struct cell nc, *cp;

	cp = &term->cells[x + y * term->cols];
	nc.gp = gp;
	nc.color.fg = (term->attribute & attr_mask[BOLD] && term->color.fg <= 7) ?
		term->color.fg + BRIGHT_INC: term->color.fg;
	nc.color.bg = (term->attribute & attr_mask[BLINK] && term->color.bg <= 7) ?
		term->color.bg + BRIGHT_INC: term->color.bg;

	if (term->attribute & attr_mask[REVERSE])
		swap_color(&nc.color);

	nc.attribute = term->attribute;
	nc.width = gp->width;

	*cp = nc;
	term->line_dirty[y] = true;

	if (nc.width == WIDE && x + 1 < term->cols) {
		cp = &term->cells[x + 1 + y * term->cols];
		*cp = nc;
		cp->width = NEXT_TO_WIDE;
		return WIDE;
	}

	return HALF;
}

void scroll(struct terminal *term, int from, int to, int offset)
{
	int i, j, size, abs_offset;
	struct cell *dst, *src;

	if (offset == 0 || from >= to)
		return;

	if (DEBUG)
		fprintf(stderr, "scroll from:%d to:%d offset:%d\n", from, to, offset);

	for (i = from; i <= to; i++)
		term->line_dirty[i] = true;

	abs_offset = abs(offset);
	size = sizeof(struct cell) * ((to - from + 1) - abs_offset) * term->cols;

	dst = term->cells + from * term->cols;
	src = term->cells + (from + abs_offset) * term->cols;

	if (offset > 0) {
		memmove(dst, src, size);
		for (i = (to - offset + 1); i <= to; i++)
			for (j = 0; j < term->cols; j++)
				erase_cell(term, i, j);
	}
	else {
		memmove(src, dst, size);
		for (i = from; i < from + abs_offset; i++)
			for (j = 0; j < term->cols; j++)
				erase_cell(term, i, j);
	}
}

/* relative movement: cause scrolling */
void move_cursor(struct terminal *term, int y_offset, int x_offset)
{
	int x, y, top, bottom;

	x = term->cursor.x + x_offset;
	y = term->cursor.y + y_offset;

	top = term->scroll.top;
	bottom = term->scroll.bottom;

	if (x < 0)
		x = 0;
	else if (x >= term->cols) {
		if (term->mode & MODE_AMRIGHT)
			term->wrap = true;
		x = term->cols - 1;
	}
	term->cursor.x = x;

	y = (y < 0) ? 0:
		(y >= term->lines) ? term->lines - 1: y;

	if (term->cursor.y == top && y_offset < 0) {
		y = top;
		scroll(term, top, bottom, y_offset);
	}
	else if (term->cursor.y == bottom && y_offset > 0) {
		y = bottom;
		scroll(term, top, bottom, y_offset);
	}
	term->cursor.y = y;
}

/* absolute movement: never scroll */
void set_cursor(struct terminal *term, int y, int x)
{
	int top, bottom;

	if (term->mode & MODE_ORIGIN) {
		top = term->scroll.top;
		bottom = term->scroll.bottom;
		y += term->scroll.top;
	}
	else {
		top = 0;
		bottom = term->lines - 1;
	}

	x = (x < 0) ? 0: (x >= term->cols) ? term->cols - 1: x;
	y = (y < top) ? top: (y > bottom) ? bottom: y;

	term->cursor.x = x;
	term->cursor.y = y;
}

void addch(struct terminal *term, uint32_t code)
{
	int width, drcs_ku, drcs_ten;
	const struct static_glyph_t *gp;

	if (DEBUG)
		fprintf(stderr, "addch: U+%.4X\n", code);

	width = wcwidth(code);

	if (width <= 0) /* zero width */
		return;
	else if (0x100000 <= code && 0x10FFFD) { /* Unicode private area: plane 16 */
		/* DRCSMMv1
			ESC ( SP <\xXX> <\xYY> ESC ( B
			<===> U+10XXYY ( 0x40 <= 0xXX <=0x7E, 0x20 <= 0xYY <= 0x7F )
		*/
		drcs_ku = (0xFF00 & code) >> 8;
		drcs_ten = 0xFF & code;

		if (DEBUG)
			fprintf(stderr, "ku:0x%.2X ten:0x%.2X\n", drcs_ku, drcs_ten);

		if ((0x40 <= drcs_ku && drcs_ku <= 0x7E)
			&& (0x20 <= drcs_ten && drcs_ten <= 0x7F)
			&& (term->drcs[drcs_ku - 0x40].chars != NULL))
			gp = &term->drcs[drcs_ku - 0x40].chars[drcs_ten - 0x20]; /* sub each offset */
		else {
			fprintf(stderr, "drcs char not found\n");
			gp = term->glyph_map[SUBSTITUTE_HALF];
		}
	}
	else if (code >= UCS2_CHARS /* yaft support only UCS2 */
		|| term->glyph_map[code] == NULL /* missing glyph */
		|| term->glyph_map[code]->width != width) /* width unmatch */
		gp = (width == 1) ? term->glyph_map[SUBSTITUTE_HALF]: term->glyph_map[SUBSTITUTE_WIDE];
	else
		gp = term->glyph_map[code];

	if ((term->wrap && term->cursor.x == term->cols - 1) /* folding */
		|| (gp->width == WIDE && term->cursor.x == term->cols - 1)) {
		set_cursor(term, term->cursor.y, 0);
		move_cursor(term, 1, 0);
	}
	term->wrap = false;

	move_cursor(term, 0, set_cell(term, term->cursor.y, term->cursor.x, gp));
}

void reset_esc(struct terminal *term)
{
	if (DEBUG)
		fprintf(stderr, "*esc reset*\n");
	memset(term->esc.buf, '\0', BUFSIZE);
	term->esc.bp = term->esc.buf;
	term->esc.state = STATE_RESET;
}

bool is_string_terminator(struct esc_t *esc, uint8_t ch)
{
	/* OSC/DCS ST: BELL or ESC \ */
	if (ch == BEL)
		return true;

	if (ch == BACKSLASH && strlen(esc->buf) >= 2 && *(esc->bp - 2) == ESC)
		return true;

	return false;
}

bool push_esc(struct terminal *term, uint8_t ch)
{
	if (term->esc.bp == &term->esc.buf[MAX_ESC_LENGTH - 1]) { /* buffer limit */
		reset_esc(term);
		return false;
	}

	*term->esc.bp++ = ch;
	if (term->esc.state == STATE_ESC) {
		if ('0' <= ch && ch <= '~')
			return true;
		else if (SPACE > ch || ch > '/')
			reset_esc(term);
	}
	else if (term->esc.state == STATE_CSI) {
		if ('@' <= ch && ch <= '~')
			return true;
		else if (SPACE > ch || ch > '?')
			reset_esc(term);
	}
	else if (term->esc.state == STATE_OSC) {
		if (is_string_terminator(&term->esc, ch))
			return true;
		else if ((ch != ESC) && (ch < SPACE || ch >= DEL))
			reset_esc(term);
	}
	else if (term->esc.state == STATE_DCS) {
		if (is_string_terminator(&term->esc, ch))
			return true;
		else if ((ch != ESC) && (ch < SPACE || ch >= DEL))
			reset_esc(term);
	}
	return false;
}

void reset_charset(struct terminal *term)
{
	term->charset.code = term->charset.count = term->charset.following_byte = 0;
	term->charset.is_valid = true;
}

void redraw(struct terminal *term)
{
	int i;

	for (i = 0; i < term->lines; i++)
		term->line_dirty[i] = true;
}

void reset(struct terminal *term)
{
	int i, j;

	term->mode = RESET;
	term->mode |= (MODE_CURSOR | MODE_AMRIGHT);
	term->wrap = false;

	term->scroll.top = 0;
	term->scroll.bottom = term->lines - 1;

	term->cursor.x = term->cursor.y = 0;

	term->state.mode = term->mode;
	term->state.cursor = term->cursor;
	term->state.attribute = RESET;

	term->color.fg = DEFAULT_FG;
	term->color.bg = DEFAULT_BG;

	term->attribute = RESET;

	for (i = 0; i < term->lines; i++) {
		for (j = 0; j < term->cols; j++) {
			erase_cell(term, i, j);
			if ((j % TABSTOP) == 0)
				term->tabstop[j] = true;
			else
				term->tabstop[j] = false;
		}
		term->line_dirty[i] = true;
	}

	reset_esc(term);
	reset_charset(term);
}

void swap_resolution(int *a, int *b)
{
	int tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void term_init(struct terminal *term, int width, int height)
{
	int i;
	uint32_t code, gi;

	term->width = width;
	term->height = height;

	term->cols = term->width / CELL_WIDTH;
	term->lines = term->height / CELL_HEIGHT;

	if (DEBUG)
		fprintf(stderr, "width:%d height:%d cols:%d lines:%d\n",
			width, height, term->cols, term->lines);

	term->line_dirty = (bool *) ecalloc(sizeof(bool) * term->lines);
	term->tabstop = (bool *) ecalloc(sizeof(bool) * term->cols);
	term->cells = (struct cell *) ecalloc(sizeof(struct cell) * term->cols * term->lines);

	/* initialize glyph map */
	for (code = 0; code < UCS2_CHARS; code++)
		term->glyph_map[code] = NULL;

	for (gi = 0; gi < sizeof(glyphs) / sizeof(struct static_glyph_t); gi++)
		term->glyph_map[glyphs[gi].code] = &glyphs[gi];

	if (term->glyph_map[DEFAULT_CHAR] == NULL
		|| term->glyph_map[SUBSTITUTE_HALF] == NULL
		|| term->glyph_map[SUBSTITUTE_WIDE] == NULL)
		fatal("cannot find DEFAULT_CHAR or SUBSTITUTE_HALF or SUBSTITUTE_HALF\n");

	/* initialize drcs */
	for (i = 0; i < DRCS_CHARSETS; i++)
		term->drcs[i].chars = NULL;

	reset(term);
}

void term_die(struct terminal *term)
{
	free(term->line_dirty);
	free(term->tabstop);
	free(term->cells);
}
