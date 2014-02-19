
#ifndef _PELTAR_NOISE_H_
#define _PELTAR_NOISE_H_

#include <stdint.h>

typedef uint32_t peltar_noise;

struct point_3d;

static inline peltar_noise noise_random(uint32_t x, uint32_t y, uint32_t z,
		uint32_t seed)
{
	/* Note the primeness of constants here is crucial. */
	peltar_noise n = (1619 * x) + (31337 * y) + (6971 * z) + (1013 * seed);
	n = (n >> 13) ^ n;

	return n * (n * n * 60493 + 19990303) + 1376312589;
}

/* 3d versions */
peltar_noise noise_get_value_at_pos_standard(
		struct point_3d p, uint32_t seed, uint32_t levels);
peltar_noise noise_get_value_at_pos_flipflop(
		struct point_3d p, uint32_t seed, uint32_t levels);

peltar_noise noise_get_value_at_pos_standard_range(
		struct point_3d p, uint32_t seed,
		uint32_t levels, uint32_t start);
peltar_noise noise_get_value_at_pos_flipflop_range(
		struct point_3d p, uint32_t seed,
		uint32_t levels, uint32_t start);

/* 2d versions */
peltar_noise noise_get_value_at_pos_standard_range_2d(
		struct point_3d p, uint32_t seed,
		uint32_t levels, uint32_t start);

#endif

