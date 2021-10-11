
#ifndef _PELTAR_UTIL_H_
#define _PELTAR_UTIL_H_

#include <math.h>

static inline int peltar_hypot(int dx, int dy)
{
	int hypot_squared = dx * dx + dy * dy;
	if (hypot_squared == 0) {
		hypot_squared = 1;
	}

	return sqrt(hypot_squared);
}

#endif
