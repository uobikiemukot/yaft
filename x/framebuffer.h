/* See LICENSE for licence details. */
void fb_init(struct framebuffer *fb)
{
	XTextProperty title;

	if ((fb->dpy = XOpenDisplay(NULL)) == NULL)
		fatal("XOpenDisplay failed");

	fb->sc = DefaultScreen(fb->dpy);

	fb->win = XCreateSimpleWindow(fb->dpy, DefaultRootWindow(fb->dpy),
		0, 0, WIDTH, HEIGHT, 0, color_list[DEFAULT_FG], color_list[DEFAULT_BG]);

	title.value = (unsigned char *) "yaftx";
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = 5;
	XSetWMProperties(fb->dpy, fb->win, &title, NULL, NULL, 0, NULL, NULL, NULL);

	fb->gc = XCreateGC(fb->dpy, fb->win, 0, NULL);

	fb->res.x = WIDTH;
	fb->res.y = HEIGHT;

	fb->buf = XCreatePixmap(fb->dpy, fb->win, WIDTH, HEIGHT, XDefaultDepth(fb->dpy, fb->sc));
	XSelectInput(fb->dpy, fb->win, FocusChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);

	XMapWindow(fb->dpy, fb->win);
}

void fb_die(struct framebuffer *fb)
{
	XFreeGC(fb->dpy, fb->gc);
	XFreePixmap(fb->dpy, fb->buf);
	XDestroyWindow(fb->dpy, fb->win);
	XCloseDisplay(fb->dpy);
}

void set_bitmap(struct framebuffer *fb, struct terminal *term, int y, int x, int offset)
{
	int i, shift, glyph_width;
	struct color_pair color;
	struct cell *cp;
	const struct static_glyph_t *gp;
	extern struct tty_state tty;

	cp = &term->cells[x + y * term->cols];
	if (cp->wide == NEXT_TO_WIDE)
		return;

	gp = cp->gp;
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
			XSetForeground(fb->dpy, fb->gc, color_list[color.fg]);
			XDrawPoint(fb->dpy, fb->buf, fb->gc, cell_width * x + i, cell_height * y + offset);
		}
		else if (color.bg != DEFAULT_BG) {
			XSetForeground(fb->dpy, fb->gc, color_list[color.bg]);
			XDrawPoint(fb->dpy, fb->buf, fb->gc, cell_width * x + i, cell_height * y + offset);
		}
	}
}

void draw_line(struct framebuffer *fb, struct terminal *term, int y)
{
	int offset, x;

	XSetForeground(fb->dpy, fb->gc, color_list[DEFAULT_BG]);
	XFillRectangle(fb->dpy, fb->buf, fb->gc, 0, y * cell_height, term->width, cell_height);

	for (offset = 0; offset < cell_height; offset++) {
		for (x = 0; x < term->cols; x++)
			set_bitmap(fb, term, y, x, offset);
	}
	term->line_dirty[y] = (term->mode & MODE_CURSOR && term->cursor.y == y) ? true: false;

	XCopyArea(fb->dpy, fb->buf, fb->win, fb->gc, 0, y * cell_height,
		term->width, cell_height, 0, y * cell_height);
}

void refresh(struct framebuffer *fb, struct terminal *term)
{
	int y;

	if (term->mode & MODE_CURSOR)
		term->line_dirty[term->cursor.y] = true;

	for (y = 0; y < term->lines; y++) {
		if (term->line_dirty[y])
			draw_line(fb, term, y);
	}
}
