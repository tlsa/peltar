
#ifndef _PELTAR_TEXTURE_PLAYER_H_
#define _PELTAR_TEXTURE_PLAYER_H_

#include <stdint.h>
#include "../image.h"

struct point_3d;

uint32_t texture_man_made_32bpp(struct point_3d p,
		peltar_fixed dist, peltar_fixed max_dist,
		const uint32_t seeds[4], int s, uint32_t colour);

#endif

