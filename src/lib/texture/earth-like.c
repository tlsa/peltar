
#include "../../noise/noise.h"
#include "../fixed-point.h"
#include "../image.h"
#include "../types.h"
#include "../colours.h"
#include "earth-like.h"


enum earth_like_terrain_type {
	DESERT,
	DESERT_GRASS,
	GRASS,
	GRASS_FOREST,
	FOREST
};

static inline int interpolate(int64_t a, int64_t b, int f)
{
	if (a < b)
		return a + (((b - a) * f) / FIX_MULTIPLE);
	else
		return b + (((a - b) * (FIX_MULTIPLE - f)) / FIX_MULTIPLE);
}

static inline void texture_earth_like_get_thresholds(int y, int r,
		int levels[4])
{
	int pos;

	/* Consider distance from nearest pole */
	if (y >= r)
		y = 2 * r - y;

#define Y_THRESH_1 (     r  / 8)
#define Y_THRESH_2 (     r  / 4)
#define Y_THRESH_3 ((3 * r) / 4)

#define EQUATOR_D_DG (( (0x7fffffff / 16) *  7) / 2 + (0x7fffffff / 4))
#define EQUATOR_DG_G (( (0x7fffffff / 16) *  9) / 2 + (0x7fffffff / 4))
#define EQUATOR_G_GF (( (0x7fffffff / 16) *  9) / 2 + (0x7fffffff / 4) + 1)
#define EQUATOR_GF_F (( (0x7fffffff / 16) * 11) / 2 + (0x7fffffff / 4))

#define     MID_D_DG (( (0x0            )     ) / 2 + (0x7fffffff / 4))
#define     MID_DG_G (( (0x7fffffff / 16) *  6) / 2 + (0x7fffffff / 4))
#define     MID_G_GF (( (0x7fffffff / 16) *  7) / 2 + (0x7fffffff / 4))
#define     MID_GF_F (( (0x7fffffff / 16) * 10) / 2 + (0x7fffffff / 4))

#define    POLE_D_DG ((-(0x7fffffff /  3)     ) / 2 + (0x7fffffff / 4))
#define    POLE_DG_G (( (0x0            )     ) / 2 + (0x7fffffff / 4))
#define    POLE_G_GF (( (0x7fffffff /  3)     ) / 2 + (0x7fffffff / 4))
#define    POLE_GF_F (( (0x7fffffff /  3) *  2) / 2 + (0x7fffffff / 4))

	/* Set thresholds for:
	 *   D_DG:                  desert --> desert-grass transition
	 *   DG_G: desert-grass transition --> grass
	 *   G_GF:                   grass --> grass-forest
	 *   GF_F: grass-forest transition --> forest transition
	 */
	if (y < Y_THRESH_1) {
		/* Near pole; no desert, no desert-grass transition */
		/* Constant thresholds */
		levels[0] = POLE_D_DG;
		levels[1] = POLE_DG_G;
		levels[2] = POLE_G_GF;
		levels[3] = POLE_GF_F;

	} else if (y < Y_THRESH_2) {
		/* No desert, allow desert-grass transition */
		/* Scale thresholds with y */
		pos = ((y - Y_THRESH_1) << FIX_SHIFT) /
				(Y_THRESH_2 - Y_THRESH_1);
		levels[0] = interpolate(POLE_D_DG, MID_D_DG, pos);
		levels[1] = interpolate(POLE_DG_G, MID_DG_G, pos);
		levels[2] = interpolate(POLE_G_GF, MID_G_GF, pos);
		levels[3] = interpolate(POLE_GF_F, MID_GF_F, pos);

	} else if (y < Y_THRESH_3) {
		/* Allow desert, scale thresholds with y */
		pos = ((y - Y_THRESH_2) << FIX_SHIFT) /
				(Y_THRESH_3 - Y_THRESH_2);
		levels[0] = interpolate(MID_D_DG, EQUATOR_D_DG, pos);
		levels[1] = interpolate(MID_DG_G, EQUATOR_DG_G, pos);
		levels[2] = interpolate(MID_G_GF, EQUATOR_G_GF, pos);
		levels[3] = interpolate(MID_GF_F, EQUATOR_GF_F, pos);

	} else {
		/* Near equator */
		/* Constant thresholds */
		levels[0] = EQUATOR_D_DG;
		levels[1] = EQUATOR_DG_G;
		levels[2] = EQUATOR_G_GF;
		levels[3] = EQUATOR_GF_F;

	}
#undef Y_THRESH_1
#undef Y_THRESH_2
#undef Y_THRESH_3

#undef EQUATOR_D_DG
#undef EQUATOR_DG_G
#undef EQUATOR_G_GF
#undef EQUATOR_GF_F
#undef MID_D_DG
#undef MID_DG_G
#undef MID_G_GF
#undef MID_GF_F
#undef POLE_D_DG
#undef POLE_DG_G
#undef POLE_G_GF
#undef POLE_GF_F

}

static inline uint32_t texture_earth_like_forest(const struct point_3d p,
		const uint32_t seeds[4], int s)
{
	uint8_t texture, texture2;
	uint32_t res;

	(void)(s);

	texture = noise_get_value_at_pos_standard(p, seeds[2], 2) >> 24;
	texture /= 16;
	texture += 255 / 8 + 255 / 32;
	texture2 = texture / 2;
	res = texture2 / 2 + (texture << 8) + (texture2 << 16);

#ifdef _RED_BLUE_SWAP
	return colours_swap_rb(res);
#else
	return res;
#endif
}

