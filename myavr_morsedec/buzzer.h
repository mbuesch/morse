#ifndef BUZZER_H_
#define BUZZER_H_

#include "util.h"

#include <stdint.h>


typedef uint8_t note_t;

enum note_t_fields {
	NOTE_ID_MASK		= 0x0F,
	NOTE_ID_SHIFT		= 0,
	NOTE_VAL_MASK		= 0x70,
	NOTE_VAL_SHIFT		= 4,
};

enum note_ids {
	NOTEID_PAUSE	= 0,
	NOTEID_C4,
	NOTEID_D4,
	NOTEID_E4,
	NOTEID_F4,
	NOTEID_G4,
	NOTEID_A4,
	NOTEID_B4,
	NOTEID_C5,
	NOTEID_D5,
	NOTEID_E5,
	NOTEID_F5,
	NOTEID_G5,
	NOTEID_A5,
	NOTEID_B5,

	NOTEID_FLAGS,
};

enum note_values {
	NOTEVAL_1_1	= 0,	/* 1/1 */
	NOTEVAL_1_2,		/* 1/2 */
	NOTEVAL_1_4,		/* 1/4 */
	NOTEVAL_1_8,		/* 1/8 */
	NOTEVAL_1_16,		/* 1/16 */
	NOTEVAL_1_32,		/* 1/32 */
	NOTEVAL_1_64,		/* 1/64 */
	NOTEVAL_UNUSED0,
};

enum note_flags_values {
	NOTEVAL_SHARP		= 0,
	NOTEVAL_DOT,
	NOTEVAL_OCTAVE_SH_UP,
	NOTEVAL_OCTAVE_SH_DOWN,
	NOTEVAL_UNUSED1,
	NOTEVAL_UNUSED2,
	NOTEVAL_UNUSED3,
	NOTEVAL_UNUSED4,
};

#define note(id, val)	(					\
	((NOTEID_##id << NOTE_ID_SHIFT) & NOTE_ID_MASK) |	\
	((NOTEVAL_##val << NOTE_VAL_SHIFT) & NOTE_VAL_MASK)	)

#define n_sharp		note(FLAGS, SHARP)
#define n_dot		note(FLAGS, DOT)
#define n_octave_up	note(FLAGS, OCTAVE_SH_UP)
#define n_octave_down	note(FLAGS, OCTAVE_SH_DOWN)
#define n_pause(val)	note(PAUSE, val)

#define invalid_note	0xFF

#define note_array_end	invalid_note

extern const note_t PROGMEM buzzer_elise[];

void buzzer_init(uint16_t basespeed_note_1_1_ms);

void buzzer_play(const note_t PROGPTR *notes);

void buzzer_tune_note(note_t n);

#endif /* BUZZER_H_ */
