#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>

#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))

#define BUILD_BUG_ON(x)		((void)sizeof(char[1 - 2 * !!(x)]))

typedef _Bool		bool;
#define true		((bool)(!!1))
#define false		((bool)(!!0))

#endif /* UTIL_H_ */
