/* See LICENSE for licence details. */
void (*ctrl_func[CTRL_CHARS])(struct terminal * term, void *arg) = {
	[BS] = bs,
	[HT] = tab,
	[LF] = nl,
	[VT] = nl,
	[FF] = nl,
	[CR] = cr,
	[ESC] = enter_esc,
};

void (*esc_func[ESC_CHARS])(struct terminal * term, void *arg) = {
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

void (*csi_func[ESC_CHARS])(struct terminal * term, void *arg) = {
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

void reset_parm(struct parm_t *pt)
{
	int i;

	pt->argc = 0;
	for (i = 0; i < ESC_PARAMS; i++)
		pt->argv[i] = NULL;
}

void parse_arg(char *buf, struct parm_t *pt, int delim, int (is_valid)(int c))
{
	int length;
	char *cp;

	length = strlen(buf);
	cp = buf;

	while (cp < &buf[length - 1]) {
		if (*cp == delim)
			*cp = '\0';
		cp++;
	}

	cp = buf;
  start:
	if (pt->argc < ESC_PARAMS && is_valid(*cp)) {
		pt->argv[pt->argc] = cp;
		pt->argc++;
	}

	while (is_valid(*cp))
		cp++;

	while (!is_valid(*cp) && cp < &buf[length - 1])
		cp++;

	if (cp < &buf[length - 1])
		goto start;
}

void control_character(struct terminal *term, uint8_t ch)
{
	static const char *ctrl_char[] = {
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

void esc_sequence(struct terminal *term, uint8_t ch)
{
	if (DEBUG)
		fprintf(stderr, "esc: ESC %s\n", term->esc.buf);

	if (strlen(term->esc.buf) == 1 && esc_func[ch])
		esc_func[ch](term, NULL);

	if (ch != '[' && ch != ']')
		reset_esc(term);
}

void csi_sequence(struct terminal *term, uint8_t ch)
{
	struct parm_t parm;

	if (DEBUG)
		fprintf(stderr, "csi: CSI %s\n", term->esc.buf);

	reset_parm(&parm);
	parse_arg(term->esc.buf, &parm, ';', isdigit);
	*(term->esc.bp - 1) = '\0'; /* omit final character */

	if (csi_func[ch])
		csi_func[ch](term, &parm);

	reset_esc(term);
}

void osc_sequence(struct terminal *term, uint8_t ch)
{
	if (DEBUG)
		fprintf(stderr, "osc: OSC %s\n", term->esc.buf);

	reset_esc(term);
}

void utf8_character(struct terminal *term, uint8_t ch)
{
	if (0x80 <= ch && ch <= 0xBF) {
		/* check illegal UTF-8 sequence
			* yaft not recognize C1 (8bit) control character
			* 2 byte sequence: first byte must be between 0xC2 ~ 0xDF
 			* 3 byte sequence: second byte following 0xE0 must be between 0xA0 ~ 0xBF
 			* 4 byte sequence: second byte following 0xF0 must be between 0x90 ~ 0xBF
 			* 5 byte sequence: second byte following 0xF8 must be between 0x88 ~ 0xBF
 			* 6 byte sequence: second byte following 0xFC must be between 0x84 ~ 0xBF
		*/
		if (term->ucs.following_byte == 0
			|| (term->ucs.following_byte == 1 && term->ucs.count == 0 && term->ucs.code <= 1)
			|| (term->ucs.following_byte == 2 && term->ucs.count == 0 && term->ucs.code == 0 && ch < 0xA0)
			|| (term->ucs.following_byte == 3 && term->ucs.count == 0 && term->ucs.code == 0 && ch < 0x90)
			|| (term->ucs.following_byte == 4 && term->ucs.count == 0 && term->ucs.code == 0 && ch < 0x88)
			|| (term->ucs.following_byte == 5 && term->ucs.count == 0 && term->ucs.code == 0 && ch < 0x84))
			term->ucs.is_valid = false;
		term->ucs.code <<= 6;
		term->ucs.code += ch & 0x3F;
		term->ucs.count++;
	}
	else if (0xC0 <= ch && ch <= 0xDF) {
		term->ucs.following_byte = 1;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x1F;
		return;
	}
	else if (0xE0 <= ch && ch <= 0xEF) {
		term->ucs.following_byte = 2;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x0F;
		return;
	}
	else if (0xF0 <= ch && ch <= 0xF7) {
		term->ucs.following_byte = 3;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x07;
		return;
	}
	else if (0xF8 <= ch && ch <= 0xFB) {
		term->ucs.following_byte = 4;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x03;
		return;
	}
	else if (0xFC <= ch && ch <= 0xFD) {
		term->ucs.following_byte = 5;
		term->ucs.count = 0;
		term->ucs.code = ch & 0x01;
		return;
	}
	else { /* 0xFE - 0xFF: not used in UTF-8 */
		addch(term, REPLACEMENT_CHAR);
		reset_ucs(term);
		return;
	}

	if (term->ucs.count >= term->ucs.following_byte) {
		/*	illegal code point (ref: http://www.unicode.org/reports/tr27/tr27-4.html)
			0xD800   ~ 0xDFFF : surrogate pair
			0xFDD0   ~ 0xFDEF : noncharacter
			0xnFFFE  ~ 0xnFFFF: noncharacter (n: 0x00 ~ 0x10)
			0x110000 ~        : invalid (unicode U+0000 ~ U+10FFFF)
		*/
		if (!term->ucs.is_valid
			|| (0xD800 <= term->ucs.code && term->ucs.code <= 0xDFFF)
			|| (0xFDD0 <= term->ucs.code && term->ucs.code <= 0xFDEF)
			|| ((term->ucs.code & 0xFFFF) == 0xFFFE || (term->ucs.code & 0xFFFF) == 0xFFFF)
			|| (term->ucs.code > 0x10FFFF))
			addch(term, REPLACEMENT_CHAR);
		else {
			if (DEBUG)
				fprintf(stderr, "unicode: U+%4.4X\n", term->ucs.code);
			addch(term, term->ucs.code);
		}
		reset_ucs(term);
	}
}

void parse(struct terminal *term, uint8_t *buf, int size)
{
	uint8_t ch;
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
			if (term->ucs.following_byte > 0 && (ch < 0x80 || ch > 0xBF)) {
				addch(term, REPLACEMENT_CHAR);
				reset_ucs(term);
			}

			if (ch <= 0x1F)
				control_character(term, ch);
			else if (ch <= 0x7F) {
				if (DEBUG)
					fprintf(stderr, "ascii: %c\n", ch);
				addch(term, ch);
			}
			else /* ch >= 0x80 */
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
