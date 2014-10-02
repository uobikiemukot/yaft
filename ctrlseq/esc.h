/* See LICENSE for licence details. */
/* function for control character */
void bs(struct terminal *term)
{
	move_cursor(term, 0, -1);
}

void tab(struct terminal *term)
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

void nl(struct terminal *term)
{
	move_cursor(term, 1, 0);
}

void cr(struct terminal *term)
{
	set_cursor(term, term->cursor.y, 0);
}

void enter_esc(struct terminal *term)
{
	term->esc.state = STATE_ESC;
}

/* function for escape sequence */
void save_state(struct terminal *term)
{
	term->state.mode = term->mode & MODE_ORIGIN;
	term->state.cursor = term->cursor;
	term->state.attribute = term->attribute;
}

void restore_state(struct terminal *term)
{
	/* restore state */
	if (term->state.mode & MODE_ORIGIN)
		term->mode |= MODE_ORIGIN;
	else
		term->mode &= ~MODE_ORIGIN;
	term->cursor    = term->state.cursor;
	term->attribute = term->state.attribute;
}

void crnl(struct terminal *term)
{
	cr(term);
	nl(term);
}

void set_tabstop(struct terminal *term)
{
	term->tabstop[term->cursor.x] = true;
}

void reverse_nl(struct terminal *term)
{
	move_cursor(term, -1, 0);
}

void identify(struct terminal *term)
{
	ewrite(term->fd, "\033[?6c", 5); /* "I am a VT102" */
}

void enter_csi(struct terminal *term)
{
	term->esc.state = STATE_CSI;
}

void enter_osc(struct terminal *term)
{
	term->esc.state = STATE_OSC;
}

void enter_dcs(struct terminal *term)
{
	term->esc.state = STATE_DCS;
}

void ris(struct terminal *term)
{
	reset(term);
}