static inline uint32_t texture_earth_like_grass(const struct point_3d p,
		const uint32_t seeds[4], int s)
{
	uint8_t texture, texture2;
	uint32_t res;

	(void)(s);

	texture = noise_get_value_at_pos_standard(p, seeds[2], 2) >> 24;
	texture /= 8;
	texture += 255 / 8 + 255 / 16 + 255 / 32 + 255 / 64;
	texture2 = texture / 2;
	res = texture2 / 2 + (texture << 8) + (texture2 << 16);

#ifdef _RED_BLUE_SWAP
	return colours_swap_rb(res);
#else
	return res;
#endif
}

static inline uint32_t texture_earth_like_desert(const struct point_3d p,
		const uint32_t seeds[4], int s)
{
	uint32_t res, value, texture;

	value = noise_get_value_at_pos_flipflop(p, seeds[3], s - 1);

#define THRESH_1 (((0xffffffff / 16) *  5) / 2 + (0xffffffff / 4))
#define THRESH_2 (((0xffffffff / 16) * 10) / 2 + (0xffffffff / 4))

	if (value < THRESH_1) {
		/* Light / yellow desert */
		texture = noise_get_value_at_pos_standard(p, seeds[0], 3) >>
				(32 - FIX_SHIFT);
		res = colour_interpolate(
				DESERT_LIGHT_1, DESERT_LIGHT_2, texture);

	} else if (value < THRESH_2) {
		/* Transition desert */
		uint32_t light, dark;
		uint64_t wide;
		texture = noise_get_value_at_pos_standard(p, seeds[0], 3) >>
				(32 - FIX_SHIFT);
		light = colour_interpolate(
				DESERT_LIGHT_1, DESERT_LIGHT_2, texture);

		texture = noise_get_value_at_pos_standard(p, seeds[2], 3) >>
				(32 - FIX_SHIFT);
		dark = colour_interpolate(
				DESERT_DARK_1, DESERT_DARK_2, texture);

		wide = ((uint64_t)(value - THRESH_1)) << FIX_SHIFT;
		texture = (uint32_t)(wide / (uint64_t)(THRESH_2 - THRESH_1));

		res = colour_interpolate(
				light, dark, texture);
	} else {
		/* Dark / red desert */
		texture = noise_get_value_at_pos_standard(p, seeds[2], 3) >>
				(32 - FIX_SHIFT);
		res = colour_interpolate(
				DESERT_DARK_1, DESERT_DARK_2, texture);
	}

#undef THRESH_1
#undef THRESH_2

	return res;
}

static inline void texture_earth_like_get_terrain_type(uint32_t value,
		int levels[4], enum earth_like_terrain_type *type,
		uint32_t *pos)
{
	uint64_t wide;
	int level = (value / 2) - 1;

	if (level < levels[0]) {
		/* Desert */
		*type = DESERT;
		*pos = 0; /* only matters for transitions */

	} else if (level < levels[1]) {
		/* Desert-grass */
		*type = DESERT_GRASS;
		wide = ((uint64_t)(level - levels[0])) << FIX_SHIFT;
		*pos = (uint32_t)(wide / (levels[1] - levels[0]));

	} else if (level < levels[2]) {
		/* Grass */
		*type = GRASS;
		*pos = 0; /* only matters for transitions */

	} else if (level < levels[3]) {
		/* Grass-forest */
		*type = GRASS_FOREST;
		wide = ((uint64_t)(level - levels[2])) << FIX_SHIFT;
		*pos = (uint32_t)(wide / (levels[3] - levels[2]));

	} else {
		/* Forest */
		*type = FOREST;
		*pos = 0; /* only matters for transitions */

	}
}

uint32_t texture_earth_like_planet_32bpp(const struct point_3d p,
		const uint32_t seeds[4], int s, uint32_t radius, uint32_t y)
{
	uint32_t res, value, pos;
	int levels[4];
	enum earth_like_terrain_type type;

	/* Get texture noise value for pixel */
	value = noise_get_value_at_pos_flipflop(p, seeds[0], s);

#define SEA_LEVEL 0x87000000

	/* Decide how to colour the pixel */
	if (value > SEA_LEVEL) {
		/* Land */
		/* Get terrain thresholds */
		texture_earth_like_get_thresholds(y, radius, levels);

		/* Get terrain value */
		value = noise_get_value_at_pos_flipflop(p, seeds[1], s);

		/* Get terrain type, and any transition value */
		texture_earth_like_get_terrain_type(value, levels, &type, &pos);

		switch (type) {
		case DESERT:
			res = texture_earth_like_desert(p, seeds, s);
			break;

		case DESERT_GRASS:
			res = colour_interpolate(
					texture_earth_like_desert(p, seeds, s),
					texture_earth_like_grass(p, seeds, s),
					pos);
			break;

		case GRASS:
			res = texture_earth_like_grass(p, seeds, s);
			break;

		case GRASS_FOREST:
			res = colour_interpolate(
					texture_earth_like_grass(p, seeds, s),
					texture_earth_like_forest(p, seeds, s),
					pos);
			break;

		case FOREST:
			res = texture_earth_like_forest(p, seeds, s);
			break;
		}
	} else {
		/* Sea */
		res = SEA_COLOUR;
	}

#undef SEA_LEVEL

	return res;
}


