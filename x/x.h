/* See LICENSE for licence details. */
void x_fatal(char *str)
{
	fprintf(stderr, "%s\n", str);
	exit(EXIT_FAILURE);
}

void x_init(xwindow *xw)
{
	XTextProperty title;
	int width = 640, height= 384;

	if ((xw->dsp = XOpenDisplay("")) == NULL)
		x_fatal("can't open display");

	xw->sc = DefaultScreen(xw->dsp);

	xw->win = XCreateSimpleWindow(xw->dsp, DefaultRootWindow(xw->dsp),
		0, 0, width, height,
		0, color_palette[DEFAULT_FG], color_palette[DEFAULT_BG]);

	title.value = (unsigned char *) "yaft";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 5;
	XSetWMProperties(xw->dsp, xw->win, &title, NULL, NULL, 0, NULL, NULL, NULL);

	xw->gc = XCreateGC(xw->dsp, xw->win, 0, NULL);

	xw->res.x = width;
	xw->res.y = height;

	xw->buf = XCreatePixmap(xw->dsp, xw->win,
		width, height,
		XDefaultDepth(xw->dsp, xw->sc));
	
	XSelectInput(xw->dsp, xw->win,
		KeyPressMask | StructureNotifyMask | ExposureMask);

	XMapWindow(xw->dsp, xw->win);
}

void x_die(xwindow *xw)
{
	XFreeGC(xw->dsp, xw->gc); // SEGV!!
	XFreePixmap(xw->dsp, xw->buf);
	XDestroyWindow(xw->dsp, xw->win);
	XCloseDisplay(xw->dsp);
}

void set_bitmap(xwindow *xw, terminal *term, int y, int x, int offset)
{
	int i, shift;
	color_pair color;
	cell *cp;
	glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = term->fonts[cp->code];
	shift = ceil((double) gp->size.x / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	/* attribute */
	if ((cp->attribute & attr_mask[UNDERLINE])
		&& (offset == (term->cell_size.y - 1)))
		color.bg = color.fg;

	for (i = 0; i < gp->size.x; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1))) {
			XSetForeground(xw->dsp, xw->gc, color_palette[color.fg]);
			XDrawPoint(xw->dsp, xw->buf, xw->gc,
				term->cell_size.x * x + i, term->cell_size.y * y + offset);
		}
		else if (color.bg != DEFAULT_BG) {
			XSetForeground(xw->dsp, xw->gc, color_palette[color.bg]);
			XDrawPoint(xw->dsp, xw->buf, xw->gc,
				term->cell_size.x * x + i, term->cell_size.y * y + offset);
		}
	}
}

void draw_line(xwindow *xw, terminal *term, int line)
{
	int i, j;

	XSetForeground(xw->dsp, xw->gc, color_palette[DEFAULT_BG]);
	XFillRectangle(xw->dsp, xw->buf, xw->gc,
		0, line * term->cell_size.y,
		term->width, term->cell_size.y);

	for (i = 0; i < term->cell_size.y; i++) {
		for (j = 0; j < term->cols; j++)
			set_bitmap(xw, term, line, j, i);
	}
	term->line_dirty[line] = false;

	XCopyArea(xw->dsp, xw->buf, xw->win, xw->gc,
		0, line * term->cell_size.y,
		term->width, term->cell_size.y,
		0, line * term->cell_size.y);
}

void draw_curs(xwindow *xw, terminal *term)
{
	int i, width;
	color_pair store_color;
	pair store_curs;
	u8 store_attr;
	cell *cp;

	store_curs = term->cursor;
	if (term->cells[term->cursor.x + term->cursor.y * term->cols].wide == NEXT_TO_WIDE)
		term->cursor.x--;

	cp = &term->cells[term->cursor.x + term->cursor.y * term->cols];
	store_color = cp->color;
	cp->color.fg = DEFAULT_BG;
	cp->color.bg = CURSOR_COLOR;

	store_attr = cp->attribute;
	cp->attribute = RESET;

	width = (cp->wide == WIDE) ? 
		term->cell_size.x * 2: term->cell_size.x;

	for (i = 0; i < term->cell_size.y; i++)
		set_bitmap(xw, term, term->cursor.y, term->cursor.x, i);

	XCopyArea(xw->dsp, xw->buf, xw->win, xw->gc,
				term->cursor.x * term->cell_size.x, term->cursor.y * term->cell_size.y,
				width, term->cell_size.y,
				term->cursor.x * term->cell_size.x, term->cursor.y * term->cell_size.y);

	cp->color = store_color;
	cp->attribute = store_attr;
	term->cursor = store_curs;
	term->line_dirty[term->cursor.y] = true;
}

void refresh(xwindow *xw, terminal *term)
{
	int i;

	for (i = 0; i < term->lines; i++) {
		if (term->line_dirty[i])
			draw_line(xw, term, i);
	}

	if (term->mode & CURSOR)
		draw_curs(xw, term);
}
