/**
 * \file main.c
 * \brief Morsedecoder
 *
 * \subsection Copyright
 * Copyright (c) 2012 Michael Buesch <m@bues.ch>
 *
 * \subsection License
 * Licensed under the terms of the GNU General Public License version 2.
 */

/** \mainpage
 *
 * \author Michael Buesch <m@bues.ch>
 *
 * \section a Aufgabenstellung
 *	Projektarbeit in M06 am BNT Trier:
 *	Es soll ein myAVR-basierter Morsedecoder erstellt werden.
 *	Morsecodes werden ueber einen digitalen Eingabetaster
 *	eingegeben und decodierte ASCII Zeichen werden am
 *	LCD ausgegeben. Wird ein Symbol nicht erkannt, wird
 *	ein Fehlerzeichen ":(" auf dem LCD ausgegeben.
 *	Die Morsegeschwindigkeit wird ueber das Poti zwischen 1 und
 *	20 "Woertern pro Minute" (WpM) eingestellt.
 *
 * \section b Hardware
 *	myAVR Basisboard mit myAVR LCD Modul.
 *
 * \section c Elektrische Anschluesse
 *	\li Port B0 => Morsetaster.
 *	\li Port B1 => Cleartaster.
 *	\li Port B2 => Summer.
 *	\li Port C0 => Poti.
 *	\li \image latex pinout.jpg
 *	    \image html pinout.jpg
 *
 * \section d Kurzbeschreibung der Funktion
 *	Die Morsesymbolerkennung geschieht asynchron ueber einen
 *	Timer, der regelmaessig den Morsetaster abtastet und
 *	einen Zustandsautomat beeinflusst.
 *	Erkannte Symbole werden in einem temporaeren Puffer
 *	zwischengespeichert. Die Decodierung und Ausgabe der Symbole wird
 *	im Hauptzyklus des Programms durchgefuehrt. Waehrenddessen
 *	koennen neue Symbole im Interrupt erkannt und
 *	zwischengespeichert werden.
 *
 * \section e Interruptsynchronisation
 *	Die Synchronisierung zwischen Hauptprogramm und Interrupt
 *	geschieht ueber Interrupt-sperren
 *	(irq_disable(), irq_enable()) und Memory-Barriers
 *	(mb(), http://en.wikipedia.org/wiki/Memory_barrier).
 *	Durch den Einsatz von Memory-Barriers wird bewusst auf
 *	die Deklaration der Speicherbereiche als "volatile" verzichtet.
 *	Damit hat der Compiler groessere Freiheiten bei der Optimierung
 *	und generiert bei korrektem Einsatz der mb()s trotzdem
 *	korrekten Maschinencode.
 *
 * \section f Compilieren unter Windows
 *	Die GNU AVR-GCC Toolchain fuer Windows muss installiert sein.
 *	Durch das Ausfuehren der 'compile.bat' Datei wird das Projekt
 *	neu uebersetzt und eine neue .hex Datei erstellt.
 */

#include "util.h"
#include "lcd.h"
#include "morse.h"
#include "buzzer.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdint.h>
#include <string.h>
#include <ctype.h>


/** Versionsnummer. */
enum version_number {
	VERSION_MAJOR	= 1,
	VERSION_MINOR	= 1,
};

/** Systemweite Parameter. */
enum global_parameters {
	/** Anzahl der Mikrosekunden pro Tick. */
	US_PER_TICK		= 11111,
	/** Minimale "Words per minute" Erkennungsrate. */
	MIN_WPM			= 1,
	/** Maximale "Words per minute" Erkennungsrate. */
	MAX_WPM			= 20,
	/** Groesse des capture-Puffers.
	 * In Anzahl von Morsesymbolen. */
	CAPTURE_BUF_SIZE	= 4,
	/** Laenge des Textausgabepuffers am LCD. */
	OUT_TEXT_LEN		= 16,
	/** Tastenentprellungszeit in Ticks. */
	DEBOUNCE_TICKS		= 4,
};

