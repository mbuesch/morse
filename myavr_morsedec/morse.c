/*
 * Morse decoder
 *
 * Copyright (c) 2012 Michael Buesch <m@bues.ch>
 * Licensed under the terms of the GNU General Public License version 2.
 */

#include "morse.h"
#include "util.h"


#define dit			MORSE_DIT
#define dah			MORSE_DAH

#define CHAR_SYM(charac, sym)	[(charac) - MORSE_CHARS_START] = sym
#define NUM_SYM(num, sym)	[(num) - MORSE_NUMS_START] = sym
#define SPEC_SYM(spec, sym)	[(spec) - MORSE_SPEC_START] = sym
#define SIG_SYM(sig, sym)	[(sig) - MORSE_SIG_START] = sym

static const morse_sym_t PROGMEM character_symbols[] = {
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

static const morse_sym_t PROGMEM number_symbols[] = {
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

static const morse_sym_t PROGMEM special_symbols[] = {
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

static const morse_sym_t PROGMEM signal_symbols[] = {
	SIG_SYM(MORSE_SIG_KA, MORSE_SYM_5(dah, dit, dah, dit, dah)),
	SIG_SYM(MORSE_SIG_BT, MORSE_SYM_5(dah, dit, dit, dit, dah)),
	SIG_SYM(MORSE_SIG_AR, MORSE_SYM_5(dit, dah, dit, dah, dit)),
	SIG_SYM(MORSE_SIG_VE, MORSE_SYM_5(dit, dit, dit, dah, dit)),
	SIG_SYM(MORSE_SIG_SK, MORSE_SYM_6(dit, dit, dit, dah, dit, dah)),
	SIG_SYM(MORSE_SIG_SOS, MORSE_SYM_9(dit, dit, dit, dah, dah, dah, dit, dit, dit)),
	SIG_SYM(MORSE_SIG_ERROR, MORSE_SYM_8(dit, dit, dit, dit, dit, dit, dit, dit)),
};


static morse_sym_t fetch_sym(const morse_sym_t * PROGPTR table,
			     uint8_t offset)
{
	return pgm_read_word(&table[offset]);
}

morse_sym_t morse_encode_character(enum morse_character c)
{
	if (c >= MORSE_CHARS_START && c <= MORSE_CHARS_END)
		return fetch_sym(character_symbols, c - MORSE_CHARS_START);
	if (c >= MORSE_NUMS_START && c <= MORSE_NUMS_END)
		return fetch_sym(number_symbols, c - MORSE_NUMS_START);
	if (c >= MORSE_SPEC_START && c <= MORSE_SPEC_END)
		return fetch_sym(special_symbols, c - MORSE_SPEC_START);
	if (c >= MORSE_SIG_START && c <= MORSE_SIG_END)
		return fetch_sym(signal_symbols, c - MORSE_SIG_START);

	return fetch_sym(signal_symbols, MORSE_SIG_ERROR - MORSE_SIG_START);
}

enum morse_character morse_decode_symbol(morse_sym_t sym)
{
	unsigned int i;

#define check_decode_tab(table, startchar)				\
	for (i = 0; i < ARRAY_SIZE(table); i++) {			\
		if (fetch_sym(table, i) == sym)				\
			return (enum morse_character)(i + startchar);	\
	}

	check_decode_tab(character_symbols, MORSE_CHARS_START)
	check_decode_tab(number_symbols, MORSE_NUMS_START)
	check_decode_tab(special_symbols, MORSE_SPEC_START)
	check_decode_tab(signal_symbols, MORSE_SIG_START)

#undef check_decode_tab

	return MORSE_INVALID;
}

int8_t morse_to_ascii(char *buf, uint8_t buf_size,
		      enum morse_character mchar)
{
	int8_t ret = 0;

#define put_char(c) {			\
		if (!buf_size--)	\
			return -1;	\
		*buf++ = (c);		\
		ret++;			\
	}

	if ((mchar >= MORSE_CHARS_START && mchar <= MORSE_CHARS_END) ||
	    (mchar >= MORSE_NUMS_START && mchar <= MORSE_NUMS_END)) {
		put_char((char)mchar);
	} else {
		switch (mchar) {
		case MORSE_GACC_A:
			put_char('A');
			break;
		case MORSE_GACC_E:
		case MORSE_AACC_E:
			put_char('E');
			break;
		case MORSE_AE:
			put_char('A');
			put_char('E');
			break;
		case MORSE_OE:
			put_char('O');
			put_char('E');
			break;
		case MORSE_UE:
			put_char('U');
			put_char('E');
			break;
		case MORSE_SZ:
			put_char('S');
			put_char('S');
			break;
		case MORSE_CH:
			put_char('C');
			put_char('H');
			break;
		case MORSE_TILDE_N:
			put_char('N');
			break;
		case MORSE_PERIOD:
			put_char('.');
			break;
		case MORSE_COMMA:
			put_char(',');
			break;
		case MORSE_COLON:
			put_char(':');
			break;
		case MORSE_SEMICOLON:
			put_char(';');
			break;
		case MORSE_QUESTION:
			put_char('?');
			break;
		case MORSE_DASH:
			put_char('-');
			break;
		case MORSE_UNDERSCORE:
			put_char('_');
			break;
		case MORSE_PAREN_OPEN:
			put_char('(');
			break;
		case MORSE_PAREN_CLOSE:
			put_char(')');
			break;
		case MORSE_TICK:
			put_char('\'');
			break;
		case MORSE_EQUAL:
			put_char('=');
			break;
		case MORSE_PLUS:
			put_char('+');
			break;
		case MORSE_SLASH:
			put_char('/');
			break;
		case MORSE_AT:
			put_char('@');
			break;
		case MORSE_SPACE:
			put_char(' ');
			break;
		case MORSE_SIG_KA:
		case MORSE_SIG_BT:
		case MORSE_SIG_AR:
		case MORSE_SIG_VE:
		case MORSE_SIG_SK:
			put_char(' ');
			break;
		case MORSE_SIG_SOS:
			put_char('S');
			put_char('0');
			put_char('S');
			break;
		case MORSE_SIG_ERROR:
			put_char('-');
			put_char('E');
			put_char('R');
			put_char('R');
			put_char('O');
			put_char('R');
			put_char('-');
			break;
		default:
			return -1;
		}
	}
#undef put_char

	return ret;
}
