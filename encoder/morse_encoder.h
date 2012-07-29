#ifndef MORSE_ENCODER_H_
#define MORSE_ENCODER_H_

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

/* Get the size (in number of marks) of a morse symbol */
#define MORSE_SYM_SIZE(symbol)		(((uint16_t)(symbol) & 0xF000) >> 12)

/* Returns the marks */
#define MORSE_SYM_MARKS(symbol)		((symbol) & 0x1FF)

/* Returns true, if the symbol is a space between words */
#define MORSE_SYM_IS_SPACE(symbol)	((symbol) == 0)

#endif /* MORSE_ENCODER_H_ */
