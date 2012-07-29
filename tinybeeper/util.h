#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#define likely(x)		__builtin_expect(!!(x), 1)
#define unlikely(x)		__builtin_expect(!!(x), 0)

#undef offsetof
#define offsetof(type, member)	((size_t)&((type *)0)->member)

#define min(a, b)		((a) < (b) ? (a) : (b))
#define max(a, b)		((a) > (b) ? (a) : (b))

#define abs(x)	({			\
	__typeof__(x) __x = (x);	\
	(__x < 0) ? -__x : __x;		\
		})

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

/* Memory barrier.
 * The CPU doesn't have runtime reordering, so we just
 * need a compiler memory clobber. */
#define mb()			__asm__ __volatile__("" : : : "memory")

/* Convert something indirectly to a string */
#define __stringify(x)		#x
#define stringify(x)		__stringify(x)

#define BUILD_BUG_ON(x)		((void)sizeof(char[1 - 2 * !!(x)]))

typedef _Bool		bool;
#define true		((bool)(!!1))
#define false		((bool)(!!0))


static inline void irq_disable(void)
{
	cli();
	mb();
}

static inline void irq_enable(void)
{
	mb();
	sei();
}

static inline uint8_t irq_disable_save(void)
{
	uint8_t sreg = SREG;
	cli();
	mb();
	return sreg;
}

static inline void irq_restore(uint8_t sreg_flags)
{
	mb();
	SREG = sreg_flags;
}

#endif /* UTIL_H_ */
