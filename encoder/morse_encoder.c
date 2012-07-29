/*
 *  Morse encoder
 *
 *  Copyright (C) 2011 Michael Buesch <m@bues.ch>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "util.h"
#include "morse_encoder.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>


enum morse_encoding {
	ENC_BINARY,
	ENC_DASHDOT,
	ENC_DITDAH,
};

static int syms_bigendian;
static int decode;
static enum morse_encoding morse_encoding = ENC_DASHDOT;
static const char *input_text;
static size_t input_text_len;


/* The morse alphabet */
enum morse_character {
	/* Characters */
	MORSE_A		= 'A',
	MORSE_B		= 'B',
	MORSE_C		= 'C',
	MORSE_D		= 'D',
	MORSE_E		= 'E',
	MORSE_F		= 'F',
	MORSE_G		= 'G',
	MORSE_H		= 'H',
	MORSE_I		= 'I',
	MORSE_J		= 'J',
	MORSE_K		= 'K',
	MORSE_L		= 'L',
	MORSE_M		= 'M',
	MORSE_N		= 'N',
	MORSE_O		= 'O',
	MORSE_P		= 'P',
	MORSE_Q		= 'Q',
	MORSE_R		= 'R',
	MORSE_S		= 'S',
	MORSE_T		= 'T',
	MORSE_U		= 'U',
	MORSE_V		= 'V',
	MORSE_W		= 'W',
	MORSE_X		= 'X',
	MORSE_Y		= 'Y',
	MORSE_Z		= 'Z',

	MORSE_CHARS_START	= MORSE_A,
	MORSE_CHARS_END		= MORSE_Z,

	/* Numbers */
	MORSE_0		= '0',
	MORSE_1		= '1',
	MORSE_2		= '2',
	MORSE_3		= '3',
	MORSE_4		= '4',
	MORSE_5		= '5',
	MORSE_6		= '6',
	MORSE_7		= '7',
	MORSE_8		= '8',
	MORSE_9		= '9',

	MORSE_NUMS_START	= MORSE_0,
	MORSE_NUMS_END		= MORSE_9,

	/* Special characters */
	MORSE_GACC_A	= 128,		/* grave accent A */
	MORSE_AE,			/* Ä */
	MORSE_GACC_E,			/* grave accent E */
	MORSE_AACC_E,			/* acute accent E */
	MORSE_OE,			/* Ö */
	MORSE_UE,			/* Ü */
	MORSE_SZ,			/* ß */
	MORSE_CH,			/* CH */
	MORSE_TILDE_N,			/* tilde N */
	MORSE_PERIOD,			/* . */
	MORSE_COMMA,			/* , */
	MORSE_COLON,			/* : */
	MORSE_SEMICOLON,		/* ; */
	MORSE_QUESTION,			/* ? */
	MORSE_DASH,			/* - */
	MORSE_UNDERSCORE,		/* _ */
	MORSE_PAREN_OPEN,		/* ( */
	MORSE_PAREN_CLOSE,		/* ) */
	MORSE_TICK,			/* ' */
	MORSE_EQUAL,			/* = */
	MORSE_PLUS,			/* + */
	MORSE_SLASH,			/* / */
	MORSE_AT,			/* @ */
	MORSE_SPACE,			/*   */

	MORSE_SPEC_START	= MORSE_GACC_A,
	MORSE_SPEC_END		= MORSE_SPACE,

	/* Signals */
	MORSE_SIG_KA	= 192,		/* Start */
	MORSE_SIG_BT,			/* Pause */
	MORSE_SIG_AR,			/* End */
	MORSE_SIG_VE,			/* Understood */
	MORSE_SIG_SK,			/* End of work */
	MORSE_SIG_SOS,			/* SOS */
	MORSE_SIG_ERROR,		/* Error */

	MORSE_SIG_START		= MORSE_SIG_KA,
	MORSE_SIG_END		= MORSE_SIG_ERROR,
};


#define dit	MORSE_DIT
#define dah	MORSE_DAH

#define CHAR_SYM(character, symbol)	[(character) - MORSE_CHARS_START] = symbol
#define NUM_SYM(number, symbol)		[(number) - MORSE_NUMS_START] = symbol
#define SPEC_SYM(spec, symbol)		[(spec) - MORSE_SPEC_START] = symbol
#define SIG_SYM(signal, symbol)		[(signal) - MORSE_SIG_START] = symbol

