
#ifndef _PELTAR_COLOURS_H_
#define _PELTAR_COLOURS_H_

/* Comment/uncomment to set red/blue channel swapping */
//#define _RED_BLUE_SWAP

#ifndef _RED_BLUE_SWAP
#define DESERT_LIGHT_1 0xffffd3
#define DESERT_LIGHT_2 0xf0ae7c
#define DESERT_DARK_1  0xdcbb95
#define DESERT_DARK_2  0x9f6a48
#define SEA_COLOUR ((0x9 << 8) + 0x30)
#else
#define DESERT_LIGHT_1 0xd3ffff
#define DESERT_LIGHT_2 0x7caef0
#define DESERT_DARK_1  0x95bbdc
#define DESERT_DARK_2  0x486a9f
#define SEA_COLOUR ((0x9 << 8) + (0x30 << 16))
#endif

static inline uint32_t colours_swap_rb(uint32_t c)
{
	return ((c & 0xff) << 16) | (c & 0xff00) | ((c & 0xff0000) >> 16);
}

/**
 * Interpolate a colour channel.
 *
 * \param[in]  a  First value to interpolate between.
 * \param[in]  b  Second value to interpolate between.
 * \param[in]  f  Interpolation fraction. Fixed point value between 0 and 1.
 * \return Interpolated value,
 */
static inline uint32_t colour_interpolate_channel(
		uint32_t a, uint32_t b, uint32_t f)
{
	uint64_t difference;

	if (a > b) {
		difference = a - b;

		return b + ((difference * (FIX_MULTIPLE - f)) / FIX_MULTIPLE);
	} else {
		difference = b - a;

		return a + ((difference * f) / FIX_MULTIPLE);
	}
}

/**
 * Interpolate between two colours.
 *
 * \param[in]  a  First colour to interpolate between.
 * \param[in]  b  Second colour to interpolate between.
 * \param[in]  f  Interpolation fraction. Fixed point value between 0 and 1.
 * \return Interpolated colour,
 */
static inline uint32_t colour_interpolate(
		uint32_t a, uint32_t b, uint32_t f)
{
	uint32_t res;
	uint8_t r1, g1, b1;
	uint8_t r2, g2, b2;

	r1 = (a & 0xff0000) >> 16;
	r2 = (b & 0xff0000) >> 16;

	g1 = (a & 0xff00) >> 8;
	g2 = (b & 0xff00) >> 8;

	b1 = (a & 0xff);
	b2 = (b & 0xff);

	r1 = (colour_interpolate_channel(r1, r2, f) & 0xff);
	g1 = (colour_interpolate_channel(g1, g2, f) & 0xff);
	b1 = (colour_interpolate_channel(b1, b2, f) & 0xff);

	res = b1 + (g1 << 8) + (r1 << 16);

	return res;
}

#endif

