/*
 * Piezosummeransteuerung
 *
 * Copyright (c) 2012 Michael Buesch <m@bues.ch>
 * Licensed under the terms of the GNU General Public License version 2.
 */

#include "buzzer.h"
#include "util.h"

#include <avr/io.h>
#include <avr/pgmspace.h>

#include <stdint.h>


struct noteplayer_context {
	uint16_t note_1_1_length_ms;
	bool sharp;
	int8_t octave_shift;
	uint16_t prev_note_ms;
};

static struct noteplayer_context player_ctx;

/* In 0.1 Hz */
static const uint16_t PROGMEM note_freqs[] = {
	[NOTEID_PAUSE]	= 0,
	[NOTEID_C4]	= 2616,		/* c' */
	[NOTEID_D4]	= 2937,		/* d' */
	[NOTEID_E4]	= 3296,		/* e' */
	[NOTEID_F4]	= 3492,		/* f' */
	[NOTEID_G4]	= 3919,		/* g' */
	[NOTEID_A4]	= 4400,		/* a' */
	[NOTEID_B4]	= 4939,		/* h' */
	[NOTEID_C5]	= 5233,		/* c'' */
	[NOTEID_D5]	= 5873,		/* d'' */
	[NOTEID_E5]	= 6593,		/* e'' */
	[NOTEID_F5]	= 6985,		/* f'' */
	[NOTEID_G5]	= 7840,		/* g'' */
	[NOTEID_A5]	= 8800,		/* a'' */
	[NOTEID_B5]	= 9878,		/* h'' */
};

/* In 0.1 Hz */
static const uint16_t PROGMEM note_sharp_freqs[] = {
	[NOTEID_PAUSE]	= 0,
	[NOTEID_C4]	= 2772,		/* cis' */
	[NOTEID_D4]	= 3111,		/* dis' */
	[NOTEID_E4]	= 3296,		/* e' */
	[NOTEID_F4]	= 3700,		/* fis' */
	[NOTEID_G4]	= 4153,		/* gis' */
	[NOTEID_A4]	= 4662,		/* ais' */
	[NOTEID_B4]	= 4939,		/* h' */
	[NOTEID_C5]	= 5544,		/* cis'' */
	[NOTEID_D5]	= 6223,		/* dis'' */
	[NOTEID_E5]	= 6593,		/* e'' */
	[NOTEID_F5]	= 7400,		/* fis'' */
	[NOTEID_G5]	= 8306,		/* gis'' */
	[NOTEID_A5]	= 9323,		/* ais'' */
	[NOTEID_B5]	= 9878,		/* h'' */
};