static const morse_sym_t character_symbols[] = {
	CHAR_SYM(MORSE_A, MORSE_SYM_2(dit, dah)),
	CHAR_SYM(MORSE_B, MORSE_SYM_4(dah, dit, dit, dit)),
	CHAR_SYM(MORSE_C, MORSE_SYM_4(dah, dit, dah, dit)),
	CHAR_SYM(MORSE_D, MORSE_SYM_3(dah, dit, dit)),
	CHAR_SYM(MORSE_E, MORSE_SYM_1(dit)),
	CHAR_SYM(MORSE_F, MORSE_SYM_4(dit, dit, dah, dit)),
	CHAR_SYM(MORSE_G, MORSE_SYM_3(dah, dah, dit)),
	CHAR_SYM(MORSE_H, MORSE_SYM_4(dit, dit, dit, dit)),
	CHAR_SYM(MORSE_I, MORSE_SYM_2(dit, dit)),
	CHAR_SYM(MORSE_J, MORSE_SYM_4(dit, dah, dah, dah)),
	CHAR_SYM(MORSE_K, MORSE_SYM_3(dah, dit, dah)),
	CHAR_SYM(MORSE_L, MORSE_SYM_4(dit, dah, dit, dit)),
	CHAR_SYM(MORSE_M, MORSE_SYM_2(dah, dah)),
	CHAR_SYM(MORSE_N, MORSE_SYM_2(dah, dit)),
	CHAR_SYM(MORSE_O, MORSE_SYM_3(dah, dah, dah)),
	CHAR_SYM(MORSE_P, MORSE_SYM_4(dit, dah, dah, dit)),
	CHAR_SYM(MORSE_Q, MORSE_SYM_4(dah, dah, dit, dah)),
	CHAR_SYM(MORSE_R, MORSE_SYM_3(dit, dah, dit)),
	CHAR_SYM(MORSE_S, MORSE_SYM_3(dit, dit, dit)),
	CHAR_SYM(MORSE_T, MORSE_SYM_1(dah)),
	CHAR_SYM(MORSE_U, MORSE_SYM_3(dit, dit, dah)),
	CHAR_SYM(MORSE_V, MORSE_SYM_4(dit, dit, dit, dah)),
	CHAR_SYM(MORSE_W, MORSE_SYM_3(dit, dah, dah)),
	CHAR_SYM(MORSE_X, MORSE_SYM_4(dah, dit, dit, dah)),
	CHAR_SYM(MORSE_Y, MORSE_SYM_4(dah, dit, dah, dah)),
	CHAR_SYM(MORSE_Z, MORSE_SYM_4(dah, dah, dit, dit)),
};

static const morse_sym_t number_symbols[] = {
	NUM_SYM(MORSE_0, MORSE_SYM_5(dah, dah, dah, dah, dah)),
	NUM_SYM(MORSE_1, MORSE_SYM_5(dit, dah, dah, dah, dah)),
	NUM_SYM(MORSE_2, MORSE_SYM_5(dit, dit, dah, dah, dah)),
	NUM_SYM(MORSE_3, MORSE_SYM_5(dit, dit, dit, dah, dah)),
	NUM_SYM(MORSE_4, MORSE_SYM_5(dit, dit, dit, dit, dah)),
	NUM_SYM(MORSE_5, MORSE_SYM_5(dit, dit, dit, dit, dit)),
	NUM_SYM(MORSE_6, MORSE_SYM_5(dah, dit, dit, dit, dit)),
	NUM_SYM(MORSE_7, MORSE_SYM_5(dah, dah, dit, dit, dit)),
	NUM_SYM(MORSE_8, MORSE_SYM_5(dah, dah, dah, dit, dit)),
	NUM_SYM(MORSE_9, MORSE_SYM_5(dah, dah, dah, dah, dit)),
};

