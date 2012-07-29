/*
 *  Morse code beeper
 *
 *  Copyright (C) 2010 Michael Buesch <mb@bu3sch.de>
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

#include <util/delay.h>

#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include "../encoder/morse_encoder.h"


/* Beeper connection */
#define MORSE_BEEPER_PORT	PORTB
#define MORSE_BEEPER_DDR	DDRB
#define MORSE_BEEPER_BIT	0


/* The morse symbols, in EEPROM. */
static const morse_sym_t EEMEM morse_symbols[32];


/* Beeper context */
enum {
	BEEPER_STOPPED,
	BEEPER_NEXTSYM,
	BEEPER_INSYM,
};
static uint8_t beeper_state;
static uint16_t timer;
static uint16_t counter;
static uint8_t morse_sym_ptr;		/* Symbol pointer */
static uint8_t morse_sym_size;		/* Size of the current symbol */
static uint16_t morse_sym_marks;	/* The marks of the current symbol */

#define TIMER_INBEEP		1
#define TIMER_DIT		(TIMER_INBEEP * 240)
#define TIMER_DAH		(TIMER_DIT * 3)
#define TIMER_INTERMARK		(TIMER_DIT * 1)
#define TIMER_INTERCHAR		(TIMER_DIT * 3)
#define TIMER_INTERWORD		(TIMER_DIT * 7)


/* 2 kHz trigger frequency */
ISR(TIM0_COMPA_vect)
{
	if (timer)
		timer--;
}

static inline void timer_set(uint16_t value)
{
	timer = value;
}

static inline uint8_t timer_running(void)
{
	return (timer != 0);
}

static void timer_init(void)
{
	TCCR0B = 0; /* Disable */
	TIFR0 |= (1 << OCF0A);
	TIMSK0 |= (1 << OCIE0A); /* Enable compA IRQ */
	TCNT0 = 0;
	OCR0A = 38;
	TCCR0A = (1 << WGM01); /* CTC */
	TCCR0B = (0 << CS02) | (1 << CS01) | (1 << CS00); /* prescaler 64 */
}

static void beeper_startup(void)
{
	beeper_state = BEEPER_NEXTSYM;
	morse_sym_ptr = 0;
}

static void beeper_stop(void)
{
	beeper_state = BEEPER_STOPPED;
	MORSE_BEEPER_PORT &= ~(1 << MORSE_BEEPER_BIT);
}

static inline uint8_t int0_triggered(void)
{
	return !(PINB & (1 << 1));
}

ISR(INT0_vect)
{
	if (beeper_state == BEEPER_STOPPED)
		beeper_startup();
	else
		beeper_stop();
	while (1) {
		while (int0_triggered())
			; /* Wait for release */
		_delay_ms(50);
		if (!int0_triggered())
			break;
	}
}

/* Runs with IRQs disabled. */
static void run_beeper(void)
{
	morse_sym_t sym;

	if (timer_running())
		return; /* delay */

	switch (beeper_state) {
	case BEEPER_NEXTSYM:
		if (morse_sym_ptr >= ARRAY_SIZE(morse_symbols)) {
			beeper_stop();
			return;
		}
		sym = eeprom_read_word(&morse_symbols[morse_sym_ptr]);
		if (sym == 0xFFFF) {
			beeper_stop();
			return;
		}
		morse_sym_ptr++;
		if (MORSE_SYM_IS_SPACE(sym)) {
			timer_set(TIMER_INTERWORD);
			return;
		}
		morse_sym_marks = MORSE_SYM_MARKS(sym);
		morse_sym_size = MORSE_SYM_SIZE(sym);
		beeper_state = BEEPER_INSYM;
		timer_set(TIMER_INBEEP);
		counter = 0;
		/* fallthrough */
	case BEEPER_INSYM:
		if (counter == 0) {
			if (!morse_sym_size) {
				beeper_state = BEEPER_NEXTSYM;
				timer_set(TIMER_INTERCHAR);
				return;
			}
			if ((morse_sym_marks & 1) == MORSE_DIT)
				counter = TIMER_DIT;
			else /* dah */
				counter = TIMER_DAH;
			morse_sym_size--;
			morse_sym_marks >>= 1;
		} else {
			counter--;
			if (counter == 0) {
				timer_set(TIMER_INTERMARK);
				MORSE_BEEPER_PORT &= ~(1 << MORSE_BEEPER_BIT);
				return;
			}
		}
		MORSE_BEEPER_PORT ^= (1 << MORSE_BEEPER_BIT);
		timer_set(TIMER_INBEEP);
		break;
	}
}

int main(void)
{
	irq_disable();

	PORTB = 0xFF;
	DDRB = 0x00;

	MORSE_BEEPER_DDR |= (1 << MORSE_BEEPER_BIT);
	MORSE_BEEPER_PORT &= ~(1 << MORSE_BEEPER_BIT);

	/* Set up INT0 */
	MCUCR |= (0 << ISC01) | (0 << ISC00); /* low level triggered */
	PORTB |= (1 << 1); /* enable pullup */
	DDRB &= ~(1 << 1);
	GIMSK |= (1 << INT0);

	timer_init();

	set_sleep_mode(SLEEP_MODE_IDLE);
	sleep_enable();
	while (1) {
		mb();
		if (beeper_state == BEEPER_STOPPED)
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		else
			set_sleep_mode(SLEEP_MODE_IDLE);
		irq_enable();
		sleep_cpu();
		irq_disable();

		if (beeper_state != BEEPER_STOPPED)
			run_beeper();
	}
}
