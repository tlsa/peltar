
#ifndef _PELTAR_COLOURS_H_
#define _PELTAR_COLOURS_H_

#include "fixed-point.h"

struct colour {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
};

#define COLOUR_TO_U32(_c) (*(uint32_t *)(&_c))

#define DESERT_LIGHT_1 (struct colour) { .r = 0xff, .g = 0xff, .b = 0xd3, }
#define DESERT_LIGHT_2 (struct colour) { .r = 0xf0, .g = 0xae, .b = 0x7c, }
#define DESERT_DARK_1  (struct colour) { .r = 0xdc, .g = 0xbb, .b = 0x95, }
#define DESERT_DARK_2  (struct colour) { .r = 0x9f, .g = 0x6a, .b = 0x48, }
#define SEA_COLOUR     (struct colour) {            .g = 0x09, .b = 0x30, }

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
static inline struct colour colour_interpolate(
		const struct colour a,
		const struct colour b,
		uint32_t f)
{
	struct colour res = {
		.r = colour_interpolate_channel(a.r, b.r, f),
		.g = colour_interpolate_channel(a.g, b.g, f),
		.b = colour_interpolate_channel(a.b, b.b, f),
	};

	return res;
}

static inline bool colour_different(
		const struct colour *a,
		const struct colour *b)
{
	return *((uint32_t*) a) != *((uint32_t*) b);
}

static inline void colour_texture_to_screen(
		const SDL_Surface *screen,
		const struct colour *in,
		uint32_t count,
		uint32_t *out)
{
	uint8_t r_shift = screen->format->Rshift;
	uint8_t g_shift = screen->format->Gshift;
	uint8_t b_shift = screen->format->Bshift;

	for (size_t i = 0; i < count; i++) {
		out[i] =
			(in[i].r << r_shift) |
			(in[i].g << g_shift) |
			(in[i].b << b_shift);
	}
}

#endif