static const morse_sym_t special_symbols[] = {
	SPEC_SYM(MORSE_GACC_A, MORSE_SYM_5(dit, dah, dah, dit, dah)),
	SPEC_SYM(MORSE_AE, MORSE_SYM_4(dit, dah, dit, dah)),
	SPEC_SYM(MORSE_GACC_E, MORSE_SYM_5(dit, dah, dit, dit, dah)),
	SPEC_SYM(MORSE_AACC_E, MORSE_SYM_5(dit, dit, dah, dit, dit)),
	SPEC_SYM(MORSE_OE, MORSE_SYM_4(dah, dah, dah, dit)),
	SPEC_SYM(MORSE_UE, MORSE_SYM_4(dit, dit, dah, dah)),
	SPEC_SYM(MORSE_SZ, MORSE_SYM_6(dit, dit, dit, dah, dah, dit)),
	SPEC_SYM(MORSE_CH, MORSE_SYM_4(dah, dah, dah, dah)),
	SPEC_SYM(MORSE_TILDE_N, MORSE_SYM_5(dah, dah, dit, dah, dah)),
	SPEC_SYM(MORSE_PERIOD, MORSE_SYM_6(dit, dah, dit, dah, dit, dah)),
	SPEC_SYM(MORSE_COMMA, MORSE_SYM_6(dah, dah, dit, dit, dah, dah)),
	SPEC_SYM(MORSE_COLON, MORSE_SYM_6(dah, dah, dah, dit, dit, dit)),
	SPEC_SYM(MORSE_SEMICOLON, MORSE_SYM_6(dah, dit, dah, dit, dah, dit)),
	SPEC_SYM(MORSE_QUESTION, MORSE_SYM_6(dit, dit, dah, dah, dit, dit)),
	SPEC_SYM(MORSE_DASH, MORSE_SYM_6(dah, dit, dit, dit, dit, dah)),
	SPEC_SYM(MORSE_UNDERSCORE, MORSE_SYM_6(dit, dit, dah, dah, dit, dah)),
	SPEC_SYM(MORSE_PAREN_OPEN, MORSE_SYM_5(dah, dit, dah, dah, dit)),
	SPEC_SYM(MORSE_PAREN_CLOSE, MORSE_SYM_6(dah, dit, dah, dah, dit, dah)),
	SPEC_SYM(MORSE_TICK, MORSE_SYM_6(dit, dah, dah, dah, dah, dit)),
	SPEC_SYM(MORSE_EQUAL, MORSE_SYM_5(dah, dit, dit, dit, dah)),
	SPEC_SYM(MORSE_PLUS, MORSE_SYM_5(dit, dah, dit, dah, dit)),
	SPEC_SYM(MORSE_SLASH, MORSE_SYM_5(dah, dit, dit, dah, dit)),
	SPEC_SYM(MORSE_AT, MORSE_SYM_6(dit, dah, dah, dit, dah, dit)),
	SPEC_SYM(MORSE_SPACE, 0),
};

static const morse_sym_t signal_symbols[] = {
	SIG_SYM(MORSE_SIG_KA, MORSE_SYM_5(dah, dit, dah, dit, dah)),
	SIG_SYM(MORSE_SIG_BT, MORSE_SYM_5(dah, dit, dit, dit, dah)),
	SIG_SYM(MORSE_SIG_AR, MORSE_SYM_5(dit, dah, dit, dah, dit)),
	SIG_SYM(MORSE_SIG_VE, MORSE_SYM_5(dit, dit, dit, dah, dit)),
	SIG_SYM(MORSE_SIG_SK, MORSE_SYM_6(dit, dit, dit, dah, dit, dah)),
	SIG_SYM(MORSE_SIG_SOS, MORSE_SYM_9(dit, dit, dit, dah, dah, dah, dit, dit, dit)),
	SIG_SYM(MORSE_SIG_ERROR, MORSE_SYM_8(dit, dit, dit, dit, dit, dit, dit, dit)),
};


static morse_sym_t morse_encode_character(enum morse_character c)
{
	BUILD_BUG_ON(ARRAY_SIZE(character_symbols) != MORSE_CHARS_END - MORSE_CHARS_START + 1);
	BUILD_BUG_ON(ARRAY_SIZE(number_symbols) != MORSE_NUMS_END - MORSE_NUMS_START + 1);
	BUILD_BUG_ON(ARRAY_SIZE(special_symbols) != MORSE_SPEC_END - MORSE_SPEC_START + 1);
	BUILD_BUG_ON(ARRAY_SIZE(signal_symbols) != MORSE_SIG_END - MORSE_SIG_START + 1);

	if (c >= MORSE_CHARS_START && c <= MORSE_CHARS_END)
		return character_symbols[c - MORSE_CHARS_START];
	if (c >= MORSE_NUMS_START && c <= MORSE_NUMS_END)
		return number_symbols[c - MORSE_NUMS_START];
	if (c >= MORSE_SPEC_START && c <= MORSE_SPEC_END)
		return special_symbols[c - MORSE_SPEC_START];
	if (c >= MORSE_SIG_START && c <= MORSE_SIG_END)
		return signal_symbols[c - MORSE_SIG_START];

	return signal_symbols[MORSE_SIG_ERROR - MORSE_SIG_START];
}

