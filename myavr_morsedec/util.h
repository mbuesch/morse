/**
 * \file util.h
 * \brief Hilfsfunktionen
 */
#ifndef MY_UTIL_H_
#define MY_UTIL_H_

#ifndef F_CPU
# warning "F_CPU not defined"
# define F_CPU	3686400
#endif
#include <util/delay.h>

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/cpufunc.h>


/** \brief Gibt den kleinsten Wert der Parameter a und b zurueck. */
#define min(a, b)	({			\
		__typeof__(a) __a = (a);	\
		__typeof__(b) __b = (b);	\
		__a < __b ? __a : __b;		\
	})

/** \brief Gibt den groessten Wert der Parameter a und b zurueck. */
#define max(a, b)	({			\
		__typeof__(a) __a = (a);	\
		__typeof__(b) __b = (b);	\
		__a > __b ? __a : __b;		\
	})

/** \brief Begrenzt value zwischen min_val und max_val. */
#define clamp(value, min_val, max_val)		\
	max(min(value, max_val), min_val)

/** \brief Gibt den Absolutwert des val Parameters zurueck. */
#define abs(val)	({			\
		__typeof__(val) __val = (val);	\
		__val >= 0 ? __val : -__val;	\
	})

/** \brief Gibt die Groesse eines Arrays in Anzahl der Elemente zurueck. */
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

/* Progmem pointer annotation. */
#define PROGPTR			/* progmem pointer */

/** \brief	Memory barrier.
 * Schraenkt die Compileroptimierung ueber den Aufrufspunkt
 * hinweg ein. */
#define mb()			__asm__ __volatile__("" : : : "memory")


#define noinline        __attribute__((__noinline__))


typedef _Bool		bool;


/** \brief	Interrupts global deaktivieren.
 *
 * Deaktiviert Interrupts und fuehrt eine Memory-Barrier durch.
 */
static inline void irq_disable(void)
{
	cli();
	mb();
}

/** \brief	Interrupts global aktivieren.
 *
 * Fuehrt eine Memory-Barrier durch und aktiviert Interrupts global.
 */
static inline void irq_enable(void)
{
	mb();
	sei();
}

/** \brief	Interrupts global deaktivieren und Status zurueckgeben.
 *
 * Deaktiviert Interrupts und fuehrt eine Memory-Barrier durch.
 * Gibt das vorherige SREG zurueck.
 */
static inline uint8_t irq_disable_save(void)
{
	uint8_t sreg = SREG;
	cli();
	mb();
	return sreg;
}

/** \brief	Interrupts wiederherstellen.
 *
 * Fuehrt eine Memory-Barrier durch und stellt Interrupts wieder her.
 */
static inline void irq_restore(uint8_t sreg_flags)
{
	mb();
	SREG = sreg_flags;
}

#endif /* MY_UTIL_H_ */
