
#ifndef _PELTAR_TEXTURE_EARTH_LIKE_H_
#define _PELTAR_TEXTURE_EARTH_LIKE_H_

#include <stdint.h>

#include "../image.h"
#include "../colours.h"

struct point_3d;

struct colour texture_earth_like_planet_32bpp(struct point_3d p,
		const uint32_t seeds[4], int s, uint32_t radius, uint32_t y);

#endif

