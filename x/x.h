/* See LICENSE for licence details. */
void x_init(struct xwindow *xw)
{
	XTextProperty title;

	if ((xw->dpy = XOpenDisplay(NULL)) == NULL)
		fatal("XOpenDisplay failed");

	xw->sc = DefaultScreen(xw->dpy);

	xw->win = XCreateSimpleWindow(xw->dpy, DefaultRootWindow(xw->dpy),
		0, 0, WIDTH, HEIGHT, 0, color_list[DEFAULT_FG], color_list[DEFAULT_BG]);

	title.value = (unsigned char *) "yaftx";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 5;
	XSetWMProperties(xw->dpy, xw->win, &title, NULL, NULL, 0, NULL, NULL, NULL);

	xw->gc = XCreateGC(xw->dpy, xw->win, 0, NULL);

	xw->res.x = WIDTH;
	xw->res.y = HEIGHT;

	xw->buf = XCreatePixmap(xw->dpy, xw->win, WIDTH, HEIGHT, XDefaultDepth(xw->dpy, xw->sc));
	XSelectInput(xw->dpy, xw->win, FocusChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);

	XMapWindow(xw->dpy, xw->win);
}

void x_die(struct xwindow *xw)
{
	XFreeGC(xw->dpy, xw->gc);
	XFreePixmap(xw->dpy, xw->buf);
	XDestroyWindow(xw->dpy, xw->win);
	XCloseDisplay(xw->dpy);
}

void set_bitmap(struct xwindow *xw, struct terminal *term, int y, int x, int offset)
{
	int i, shift, glyph_width;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;
	extern struct tty_state tty;

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
		color.bg = (tty.visible) ? CURSOR_COLOR: CURSOR_INACTIVE_COLOR;
	}

	if ((offset == (cell_height - 1)) /* underline */
		&& (cp->attribute & attr_mask[UNDERLINE]))
		color.bg = color.fg;

	for (i = 0; i < glyph_width; i++) {
		if (gp->bitmap[offset] & (0x01 << (shift - i - 1))) {
			XSetForeground(xw->dpy, xw->gc, color_list[color.fg]);
			XDrawPoint(xw->dpy, xw->buf, xw->gc, cell_width * x + i, cell_height * y + offset);
		}
		else if (color.bg != DEFAULT_BG) {
			XSetForeground(xw->dpy, xw->gc, color_list[color.bg]);
			XDrawPoint(xw->dpy, xw->buf, xw->gc, cell_width * x + i, cell_height * y + offset);
		}
	}
}

void draw_line(struct xwindow *xw, struct terminal *term, int y)
{
	int offset, x;

	XSetForeground(xw->dpy, xw->gc, color_list[DEFAULT_BG]);
	XFillRectangle(xw->dpy, xw->buf, xw->gc, 0, y * cell_height, term->width, cell_height);

	for (offset = 0; offset < cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(xw, term, y, x, offset);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;

	XCopyArea(xw->dpy, xw->buf, xw->win, xw->gc, 0, y * cell_height,
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
