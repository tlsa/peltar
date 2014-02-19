

#include "noise.h"
#include "../lib/types.h"

#define FIX_SHIFT 14
#define FIX_MULTIPLE (1 << FIX_SHIFT)
#define FIX_MASK (FIX_MULTIPLE - 1)

typedef uint32_t noise_fixed;

static inline peltar_noise noise_random_flipflop(
		uint32_t x, uint32_t y, uint32_t z,
		uint32_t seed)
{
	/* Note the primeness of constants here is crucial. */
	peltar_noise n = (1619 * x) + (31337 * y) + (6971 * z) + (1013 * seed);
	n = (n >> 13) ^ n;

	n = n * (n * n * 60493 + 19990303) + 1376312589;

	/* Flip-flop the result so that adjacent values are always on opposite
	 * sides of the middle possible value. (This ensures signal has fairly
	 * regular zero crossings, and improves the look of my value noise.) */
	if ((x ^ y ^ z) & 1)
		n = n > 0x7fffffff ? n : 0xffffffff - n;
	else
		n = n < 0x7fffffff ? n : 0xffffffff - n;

	return n;
}


/* Linear interpolation
 *
 * \param a  First value to interpolate between
 * \param b  Second value to interpolate between
 * \param f  Fixed point number between 0 and 1.
 */
static inline peltar_noise interpolate(peltar_noise a, peltar_noise b,
		noise_fixed f)
{
	uint64_t difference;

	/* TODO: Is this smoothing worth the extra time? */
//	f = (((f*f) >> FIX_SHIFT) * (3 * FIX_MULTIPLE - (2 * f))) >> FIX_SHIFT;

	if (a > b) {
		difference = a - b;

		return b + ((difference * (FIX_MULTIPLE - f)) / FIX_MULTIPLE);
	} else {
		difference = b - a;

		return a + ((difference * f) / FIX_MULTIPLE);
	}
}


static inline peltar_noise noise_get_noise_at_point(
		uint32_t x, uint32_t y, uint32_t z,
		uint32_t seed)
{
	noise_fixed xf, yf, zf; /* Fractional parts of x, y and z */

	/* Four corners on one face of the integer cube around x, y and z */
	peltar_noise ln, rn, rf, lf; /* [l=left|r=right][n=near|f=far] */

	/* Point on near and far edges of face */
	peltar_noise n, f;

	/* Point on top and bottom faces */
	peltar_noise t, b;

	xf = x & FIX_MASK;
	x >>= FIX_SHIFT;

	yf = y & FIX_MASK;
	y >>= FIX_SHIFT;

	zf = z & FIX_MASK;
	z >>= FIX_SHIFT;

	/* Get noise values for corners of top of cube */
	ln = noise_random(x,     y,     z,     seed);
	rn = noise_random(x + 1, y,     z,     seed);
	lf = noise_random(x,     y,     z + 1, seed);
	rf = noise_random(x + 1, y,     z + 1, seed);

	n = interpolate(ln, rn, xf);
	f = interpolate(lf, rf, xf);

	t = interpolate(n, f, zf);

	/* Get noise values for corners of bottom of cube */
	ln = noise_random(x,     y + 1, z,     seed);
	rn = noise_random(x + 1, y + 1, z,     seed);
	lf = noise_random(x,     y + 1, z + 1, seed);
	rf = noise_random(x + 1, y + 1, z + 1, seed);

	n = interpolate(ln, rn, xf);
	f = interpolate(lf, rf, xf);

	b = interpolate(n, f, zf);

	return interpolate(t, b, yf);
}


