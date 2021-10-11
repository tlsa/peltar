
#ifndef _PELTAR_UTIL_H_
#define _PELTAR_UTIL_H_

#ifdef USE_SQRT
#include <math.h>
#endif

static inline int peltar_hypot(int dx, int dy)
{
#ifdef USE_SQRT
	int hypot_squared = dx * dx + dy * dy;
	if (hypot_squared == 0) {
		hypot_squared = 1;
	}

	return sqrt(hypot_squared);
#else
	uint32_t xu = (dx < 0) ? -dx : dx;
	uint32_t yu = (dy < 0) ? -dy : dy;

	return (xu > yu) ?
			(xu + ((3 * yu) >> 3)) :
			(yu + ((3 * xu) >> 3));
#endif
}

#endif
