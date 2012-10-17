/* See LICENSE for licence details. */
void (*ctrl_func[CTRL_CHARS])(terminal * term, void *arg) = {
	[BS] = bs,
	[HT] = tab,
	[LF] = nl,
	[VT] = nl,
	[FF] = nl,
	[CR] = cr,
	[ESC] = enter_esc,
};

void (*esc_func[ESC_CHARS])(terminal * term, void *arg) = {
	['7'] = save_state,
	['8'] = restore_state,
	['D'] = nl,
	['E'] = crnl,
	['H'] = set_tabstop,
	['M'] = reverse_nl,
	['Z'] = identify,
	['['] = enter_csi,
	[']'] = enter_osc,
	['c'] = ris,
};

void (*csi_func[ESC_CHARS])(terminal * term, void *arg) = {
	['@'] = insert_blank,
	['A'] = curs_up,
	['B'] = curs_down,
	['C'] = curs_forward,
	['D'] = curs_back,
	['E'] = curs_nl,
	['F'] = curs_pl,
	['G'] = curs_col,
	['H'] = curs_pos,
	['J'] = erase_display,
	['K'] = erase_line,
	['L'] = insert_line,
	['M'] = delete_line,
	['P'] = delete_char,
	['X'] = erase_char,
	['a'] = curs_forward,
	['c'] = identify,
	['d'] = curs_line,
	['e'] = curs_down,
	['f'] = curs_pos,
	['g'] = clear_tabstop,
	['h'] = set_mode,
	['l'] = reset_mode,
	['m'] = set_attr,
	['n'] = status_report,
	['r'] = set_margin,
	['s'] = save_state,
	['u'] = restore_state,
	['`'] = curs_col,
};

void control_character(terminal *term, u8 ch)
{
	const char *ctrl_char[] = {
		"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
		"BS ", "HT ", "LF ", "VT ", "FF ", "CR ", "SO ", "SI ",
		"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
		"CAN", "EM ", "SUB", "ESC", "FS ", "GS ", "RS ", "US ",
	};

	if (DEBUG)
		fprintf(stderr, "ctl: %s\n", ctrl_char[ch]);

	if (ctrl_func[ch])
		ctrl_func[ch](term, NULL);
}

void esc_sequence(terminal *term, u8 ch)
{
	if (DEBUG)
		fprintf(stderr, "esc: ESC %s\n", term->esc.buf);

	if (strlen((char *) term->esc.buf) == 1 && esc_func[ch])
		esc_func[ch](term, NULL);

	if (ch != '[' && ch != ']')
		reset_esc(term);
}

void csi_sequence(terminal *term, u8 ch)
{
	parm_t parm;

	if (DEBUG)
		fprintf(stderr, "csi: CSI %s\n", term->esc.buf);

	reset_parm(&parm);
	parse_arg(term->esc.buf, &parm, ';', isdigit);
	*(term->esc.bp - 1) = '\0'; /* omit final character */

	if (csi_func[ch])
		csi_func[ch](term, &parm);

	reset_esc(term);
}

void osc_sequence(terminal *term, u8 ch)
{
	if (DEBUG)
		fprintf(stderr, "osc: OSC %s\n", term->esc.buf);

	reset_esc(term);
}

void utf8_character(terminal *term, u8 ch)
{
	if (0xC2 <= ch && ch <= 0xDF) {
		term->ucs.length = 1;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x1F;
		return;
	}
	else if (0xE0 <= ch && ch <= 0xEF) {
		term->ucs.length = 2;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x0F;
		return;
	}
	else if (0xF0 <= ch && ch <= 0xF4) {
		term->ucs.length = 3;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x07;
		return;
	}
	else if (0x80 <= ch && ch <= 0xBF) {
		if (term->ucs.length == 0) {
			/* some DEC terminals (ex VT320) recognize following C1 (8bit) contorol character,
				but these are not implemented in yaft.
				0x84 IND 0x85 NEL 0x88 HTS 0x8D RI  0x8E SS2 0x8F SS3
				0x90 DCS 0x9B CSI 0x9C ST  0x9D OSC 0x9E PM  0x9F APC */
			reset_ucs(term);
			return;
		}
		term->ucs.code <<= 6;
		term->ucs.code += ch & 0x3F;
		term->ucs.count++;
	}
	else { /* 0xC0 - 0xC1, 0xF5 - 0xFF: illegal byte */
		reset_ucs(term);
		return;
	}

	if (term->ucs.count >= term->ucs.length) {
		if (DEBUG)
			fprintf(stderr, "unicode: U+%4.4X\n", term->ucs.code);
		addch(term, term->ucs.code);
		reset_ucs(term);
	}
}

void parse(terminal *term, u8 *buf, int size)
{
	u8 ch;
	int i;
	/*
		CTRL CHARS      : 0x00 ~ 0x1F
		ASCII(printable): 0x20 ~ 0x7E
		CTRL CHARS(DEL) : 0x7F
		UTF-8           : 0x80 ~ 0xFF
	*/

	for (i = 0; i < size; i++) {
		ch = buf[i];
		if (term->esc.state == RESET) {
			if (ch <= 0x1F)
				control_character(term, ch);
			else if (ch <= 0x7E) {
				if (DEBUG)
					fprintf(stderr, "ascii: %c\n", ch);
				addch(term, ch);
			}
			else if (ch == DEL)
				continue;
			else
				utf8_character(term, ch);
		}
		else if (term->esc.state == STATE_ESC) {
			if (push_esc(term, ch))
				esc_sequence(term, ch);
		}
		else if (term->esc.state == STATE_CSI) {
			if (push_esc(term, ch))
				csi_sequence(term, ch);
		}
		else if (term->esc.state == STATE_OSC) {
			if (push_esc(term, ch))
				osc_sequence(term, ch);
		}
	}
}