static enum morse_character morse_decode_symbol(morse_sym_t sym)
{
	unsigned int i;

#define check_decode_tab(table, startchar) \
		for (i = 0; i < ARRAY_SIZE(table); i++) {			\
			if (table[i] == sym)					\
				return (enum morse_character)(i + startchar);	\
		}

	check_decode_tab(character_symbols, MORSE_CHARS_START)
	check_decode_tab(number_symbols, MORSE_NUMS_START)
	check_decode_tab(special_symbols, MORSE_SPEC_START)
	check_decode_tab(signal_symbols, MORSE_SIG_START)

#undef check_decode_tab

	return (enum morse_character)-1;
}

static enum morse_character ascii_to_morse(char ascii_char)
{
	if (ascii_char >= 'a' && ascii_char <= 'z')
		return MORSE_A + (ascii_char - 'a');
	if (ascii_char >= 'A' && ascii_char <= 'Z')
		return MORSE_A + (ascii_char - 'A');
	if (ascii_char >= '0' && ascii_char <= '9')
		return MORSE_0 + (ascii_char - '0');

	switch (ascii_char) {
	case ' ':
	case '\t':
	case '\n':
		return MORSE_SPACE;
	case '.':
		return MORSE_PERIOD;
	case ',':
		return MORSE_COMMA;
	case ':':
		return MORSE_COLON;
	case ';':
		return MORSE_SEMICOLON;
	case '?':
		return MORSE_QUESTION;
	case '-':
		return MORSE_DASH;
	case '_':
		return MORSE_UNDERSCORE;
	case '(':
		return MORSE_PAREN_OPEN;
	case ')':
		return MORSE_PAREN_CLOSE;
	case '\'':
		return MORSE_TICK;
	case '=':
		return MORSE_EQUAL;
	case '+':
		return MORSE_PLUS;
	case '/':
		return MORSE_SLASH;
	case '@':
		return MORSE_AT;
	}

	return MORSE_SIG_ERROR;
}

static char morse_to_ascii(enum morse_character mchar)
{
	if ((mchar >= MORSE_CHARS_START && mchar <= MORSE_CHARS_END) ||
	    (mchar >= MORSE_NUMS_START && mchar <= MORSE_NUMS_END))
		return (char)mchar;

	switch (mchar) {
	case MORSE_SPACE:
		return ' ';
	case MORSE_PERIOD:
		return '.';
	case MORSE_COMMA:
		return ',';
	case MORSE_COLON:
		return ':';
	case MORSE_SEMICOLON:
		return ';';
	case MORSE_QUESTION:
		return '?';
	case MORSE_DASH:
		return '-';
	case MORSE_UNDERSCORE:
		return '_';
	case MORSE_PAREN_OPEN:
		return '(';
	case MORSE_PAREN_CLOSE:
		return ')';
	case MORSE_TICK:
		return '\'';
	case MORSE_EQUAL:
		return '=';
	case MORSE_PLUS:
		return '+';
	case MORSE_SLASH:
		return '/';
	case MORSE_AT:
		return '@';
	default:
		break;
	}

	return '\0';
}