const note_t PROGMEM buzzer_elise[] = {
	/* line 1, 1st pass */
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	note(E4, 1_16),
	note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	n_sharp, note(G4, 1_16),
	note(B4, 1_16),

	note(C5, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	note(E4, 1_16),
	note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(C5, 1_16),
	note(B4, 1_16),

	note(A4, 1_4),

	/* line 1, 2nd pass */
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	note(E4, 1_16),
	note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	n_sharp, note(G4, 1_16),
	note(B4, 1_16),

	note(C5, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	note(E4, 1_16),
	note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(C5, 1_16),
	note(B4, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(B4, 1_16),
	note(C5, 1_16),
	note(D5, 1_16),

	/* line 2, common */
	note(E5, 1_8), n_dot,
	note(G4, 1_16),
	note(F5, 1_16),
	note(E5, 1_16),

	note(D5, 1_8), n_dot,
	note(F4, 1_16),
	note(E5, 1_16),
	note(D5, 1_16),

	note(C5, 1_8), n_dot,
	note(E4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(E5, 1_16),
	note(E4, 1_16),

	/* line 3 */
	note(E5, 1_8),
	n_octave_up,
	note(E5, 1_16),
	n_octave_down,
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	n_sharp, note(E4, 1_16),
	n_sharp, note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	n_sharp, note(G4, 1_16),
	note(B4, 1_16),

	note(C5, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(E5, 1_16),
	n_sharp, note(D5, 1_16),

	note(E5, 1_16),
	n_sharp, note(D5, 1_16),
	note(E5, 1_16),
	note(B4, 1_16),
	note(D5, 1_16),
	note(C5, 1_16),

	/* line 4 */
	note(A4, 1_8),
	n_pause(1_16),
	note(C4, 1_16),
	note(E4, 1_16),
	note(A4, 1_16),

	note(B4, 1_8),
	n_pause(1_16),
	note(E4, 1_16),
	note(C5, 1_16),
	note(B4, 1_16),

	note(A4, 1_8),

	note_array_end,
};


static uint16_t note_to_ms(note_t n)
{
	uint8_t noteid = (n & NOTE_ID_MASK) >> NOTE_ID_SHIFT;
	uint8_t noteval = (n & NOTE_VAL_MASK) >> NOTE_VAL_SHIFT;

	if (noteid >= NOTEID_FLAGS)
		return 0;
	return player_ctx.note_1_1_length_ms >> noteval;
}

static uint16_t note_to_decihz(note_t n)
{
	uint8_t noteid = (n & NOTE_ID_MASK) >> NOTE_ID_SHIFT;
	int8_t octave_shift;
	uint16_t decihz;

	if (player_ctx.sharp)
		decihz = pgm_read_word(&note_sharp_freqs[noteid]);
	else
		decihz = pgm_read_word(&note_freqs[noteid]);

	octave_shift = player_ctx.octave_shift;
	if (octave_shift < 0)
		decihz >>= (uint8_t)(-octave_shift);
	else
		decihz <<= (uint8_t)octave_shift;

	return decihz;
}

static uint16_t note_to_divider(note_t n)
{
	uint32_t base_freq_hz = F_CPU / 8;
	uint16_t div, decihz;

	decihz = note_to_decihz(n);
	if (!decihz)
		return 0;
	div = base_freq_hz * 10ul / decihz;

	return div;
}

static void buzzer_divider_set(uint16_t divider)
{
	uint16_t divider_half = divider / 2;

	if (divider != ICR1) {
		if (divider) {
			TCCR1A |= (1 << COM1B1) | (1 << COM1B0);
		} else {
			TCCR1A &= ~((1 << COM1B1) | (1 << COM1B0));
			TCCR1B &= ~((1 << CS10) | (1 << CS11) | (1 << CS12));
			PORTB &= ~(1 << PB2);
		}
		TCNT1 = 0;
		ICR1 = divider;
		OCR1B = divider_half;
		if (divider)
			TCCR1B |= (1 << CS11);
	}
}

static noinline void buzzer_delay_ms(uint16_t ms)
{
	while (ms--)
		_delay_ms(1);
}

void buzzer_tune_note(note_t n)
{
	uint8_t noteid = (n & NOTE_ID_MASK) >> NOTE_ID_SHIFT;
	uint16_t divider;

	if (noteid == NOTEID_FLAGS) {
		switch ((n & NOTE_VAL_MASK) >> NOTE_VAL_SHIFT) {
		case NOTEVAL_SHARP:
			player_ctx.sharp = 1;
			break;
		case NOTEVAL_DOT:
			player_ctx.prev_note_ms /= 2;
			buzzer_delay_ms(player_ctx.prev_note_ms);
			break;
		case NOTEVAL_OCTAVE_SH_UP:
			player_ctx.octave_shift++;
			break;
		case NOTEVAL_OCTAVE_SH_DOWN:
			player_ctx.octave_shift--;
			break;
		}
	} else {
		divider = note_to_divider(n);
		buzzer_divider_set(divider);
	}
}

static void buzzer_play_note(note_t n)
{
	uint16_t ms;

	buzzer_tune_note(n);
	ms = note_to_ms(n);
	player_ctx.prev_note_ms = ms;
	player_ctx.sharp = 0;
	buzzer_delay_ms(ms);
}

void buzzer_play(const note_t PROGPTR *notes)
{
	uint16_t i;
	note_t n;

	for (i = 0; ; i++) {
		n = pgm_read_byte(&notes[i]);
		if (n == note_array_end)
			break;
		buzzer_play_note(n);
	}
	buzzer_divider_set(0);
}

void buzzer_init(uint16_t basespeed_note_1_1_ms)
{
	player_ctx.note_1_1_length_ms = basespeed_note_1_1_ms;

	PORTB &= ~(1 << PB2);
	DDRB |= (1 << DDB2);

	TCCR1B = 0;
	buzzer_divider_set(0);
	TCNT1 = 0;
	/* Fast PWM. */
	TCCR1A = (1 << WGM11);
	TCCR1B = (1 << WGM13) | (1 << WGM12);
}