/** Rueckgabetyp fuer Flankenerkennung. */
enum edge_detect_result {
	/** Keine Flanke erkannt. */
	EDGE_NONE,
	/** Positive Flanke erkannt. */
	EDGE_POS,
	/** Negative Flanke erkannt. */
	EDGE_NEG,
};

/** Morse Symbolerkennung Context. */
struct symbol_capture_context {
	/** Flankenmerker fuer Morsetaster. */
	bool morse_button_prev;

	/** Boolscher Morsetonzustand.
	 *	1 -> Wir befinden uns in einem Morseton ("dit" oder "dah").
	 *	0 -> Wir befinden uns nicht in einem Morseton (Pause).
	 */
	bool in_mark;

	/** Anzahl erkannter Morsetoene ("dit"s und "dah"s). */
	uint8_t cur_mark_nr;

	/** Puffer fuer das aktuelle Morsesymbol.
	 * Das Symbol ist moeglicherweise unvollstaendig waehrend der Erkennung.
	 */
	morse_sym_t cur_symbol;

	/** Puffer fuer vollstaendig empfangene Morsesymbole. */
	morse_sym_t captured[CAPTURE_BUF_SIZE];
	/** Anzahl der vollstaendig empfangenen Morsesymbole. */
	uint8_t nr_captured;
	/** Fehlermerker fuer Capture-Fehler. */
	bool capture_error;

	/** Zaehler fuer Morsetonerkennung und Unterscheidung. */
	uint8_t ticks;

	/** Eingestellte "Wort pro Minute" Erkennungsgeschwindigkeit. */
	uint8_t wpm;
	/** Eingestellte "dit" Erkennungsgeschwindigkeit in "ticks". */
	uint8_t ticks_dit;
	/** Eingestellte "dah" Erkennungsgeschwindigkeit in "ticks". */
	uint8_t ticks_dah;
	/** Eingestellte Symbolpause Erkennungsgeschwindigkeit in "ticks". */
	uint8_t ticks_inter_char;
	/** Eingestellte Wortpause Erkennungsgeschwindigkeit in "ticks". */
	uint8_t ticks_inter_word;
};

/** Ausgabecontext. */
struct output_context {
	/** ASCII Text zur LCD-Ausgabe. */
	char text[OUT_TEXT_LEN + 1]; 
};

/** Generischer Maschinenstatus. */
struct machine_context {
	/** Flankenmerker fuer Cleartaster. */
	bool clear_button_prev;
	/** Zaehler fuer Cleartaster Entprellung. */
	uint8_t clear_button_pause;

	/** Asynchrone LCD Updateaufforderung aus Interrupts. */
	bool async_lcd_update;
};

/** Morse Symbolerkennung Context Instanz. */
static struct symbol_capture_context capture;

/** Ausgabecontext Instanz. */
static struct output_context out;

/** Maschinenstatus Instanz. */
static struct machine_context machine;


/** \brief	Eingabetaster initialisieren.
 *
 * PB0 -> Morsetaster (mit internem Pullup).
 * PB1 -> Cleartaster (mit internem Pullup).
 */
static void buttons_init(void)
{
	DDRB &= ~((1 << DDB0) | (1 << DDB1));
	PORTB |= (1 << PB0) | (1 << PB1);
}

/** \brief		Generische Flankenerkennung.
 *
 * \param sig_status	Aktueller Signalstatus.
 *
 * \param prev_status	Zeiger auf Flankenmerker fuer dieses Signal.
 *
 * \return		Gibt das Erkennungsergebnis als
 *			#edge_detect_result zurueck.
 */
static enum edge_detect_result generic_edge_detect(bool sig_status,
						   bool *prev_status)
{
	bool pos_edge, neg_edge;

	pos_edge = sig_status & ~(*prev_status);
	neg_edge = ~sig_status & (*prev_status);
	*prev_status = sig_status;

	if (pos_edge)
		return EDGE_POS;
	if (neg_edge)
		return EDGE_NEG;
	return EDGE_NONE;
}

/** \brief	Morsetaster Abfrage.
 *
 * \return	Gibt 1 bei gedruecktem Taster und ansonsten 0 zurueck.
 */
static bool morse_button_pressed(void)
{
	return !(PINB & (1 << PINB0));
}