static int morse_encode(void)
{
	const char *ascii;
	enum morse_character morse;
	morse_sym_t sym;
	unsigned int i;

	for (ascii = input_text; *ascii; ascii++) {
		morse = ascii_to_morse(*ascii);
		if (morse == MORSE_SIG_ERROR) {
			fprintf(stderr, "Could not translate character: %c\n", *ascii);
			return -1;
		}
		sym = morse_encode_character(morse);

		switch (morse_encoding) {
		case ENC_DASHDOT:
			if (MORSE_SYM_IS_SPACE(sym)) {
				printf("/  ");
			} else {
				for (i = 0; i < MORSE_SYM_SIZE(sym); i++) {
					if (((MORSE_SYM_MARKS(sym) >> i) & 1) == MORSE_DIT)
						putchar('.');
					else
						putchar('-');
				}
				putchar(' ');
				putchar(' ');
			}
			break;
		case ENC_DITDAH:
			if (MORSE_SYM_IS_SPACE(sym)) {
				printf(", ");
			} else {
				for (i = 0; i < MORSE_SYM_SIZE(sym); i++) {
					if (((MORSE_SYM_MARKS(sym) >> i) & 1) == MORSE_DIT) {
						if (i == 0)
							printf("Di");
						else if (i == MORSE_SYM_SIZE(sym) - 1)
							printf("-dit");
						else
							printf("-di");
					} else {
						if (i == 0)
							printf("Dah");
						else
							printf("-dah");
					}
				}
				putchar(' ');
			}
			break;
		case ENC_BINARY:
			if (syms_bigendian) {
				putchar((sym >> 8) & 0xFF);
				putchar(sym & 0xFF);
			} else {
				putchar(sym & 0xFF);
				putchar((sym >> 8) & 0xFF);
			}
			break;
		}
	}
	if (morse_encoding != ENC_BINARY)
		putchar('\n');

	return 0;
}

static int decode_marks(unsigned int marks, unsigned int size)
{
	morse_sym_t sym;
	enum morse_character mchar;
	char achar;

	sym = __MORSE_SYM(marks, size);
	mchar = morse_decode_symbol(sym);
	if (mchar == (enum morse_character)-1) {
		fprintf(stderr, "Could not decode symbol 0x%04X\n",
			(uint16_t)sym);
		return -1;
	}
	achar = morse_to_ascii(mchar);
	if (!achar) {
		fprintf(stderr, "Could not decode morse char 0x%02X\n",
			(uint8_t)mchar);
		return -1;
	}
	putchar(achar);

	return 0;
}

static int morse_decode_dashdot(void)
{
	const char *input = input_text;
	char c;
	unsigned int i = 0;
	unsigned int marks = 0;
	int err;

	while (1) {
		c = *input;
		if (!c) {
			if (i) {
				err = decode_marks(marks, i);
				if (err)
					return err;
			}
			break;
		}
		input++;
		if (c == '.') {
			marks &= ~(1 << i);
			marks |= (MORSE_DIT << i);
			i++;
		} else if (c == '-' || c == '_') {
			marks &= ~(1 << i);
			marks |= (MORSE_DAH << i);
			i++;
		} else if (isspace(c)) { /* end of char */
			if (i) {
				err = decode_marks(marks, i);
				if (err)
					return err;
				i = 0;
				marks = 0;
			}
		} else /* end of word */
			putchar(' ');
		if (i > 9) {
			fprintf(stderr, "Too many marks\n");
			return -1;
		}
	}

	return 0;
}

static const char * eat(const char *str, char c)
{
	while (*str == c)
		str++;
	return str;
}

static const char * eat_alpha(const char *str)
{
	while (isalpha(*str))
		str++;
	return str;
}

static const char * eat_space(const char *str)
{
	while (isspace(*str))
		str++;
	return str;
}

static int morse_decode_ditdah(void)
{
	const char *input = input_text;
	unsigned int i = 0;
	unsigned int marks = 0;
	int err;

	while (1) {
		if (!(*input)) {
			if (i) {
				err = decode_marks(marks, i);
				if (err)
					return err;
			}
			break;
		}
		if (strncasecmp(input, "di", 2) == 0) {
			marks &= ~(1 << i);
			marks |= (MORSE_DIT << i);
			i++;
			input = eat_alpha(input);
			input = eat(input, '-');
		} else if (strncasecmp(input, "da", 2) == 0) {
			marks &= ~(1 << i);
			marks |= (MORSE_DAH << i);
			i++;
			input = eat_alpha(input);
			input = eat(input, '-');
		} else if (isspace(*input)) { /* end of char */
			if (i) {
				err = decode_marks(marks, i);
				if (err)
					return err;
				i = 0;
				marks = 0;
			}
			input = eat_space(input);
		} else { /* end of word */
			putchar(' ');
			input++;
			input = eat_space(input);
		}
		if (i > 9) {
			fprintf(stderr, "Too many marks\n");
			return -1;
		}
	}

	return 0;
}

