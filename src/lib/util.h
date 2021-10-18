
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

static inline int peltar_round_up_n(int value, int n)
{
	return (value + n - 1) / n * n;
}

static inline uint32_t blend_colour(uint32_t a, uint32_t b)
{
	return (((((a & 0xff00ff) + (b & 0xff00ff)) >> 1) & 0xff00ff) |
	        ((((a & 0x00ff00) + (b & 0x00ff00)) >> 1) & 0x00ff00));
}

#endif