/** \brief	Morsetaster Flankenerkennung.
 *
 * \return	Gibt das Ergebnis der Flankenerkennung am Morsetaster
 *		als #edge_detect_result zurueck.
 */
static enum edge_detect_result get_morse_button_edge(void)
{
	return generic_edge_detect(morse_button_pressed(),
				   &capture.morse_button_prev);
}

/** \brief	Cleartaster Abfrage.
 *
 * \return	Gibt 1 bei gedruecktem Taster und ansonsten 0 zurueck.
 */
static bool clear_button_pressed(void)
{
	return !(PINB & (1 << PINB1));
}

/** \brief	Cleartaster Flankenerkennung.
 *
 * Gibt das Ergebnis der Flankenerkennung am Cleartaster zurueck.
 */
static enum edge_detect_result get_clear_button_edge(void)
{
	return generic_edge_detect(clear_button_pressed(),
				   &machine.clear_button_prev);
}

/** \brief	Fuegt ein Morsesymbol zum Symbolpuffer hinzu.
 *
 * \param sym	Das Morsesymbol welches zum Symbolpuffer hinzugefuegt werden soll.
 *
 * \return	Gibt 0 bei Erfolg oder einen negativen Fehlercode zurueck.
 */
static int8_t add_captured_symbol(morse_sym_t sym)
{
	if (capture.nr_captured >= ARRAY_SIZE(capture.captured))
		return -1; /* Pufferueberlauf */

	capture.captured[capture.nr_captured] = sym;
	capture.nr_captured++;

	return 0;
}

/** \brief	Erkennungstimings setzen.
 *
 * Hilfsfunktion.
 *
 * \param dit_base_ticks	Anzahl Ticks in einem Morse-"dit".
 */
static void set_timings(uint8_t dit_base_ticks)
{
	capture.ticks_dit = dit_base_ticks * FACTOR_DIT;
	capture.ticks_dah = capture.ticks_dit * FACTOR_DAH;
	capture.ticks_inter_char = capture.ticks_dit * FACTOR_INTER_CHAR;
	capture.ticks_inter_word = capture.ticks_dit * FACTOR_INTER_WORD;
}

/** \brief	Erkennungsgeschwindigkeit setzen.
 *
 * \param wpm	Erkennungsgeschwindigkeit in "Words per minute".
 *		Ein Wort is definiert als 5 Morsezeichen.
 */
static void set_words_per_minute(uint8_t wpm)
{
	uint32_t dit_len;
	uint8_t dit_ticks, sreg;

	/* WpM Bereich eingrenzen. */
	wpm = clamp(wpm, MIN_WPM, MAX_WPM);

	/* Laenge eines "dit" Tones in Mikrosekunden berechnen. */
	dit_len = (uint32_t)DIT_LENGTH_1WPM_MS * 1000;
	dit_len /= wpm;

	/* Laenge eines "dit" Tones in "Ticks" umrechnen.
	 * Rundungsfehler werden ignoriert. */
	dit_ticks = dit_len / US_PER_TICK;

	/* Wenn der WpM Wert vom aktuellen abweicht,
	 * Symbolzeiten neu setzen. */
	sreg = irq_disable_save();
	if (wpm != capture.wpm) {
		capture.wpm = wpm;
		set_timings(dit_ticks);
		machine.async_lcd_update = 1;
	}
	irq_restore(sreg);
}

/** \brief	Summer einschalten. */
static void buzzer_turn_on(void)
{
	buzzer_tune_note(note(A5, 1_1));
}

/** \brief	Summer abschalten. */
static void buzzer_turn_off(void)
{
	buzzer_tune_note(n_pause(1_1));
}