static inline peltar_noise noise_get_noise_at_point_flipflop(
		uint32_t x, uint32_t y, uint32_t z,
		uint32_t seed)
{
	noise_fixed xf, yf, zf; /* Fractional parts of x, y and z */

	/* Four corners on one face of the integer cube around x, y and z */
	uint32_t ln, rn, rf, lf; /* [l=left|r=right][n=near|f=far] */

	/* Point on near and far edges of face */
	uint32_t n, f;

	/* Point on top and bottom faces */
	uint32_t t, b;

	xf = x & FIX_MASK;
	x >>= FIX_SHIFT;

	yf = y & FIX_MASK;
	y >>= FIX_SHIFT;

	zf = z & FIX_MASK;
	z >>= FIX_SHIFT;

	/* Get noise values for corners of top of cube */
	ln = noise_random_flipflop(x,     y,     z,     seed);
	rn = noise_random_flipflop(x + 1, y,     z,     seed);
	lf = noise_random_flipflop(x,     y,     z + 1, seed);
	rf = noise_random_flipflop(x + 1, y,     z + 1, seed);

	n = interpolate(ln, rn, xf);
	f = interpolate(lf, rf, xf);

	t = interpolate(n, f, zf);

	/* Get noise values for corners of bottom of cube */
	ln = noise_random_flipflop(x,     y + 1, z,     seed);
	rn = noise_random_flipflop(x + 1, y + 1, z,     seed);
	lf = noise_random_flipflop(x,     y + 1, z + 1, seed);
	rf = noise_random_flipflop(x + 1, y + 1, z + 1, seed);

	n = interpolate(ln, rn, xf);
	f = interpolate(lf, rf, xf);

	b = interpolate(n, f, zf);

	return interpolate(t, b, yf);
}


static inline peltar_noise noise_get_noise_at_point_2d(
		uint32_t x, uint32_t y, uint32_t seed)
{
	noise_fixed xf, yf; /* Fractional parts of x, y and z */

	/* Four corners on one face of the integer cube around x, y and z */
	peltar_noise ln, rn, rf, lf; /* [l=left|r=right][n=near|f=far] */

	/* Point on near and far edges of face */
	peltar_noise n, f;

	xf = x & FIX_MASK;
	x >>= FIX_SHIFT;

	yf = y & FIX_MASK;
	y >>= FIX_SHIFT;

	/* Get noise values for corners of square */
	ln = noise_random(x,     y,     200, seed);
	rn = noise_random(x + 1, y,     200, seed);
	lf = noise_random(x,     y + 1, 200, seed);
	rf = noise_random(x + 1, y + 1, 200, seed);

	n = interpolate(ln, rn, xf);
	f = interpolate(lf, rf, xf);

	return interpolate(n, f, yf);
}


peltar_noise noise_get_value_at_pos_standard(struct point_3d p,
		uint32_t seed, uint32_t levels)
{
	peltar_noise res;
	uint32_t level;

	res = noise_random(p.x >> FIX_SHIFT, p.y >> FIX_SHIFT,
			p.z >> FIX_SHIFT, seed) >> levels;

	for (level = 1; level < levels; level++) {
		res += noise_get_noise_at_point(
				p.x >> level,
				p.y >> level,
				p.z >> level,
				seed) >> (levels - level);
	}

	return res + (res >> levels);
}


peltar_noise noise_get_value_at_pos_flipflop(struct point_3d p,
		uint32_t seed, uint32_t levels)
{
	peltar_noise res;
	uint32_t level;

	res = noise_random_flipflop(p.x >> FIX_SHIFT, p.y >> FIX_SHIFT,
			p.z >> FIX_SHIFT, seed) >> levels;

	for (level = 1; level < levels; level++) {
		res += noise_get_noise_at_point_flipflop(
				p.x >> level,
				p.y >> level,
				p.z >> level,
				seed) >> (levels - level);
	}

	return res + (res >> levels);
}


peltar_noise noise_get_value_at_pos_standard_range(struct point_3d p,
		uint32_t seed, uint32_t levels, uint32_t start)
{
	peltar_noise res = 0;
	uint32_t level;

	for (level = start; level < levels; level++) {
		res += noise_get_noise_at_point(
				p.x >> level,
				p.y >> level,
				p.z >> level,
				seed) >> (levels - level);
	}

	return res + (res >> (levels - start));
}


peltar_noise noise_get_value_at_pos_flipflop_range(struct point_3d p,
		uint32_t seed, uint32_t levels, uint32_t start)
{
	peltar_noise res = 0;
	uint32_t level;

	for (level = start; level < levels; level++) {
		res += noise_get_noise_at_point_flipflop(
				p.x >> level,
				p.y >> level,
				p.z >> level,
				seed) >> (levels - level);
	}

	return res + (res >> (levels - start));
}


peltar_noise noise_get_value_at_pos_standard_range_2d(struct point_3d p,
		uint32_t seed, uint32_t levels, uint32_t start)
{
	peltar_noise res = 0;
	uint32_t level;

	for (level = start; level < levels; level++) {
		res += noise_get_noise_at_point_2d(
				p.x >> level,
				p.y >> level,
				seed) >> (levels - level);
	}

	return res + (res >> (levels - start));
}

