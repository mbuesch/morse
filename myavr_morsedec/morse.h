#ifndef MORSE_DECODER_H_
#define MORSE_DECODER_H_

#include <stdint.h>


/* Low level morse symbols */
enum morse_marks {
	MORSE_DIT = 0,		/* . */
	MORSE_DAH = 1,		/* - */
};

/* A morse symbol.
 * bit 0-8: The transmitted marks (dit, dah)
 * bit 12-15: The symbol length. In number of marks.
 */
typedef uint16_t morse_sym_t;

/* Construct a morse symbol */
#define __MORSE_SYM(marks, size)	((morse_sym_t)((marks) | (size) << 12))
#define MORSE_MARK(type, index)		((morse_sym_t)((type) << (index)))

#define MORSE_MAX_NR_MARKS		9


#define MORSE_SYM_1(m0)			__MORSE_SYM(((m0) << 0), 1)
#define MORSE_SYM_2(m0, m1)		__MORSE_SYM(((m0) << 0) | ((m1) << 1), 2)
#define MORSE_SYM_3(m0, m1, m2)		__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2), 3)
#define MORSE_SYM_4(m0, m1, m2, m3)	__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3), 4)
#define MORSE_SYM_5(m0, m1, m2, m3, m4)	__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3) | \
						    ((m4) << 4), 5)
#define MORSE_SYM_6(m0, m1, m2, m3, m4, m5) \
					__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3) | \
						    ((m4) << 4) | ((m5) << 5), 6)
#define MORSE_SYM_7(m0, m1, m2, m3, m4, m5, m6) \
					__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3) | \
						    ((m4) << 4) | ((m5) << 5) | ((m6) << 6), 7)
#define MORSE_SYM_8(m0, m1, m2, m3, m4, m5, m6, m7) \
					__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3) | \
						    ((m4) << 4) | ((m5) << 5) | ((m6) << 6) | ((m7) << 7), 8)
#define MORSE_SYM_9(m0, m1, m2, m3, m4, m5, m6, m7, m8) \
					__MORSE_SYM(((m0) << 0) | ((m1) << 1) | ((m2) << 2) | ((m3) << 3) | \
						    ((m4) << 4) | ((m5) << 5) | ((m6) << 6) | ((m7) << 7) | \
						    ((m8) << 8), 9)

static inline uint8_t morse_sym_size(morse_sym_t sym)
{
	return (sym & 0xF000) >> 12;
}

static inline void morse_sym_set_size(morse_sym_t *sym, uint8_t size)
{
	*sym |= size << 12;
}

/* Returns the marks */
#define MORSE_SYM_MARKS(symbol)		((symbol) & 0x1FF)

/* Returns true, if the symbol is a space between words */
#define MORSE_SYM_IS_SPACE(symbol)	((symbol) == 0)


/* The morse alphabet */
enum morse_character {
	MORSE_INVALID	= 0,

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

/* Morse factors - In multiples of "dit" */
enum morse_factors {
	FACTOR_DIT		= 1,
	FACTOR_DAH		= 3,
	FACTOR_INTER_MARK	= 1,
	FACTOR_INTER_CHAR	= 3,
	FACTOR_INTER_WORD	= 7,
};

/* Length of one "dit" in milliseconds at a rate
 * of one word per minute. */
#define DIT_LENGTH_1WPM_MS	1200


morse_sym_t morse_encode_character(enum morse_character mchar);
enum morse_character morse_decode_symbol(morse_sym_t sym);
int8_t morse_to_ascii(char *buf, uint8_t buf_size,
		      enum morse_character mchar);

#endif /* MORSE_DECODER_H_ */