/** \brief	Symbolerkennungstimer Interrupt Service Routine */
ISR(TIMER2_COMP_vect)
{
	enum edge_detect_result button_edge;
	morse_sym_t new_mark;
	int8_t err;

	mb();

	/* Morsetaster Flankenerkennung durchfuehren. */
	button_edge = get_morse_button_edge();

	/* Tickzaehler hochzaehlen. */
	if (capture.ticks < 0xFF)
		capture.ticks++;

	if (button_edge == EDGE_NEG) {
		/* Negative Flanke am Morsetaster erkannt. */

		/* Summer abschalten. */
		buzzer_turn_off();

		/* Anhand der Tonlaenge entscheiden ob es ein "dit" oder
		 * ein "dah" Ton war und den Ton zum aktuellen
		 * Symbol hinzufuegen. */
		if (capture.cur_mark_nr < MORSE_MAX_NR_MARKS) {
			if (capture.ticks <= capture.ticks_dit)
				new_mark = MORSE_MARK(MORSE_DIT, capture.cur_mark_nr);
			else
				new_mark = MORSE_MARK(MORSE_DAH, capture.cur_mark_nr);
			capture.cur_symbol |= new_mark;
			capture.cur_mark_nr++;
		} else
			capture.capture_error = 1;

		/* Tonzaehler und Tonzustand ruecksetzen. */
		capture.in_mark = 0;
		capture.ticks = 0;
	}

	if (button_edge == EDGE_POS) {
		/* Positive Flanke am Morsetaster erkannt. */

		/* Summer einschalten. */
		buzzer_turn_on();

		/* Tonzaehler ruecksetzen und Tonzustand setzen. */
		capture.in_mark = 1;
		capture.ticks = 0;
	}

	if (capture.in_mark) {
		/* Wir befinden uns in einem "Morseton". */

		/* Summer abschalten, wenn Signal laenger als ein "dah" ist. */
		if (capture.ticks > capture.ticks_dah)
			buzzer_turn_off();
	} else {
		/* Wir befinden uns in einer "Morsepause". */

		if (capture.ticks == capture.ticks_inter_char) {
			/* Pause hat eine inter-char Laenge.
			 * D.h. ein ganzes Symbol wurde empfangen. */
			morse_sym_set_size(&capture.cur_symbol,
					   capture.cur_mark_nr);
			err = add_captured_symbol(capture.cur_symbol);
			if (err)
				capture.capture_error = 1;
			capture.cur_mark_nr = 0;
			capture.cur_symbol = 0;
		} else if (capture.ticks == capture.ticks_inter_word) {
			/* Pause hat eine inter-Wort Laenge.
			 * D.h. ein ganzes Wort wurde empfangen.
			 * Wort mit einem Leerzeichen (MORSE_SPACE) abschliessen. */
			err = add_captured_symbol(morse_encode_character(MORSE_SPACE));
			if (err)
				capture.capture_error = 1;
		}
	}

	/* Entprellzaehler fuer Cleartaster runterzaehlen. */
	if (machine.clear_button_pause)
		machine.clear_button_pause--;

	mb();
}

/** \brief	Symbolerkennungstimer initialisieren. */
static void timer_init(void)
{
	/* CTC Timer initialisieren auf:
	 * 3686400 Hz / 1024 / 40 = 90 Hz */
	TCCR2 = (1 << CS20) | (1 << CS21) | (1 << CS22) |
		(1 << WGM21);
	OCR2 = 40;
	/* OCR Interrupt aktivieren. */
	TIMSK |= (1 << OCIE2);
}

/** \brief	Analogwert von Poti mit virtueller Rastung erfassen.
 *
 * Die virtuelle Rastung verhindert ein Kippeln und Schwingen
 * zwischen Zustaenden, wenn das Poti nahe eines
 * Umschaltpunktes steht.
 */
ISR(ADC_vect)
{
	uint16_t count_per_notch, adc, pos, frac, wpm;
	static bool first_run = 1;

	mb();

	/* ADC counts pro "Raste" errechnen.
	 * Hier kommt es zu Rundungsfehlern. */
	count_per_notch = (0x3FF + 1) / MAX_WPM;

	/* ADC Wert auslesen und um eine halbe "Raste"
	 * nach unten verschieben. */
	adc = ADCW;
	if (adc >= count_per_notch / 2)
		adc -= count_per_notch / 2;

	/* "Rastenposition" errechnen.
	 * Das Ergebnis ist um 10 skaliert um
	 * Zwischenzustaende ohne Fliesspunktzahlen zu erkennen. */
	pos = adc * 10 / count_per_notch;

	/* WpM und Abweichung von der "Raste" errechnen. */
	wpm = pos / 10 + 1;
	frac = pos % 10;

	/* Wenn wir uns nahe an der naechsten "Raste" befinden,
	 * WpM aufrunden. */
	if (frac > 7)
		wpm++;

	/* Wert nur uebernehmen, wenn wir uns nahe einer "Raste" befinden.
	 * Zwischen den "Rasten" befindet ein toter Bereich (0.3 bis 0.7)
	 * in dem nicht auf Potiaenderungen reagiert wird.
	 * Dies verhindert ein Schwingen zwischen Zustaenden. */
	if (frac > 7 || frac < 3 || first_run) {
		first_run = 0;
		set_words_per_minute(clamp(wpm, MIN_WPM, MAX_WPM));
	}

	mb();
}

