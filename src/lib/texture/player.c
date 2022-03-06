
#include "../../noise/noise.h"
#include "../fixed-point.h"
#include "../image.h"
#include "../types.h"
#include "player.h"

struct colour texture_man_made_32bpp(struct point_3d p,
		peltar_fixed dist, peltar_fixed max_dist,
		const uint32_t seeds[4], int s, struct colour c)
{
	peltar_fixed n;
	uint32_t r, g, b;

	max_dist *= 4;
	dist *= 3;

	r = (c.r * dist / max_dist) & 0xff;
	g = (c.g * dist / max_dist) & 0xff;
	b = (c.b * dist / max_dist) & 0xff;

	n  = (noise_get_value_at_pos_standard(p, seeds[0], s  ) >> 24) / 8;
	n += (noise_get_value_at_pos_standard(p, seeds[3], s/2) >> 24) / 8;

	r += n;
	g += n;
	b += n;

	return (struct colour){ .r = r, .b = b, .g = g, };
}
