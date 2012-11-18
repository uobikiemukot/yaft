/* See LICENSE for licence details. */
void x_init(struct xwindow *xw)
{
	XTextProperty title;

	if ((xw->dsp = XOpenDisplay(NULL)) == NULL) {
		fprintf(stderr, "XOpenDisplay failed\n");
		exit(EXIT_FAILURE);
	}

	xw->sc = DefaultScreen(xw->dsp);

	xw->win = XCreateSimpleWindow(xw->dsp, DefaultRootWindow(xw->dsp),
		0, 0, WIDTH, HEIGHT, 0, color_list[DEFAULT_FG], color_list[DEFAULT_BG]);

	title.value = (unsigned char *) "yaft";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 5;
	XSetWMProperties(xw->dsp, xw->win, &title, NULL, NULL, 0, NULL, NULL, NULL);

	xw->gc = XCreateGC(xw->dsp, xw->win, 0, NULL);

	xw->res.x = WIDTH;
	xw->res.y = HEIGHT;

	xw->buf = XCreatePixmap(xw->dsp, xw->win,
		WIDTH, HEIGHT, XDefaultDepth(xw->dsp, xw->sc));
	
	XSelectInput(xw->dsp, xw->win, ExposureMask | KeyPressMask | StructureNotifyMask);

	XMapWindow(xw->dsp, xw->win);
}

void x_die(struct xwindow *xw)
{
	XFreeGC(xw->dsp, xw->gc);
	XFreePixmap(xw->dsp, xw->buf);
	XDestroyWindow(xw->dsp, xw->win);
	XCloseDisplay(xw->dsp);
}

void set_bitmap(struct xwindow *xw, struct terminal *term, int y, int x, int offset)
{
	int i, shift, glyph_width;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = &fonts[cp->code];
	glyph_width = gp->width * cell_width;
	shift = ((glyph_width + BITS_PER_BYTE - 1) / BITS_PER_BYTE) * BITS_PER_BYTE;
	color = cp->color;

	if ((term->mode & MODE_CURSOR && y == term->cursor.y) /* cursor */
		&& (x == term->cursor.x || (cp->wide == WIDE && (x + 1) == term->cursor.x))) {
		color.fg = DEFAULT_BG;
		color.bg = CURSOR_COLOR;
	}

	if ((offset == (cell_height - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;

	for (i = 0; i < glyph_width; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1))) {
			XSetForeground(xw->dsp, xw->gc, color_list[color.fg]);
			XDrawPoint(xw->dsp, xw->buf, xw->gc, cell_width * x + i, cell_height * y + offset);
		}
		else if (color.bg != DEFAULT_BG) {
			XSetForeground(xw->dsp, xw->gc, color_list[color.bg]);
			XDrawPoint(xw->dsp, xw->buf, xw->gc, cell_width * x + i, cell_height * y + offset);
		}
	}
}

void draw_line(struct xwindow *xw, struct terminal *term, int y)
{
	int offset, x;

	XSetForeground(xw->dsp, xw->gc, color_list[DEFAULT_BG]);
	XFillRectangle(xw->dsp, xw->buf, xw->gc, 0, y * cell_height, term->width, cell_height);

	for (offset = 0; offset < cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(xw, term, y, x, offset);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;

	XCopyArea(xw->dsp, xw->buf, xw->win, xw->gc, 0, y * cell_height,
		term->width, cell_height, 0, y * cell_height);
}

void refresh(struct xwindow *xw, struct terminal *term)
{
	int y;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (y = 0; y < term->lines; y++) {
		if (term->line_dirty[y])
			draw_line(xw, term, y);
	}
}