/** \brief	ADC initialisieren */
static void adc_init(void)
{
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2);

	/* Eine ADC Konvertierung durchfuehren und Ergebnis verwerfen. */
	ADCSRA |= (1 << ADSC);
	while (ADCSRA & (1 << ADSC));
	(void)ADCW;

	/* ADC im free-running Modus mit Interrupts aktivieren. */
	ADCSRA |= (1 << ADSC) | (1 << ADFR) | (1 << ADIF) | (1 << ADIE);
}

/** \brief	Informationen auf LCD ausgeben. */
static void update_lcd(void)
{
	uint8_t wpm;

	irq_disable();
	wpm = capture.wpm;
	irq_enable();

	lcd_cursor(0, 0);
	lcd_printf("mdec-%d.%d", VERSION_MAJOR, VERSION_MINOR);
	lcd_cursor(0, 10);
	lcd_printf("%2d WpM", wpm);

	lcd_cursor(1, 0);
	lcd_put_mstr(out.text);
	lcd_commit();
}

/** \brief	Textausgabepuffer loeschen. */
static void clear_output_text(void)
{
	memset(out.text, ' ', OUT_TEXT_LEN);
	out.text[OUT_TEXT_LEN] = '\0';
}

/** \brief	Symbolerkennungscontext ruecksetzen. */
static void reset_capture_context(void)
{
	uint8_t sreg;

	sreg = irq_disable_save();

	capture.in_mark = 0;
	capture.cur_mark_nr = 0;
	capture.cur_symbol = 0;
	capture.nr_captured = 0;
	capture.capture_error = 0;
	capture.ticks = 0;

	irq_restore(sreg);
}

/** \brief	Maschinenzustand initialisieren. */
static void machine_state_init(void)
{
	clear_output_text();
	set_words_per_minute(10);
	reset_capture_context();
	update_lcd();
}

/** \brief	Char-array nach links verschieben.
 *
 * \param array		Char-array welches geschoben werden soll.
 *
 * \param size		Groesse des arrays.
 *
 * \param count		Anzahl der Stellen um die nach links geschoben
 *			werden soll.
 */
static void left_shift_char_array(char *array, uint8_t size,
				  uint8_t count)
{
	uint8_t i;

	for (i = count; i < size; i++)
		array[i - count] = array[i];
}

/** \brief		Morse Symbole decodieren und in LCD Puffer uebertragen.
 *
 * \param syms		Array von Morse Symbolen.
 *
 * \param count		Anzahl der Morse Symbole.
 *
 * \param decode_error	Fehlermerker fuer Decoderfehler.
 *
 * \return		Gibt die Anzahl der generierten ASCII Zeichen zurueck.
 */
