/* functions for wide char (UCS2), multibyte char (UTF-8) conversion */
/* ref:
 *	-	http://ja.m.wikipedia.org/wiki/UTF-8
 *	-	http://en.m.wikipedia.org/wiki/UTF-8
 *	-	http://www.azillionmonkeys.com/qed/unicode.html
 */

enum {
	UTF8_LEN_MAX          = 3,
	MALFORMED_CHARACTER   = 0xFFFD,
	CURSIVE_SQUARE_OFFSET = 0x60,
};

/*
 * from UCS2 to UTF-8
 * return length of UTF-8 sequence
 */
int utf8_encode(uint32_t ucs, char utf8_buf[UTF8_LEN_MAX + 1])
{
	if ((0xD800 <= ucs && ucs <= 0xDFFF)    /* used as surrogate pair in UTF-16 */
		|| (0xFDD0 <= ucs && ucs <= 0xFDEF) /* Noncharacter */
		|| ucs == 0xFFFE                    /* conflict byte order mark (U+FEFF) */
		|| ucs == 0xFFFF                    /* Noncharacter U+FFFF ("useful for internal purposes as sentinels") */
		|| ucs > 0xFFFF) {                  /* UCS2 (Unicode BMP): 0x0000 - 0xFFFF */
		/* invalid codepoint */
		return -1;
	}

	if (ucs <= 0x7F) {
		/* ASCII Character */
		utf8_buf[0] = (ucs & 0x7F);
		utf8_buf[1] = '\0';
		return 1;
	} else if (0x80 <= ucs && ucs <= 0x7FF) {
		/* 2 byte sequence */
		utf8_buf[0] = 0xC0 | ((ucs >> 6) & 0x1F);
		utf8_buf[1] = 0x80 | (ucs & 0x3F);
		utf8_buf[2] = '\0';
		return 2;
	} else if (0x800 <= ucs && ucs <= 0xFFFF) {
		/* 3 byte sequence */
		utf8_buf[0] = 0xE0 | ((ucs >> 12) & 0x0F);
		utf8_buf[1] = 0x80 | ((ucs >>  6) & 0x3F);
		utf8_buf[2] = 0x80 | (ucs & 0x3F);
		utf8_buf[3] = '\0';
		return 3;
	}
	/* illegal codepoint */
	return -1;
}

/* 
 * from UTF-8 to UCS2
 * return length of read sequence
 */
int utf8_decode(const char *utf8_str, uint32_t *ucs)
{
	int following_byte = 0, count = 0;
	uint32_t code = 0;
	bool is_valid = true;
	uint8_t ch;

	/* utf8 string must end by NUL (0x00) */
	for (int i = 0; utf8_str[i] != '\0'; i++) {
		ch = (uint8_t) utf8_str[i];

		if (ch <= 0x7F) {
			/* ASCII Character */
			*ucs = (uint32_t) ch;
			return 1;
		} else if (0x80 <= ch && ch <= 0xBF) {
			/* check illegal UTF-8 sequence
				* ? byte sequence: first byte must be between 0xC2 ~ 0xFD
				* 2 byte sequence: first byte must be between 0xC2 ~ 0xDF
				* 3 byte sequence: second byte following 0xE0 must be between 0xA0 ~ 0xBF
				* 4 byte sequence: second byte following 0xF0 must be between 0x90 ~ 0xBF
				* 5 byte sequence: second byte following 0xF8 must be between 0x88 ~ 0xBF
				* 6 byte sequence: second byte following 0xFC must be between 0x84 ~ 0xBF
			*/
			if ((following_byte == 0)
				|| (following_byte == 1 && count == 0 && code <= 1)
				|| (following_byte == 2 && count == 0 && code == 0 && ch < 0xA0)
				|| (following_byte == 3 && count == 0 && code == 0 && ch < 0x90)
				|| (following_byte == 4 && count == 0 && code == 0 && ch < 0x88)
				|| (following_byte == 5 && count == 0 && code == 0 && ch < 0x84))
				is_valid = false;
			code <<= 6;
			code += ch & 0x3F;
			count++;
		} else if (0xC0 <= ch && ch <= 0xDF) {
			code = ch & 0x1F;
			following_byte = 1;
			count = 0;
		} else if (0xE0 <= ch && ch <= 0xEF) {
			code = ch & 0x0F;
			following_byte = 2;
			count = 0;
		} else if (0xF0 <= ch && ch <= 0xF7) {
			code = ch & 0x07;
			following_byte = 3;
			count = 0;
		} else if (0xF8 <= ch && ch <= 0xFB) {
			code = ch & 0x03;
			following_byte = 4;
			count = 0;
		} else if (0xFC <= ch && ch <= 0xFD) {
			code = ch & 0x01;
			following_byte = 5;
			count = 0;
		} else { /* 0xFE - 0xFF: not used in UTF-8 */
			*ucs = MALFORMED_CHARACTER;
			return count + 1;
		}

		if (count >= following_byte) {
			/*  illegal code point (ref: http://www.unicode.org/reports/tr27/tr27-4.html)
				0xD800   ~ 0xDFFF : surrogate pair
				0xFDD0   ~ 0xFDEF : noncharacter
				0xnFFFE  ~ 0xnFFFF: noncharacter (n: 0x00 ~ 0x10)
				0x110000 ~        : invalid (unicode U+0000 ~ U+10FFFF)
			*/
			if (!is_valid
				|| (0xD800 <= code && code <= 0xDFFF)
				|| (0xFDD0 <= code && code <= 0xFDEF)
				|| ((code & 0xFFFF) == 0xFFFE || (code & 0xFFFF) == 0xFFFF)
				|| (code > 0x10FFFF)) {
				*ucs = MALFORMED_CHARACTER;
				return count + 1;
			} else {
				*ucs = code;
				return count + 1;
			}
		}
	}
	return MALFORMED_CHARACTER;
}