static int morse_decode_binary(void)
{
	size_t i;
	morse_sym_t sym;
	enum morse_character mchar;
	char achar;

	if (input_text_len % 2) {
		fprintf(stderr, "Invalid input length (odd length)\n");
		return -1;
	}
	for (i = 0; i < input_text_len; i += 2) {
		if (syms_bigendian) {
			sym = (uint16_t)input_text[i] << 8;
			sym |= (uint16_t)input_text[i + 1];
		} else {
			sym = (uint16_t)input_text[i];
			sym |= (uint16_t)input_text[i + 1] << 8;
		}
		mchar = morse_decode_symbol(sym);
		if (mchar == (enum morse_character)-1) {
			fprintf(stderr, "Could not decode symbol 0x%04X\n",
				(uint16_t)sym);
			return -1;
		}
		achar = morse_to_ascii(mchar);
		if (!achar) {
			fprintf(stderr, "Could not decode morse char 0x%02X\n",
				(uint8_t)mchar);
			return -1;
		}
		putchar(achar);
	}

	return 0;
}

static int morse_decode(void)
{
	int err = -1;

	switch (morse_encoding) {
	case ENC_DASHDOT:
		err = morse_decode_dashdot();
		break;
	case ENC_DITDAH:
		err = morse_decode_ditdah();
		break;
	case ENC_BINARY:
		err = morse_decode_binary();
		break;
	}
	if (err)
		return err;
	putchar('\n');

	return 0;
}

static void usage(int argc, char **argv)
{
	printf("Usage: %s <options> [STRING]\n\n", argv[0]);
	printf("Options:\n");
	printf(" -b|--bigendian       Bigendian symbols (default little endian)\n");
	printf(" -B|--binary          Binary format\n");
	printf(" -d|--dashdot         Human readable dash/dot format (default)\n");
	printf(" -D|--ditdah          Human readable dit/dah format\n");
	printf(" -x|--decode          Switch to decode mode\n");
}

static int parse_args(int argc, char **argv)
{
	int i = 0, c;

	static const struct option long_opts[] = {
		{ .name = "help",	.has_arg = no_argument, .flag = NULL, .val = 'h' },
		{ .name = "bigendian",	.has_arg = no_argument, .flag = NULL, .val = 'b' },
		{ .name = "binary",	.has_arg = no_argument, .flag = NULL, .val = 'B' },
		{ .name = "ditdah",	.has_arg = no_argument, .flag = NULL, .val = 'D' },
		{ .name = "dashdot",	.has_arg = no_argument, .flag = NULL, .val = 'd' },
		{ .name = "decode",	.has_arg = no_argument, .flag = NULL, .val = 'x' },
	};

	while (1) {
		c = getopt_long(argc, argv, "hbBDdx", long_opts, &i);
		if (c == -1)
			break;
		switch (c) {
		case 'h':
			usage(argc, argv);
			return 1;
		case 'b':
			syms_bigendian = 1;
			break;
		case 'B':
			morse_encoding = ENC_BINARY;
			break;
		case 'd':
			morse_encoding = ENC_DASHDOT;
			break;
		case 'D':
			morse_encoding = ENC_DITDAH;
			break;
		case 'x':
			decode = 1;
			break;
		default:
			return -1;
		}
	}
	if (optind < argc)
		input_text = argv[optind];

	return 0;
}

size_t stdin_read(const char **buffer)
{
	char *buf = NULL;
	size_t bufsize = 0, i = 0;
	int c;

	while (1) {
		c = fgetc(stdin);
		if (c == EOF)
			break;
		if (i + 1 >= bufsize) {
			bufsize += 32;
			buf = realloc(buf, bufsize);
		}
		buf[i++] = c;
	}
	if (buf)
		buf[i] = '\0';
	*buffer = buf;

	return i;
}

int main(int argc, char **argv)
{
	int err;

	err = parse_args(argc, argv);
	if (err > 0)
		return 0;
	if (err < 0)
		return 1;

	if (input_text)
		input_text_len = strlen(input_text);
	else
		input_text_len = stdin_read(&input_text);
	if (!input_text || !input_text_len)
		return 1;

	if (decode)
		err = morse_decode();
	else
		err = morse_encode();

	return err ? 1 : 0;
}