static uint8_t decode_symbols(const morse_sym_t *syms,
			      uint8_t count,
			      bool decode_error)
{
	char ascii[OUT_TEXT_LEN];
	uint8_t i, nr_decoded = 0;
	enum morse_character mc;
	bool prev_char_was_space, cur_is_space;
	int8_t res;

	/* Leerzeichenmerker initialisieren mit letztem LCD Zeichen. */
	prev_char_was_space = isspace(out.text[OUT_TEXT_LEN - 1]);

	/* Morse Symbole decodieren und in LCD-Puffer schreiben. */
	for (i = 0; i < count; i++) {
		/* Morsesymbol decodieren. */
		mc = morse_decode_symbol(syms[i]);
		res = morse_to_ascii(ascii, ARRAY_SIZE(ascii), mc);

		if (res > 0 && !decode_error && mc != MORSE_SIG_ERROR) {
			/* Erfolgreich decodiert. */

			/* Keine doppelten Leerzeichen ausgeben. */
			cur_is_space = (res == 1 && isspace(ascii[0]));
			if (cur_is_space && prev_char_was_space)
				continue;
			prev_char_was_space = cur_is_space;

			/* LCD-Puffer um die Anzahl der decodierten Zeichen
			 * nach links verschieben. */
			left_shift_char_array(out.text, OUT_TEXT_LEN, res);

			/* Decodierte ASCII Zeichen in LCD-Puffer schreiben. */
			memcpy(&out.text[OUT_TEXT_LEN - res], ascii, res);
			nr_decoded += res;
		} else {
			/* Decoderfehler */

			/* LCD-Puffer um eins nach links verschieben. */
			left_shift_char_array(out.text, OUT_TEXT_LEN, 1);

			/* Fehlermarker ausgeben. */
			out.text[OUT_TEXT_LEN - 1] = '\x01';
			nr_decoded++;

			decode_error = 0;
		}
	}

	return nr_decoded;
}

/** \brief	Cleartaster-Ereignisse abarbeiten. */
static void handle_clear_button(void)
{
	enum edge_detect_result edge;

	irq_disable();
	if (machine.clear_button_pause) {
		irq_enable();
		/* Entprellpause ist aktiv.
		 * Keine Erkennung durchfuehren. */
		return;
	}
	irq_enable();

	/* Flankenerkennung des Cleartasters aufrufen. */
	edge = get_clear_button_edge();

	if (edge == EDGE_POS) {
		/* Positive Flanke: Cleartaster wurde gedrueckt.
		 * Morsestring im LCD loeschen. */
		if (memcmp(&out.text[OUT_TEXT_LEN - 5], " BNT ", 5) == 0) {
			irq_disable();
			buzzer_play(buzzer_elise);
			irq_enable();
		}
		clear_output_text();
		reset_capture_context();
		update_lcd();
	}
	if (edge != EDGE_NONE) {
		/* Es gab eine positive oder negative Flanke.
		 * Entprellzeit einstellen. */
		irq_disable();
		machine.clear_button_pause = DEBOUNCE_TICKS;
		irq_enable();
	}
}

/** \brief	Ereignisse mit niedriger Prioritaet abarbeiten. */
static void handle_events(void)
{
	morse_sym_t captured[CAPTURE_BUF_SIZE];
	uint8_t nr_captured = 0, nr_decoded;
	bool capture_error;
	bool had_async_lcd_update;

	/* Cleartaster abfragen. */
	handle_clear_button();

	/* Empfangene Symbole in temporaeren lokalen Puffer kopieren
	 * und Interrupts so schnell wie moeglich wieder freigeben. */
	irq_disable();
	if (capture.nr_captured) {
		nr_captured = capture.nr_captured;
		capture.nr_captured = 0;
		memcpy(captured, capture.captured,
		       nr_captured * sizeof(captured[0]));
	}
	capture_error = capture.capture_error;
	capture.capture_error = 0;
	irq_enable();

	/* Empfangene Symbole dekodieren und in LCD Puffer uebertragen. */
	nr_decoded = decode_symbols(captured, nr_captured,
				    capture_error);

	/* Asynchrones LCD-update Flag abfragen und ruecksetzen. */
	irq_disable();
	had_async_lcd_update = machine.async_lcd_update;
	machine.async_lcd_update = 0;
	irq_enable();

	/* LCD auffrischen, wenn neue Symbole decodiert wurden,
	 * oder ein asynchrones LCD Update angefordert wurde. */
	if (nr_decoded || had_async_lcd_update)
		update_lcd();
}

/** \brief	Einsprungspunkt */
int main(void)
{
	buttons_init();
	adc_init();
	timer_init();
	buzzer_init(4000);
	lcd_init();

	machine_state_init();

	irq_enable();
	while (1)
		handle_events();
}
