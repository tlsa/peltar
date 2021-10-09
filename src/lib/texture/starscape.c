
#include "../../noise/noise.h"
#include "../fixed-point.h"
#include "../image.h"
#include "../types.h"


static inline uint32_t texture_starscape_star(const struct point_3d p,
		uint32_t seed)
{
	uint32_t ret = 0;
	uint32_t texture;
	uint32_t density;

	texture = noise_random(p.x, p.y, p.z, seed) >> 24;
	density = noise_get_value_at_pos_standard_range_2d(p, seed, 8, 4) >> 24;

	texture += ((density > 128) ? density - 128 : 128 - density) / 2;

	if (texture < 0x01)
		ret += 0x90 + (0xa0 << 8) + (0x90 << 16);

	else if (texture < 0x02)
		ret += 0x80 + (0x90 << 8) + (0x80 << 16);

	else if (texture < 0x04)
		ret += 0x60 + (0x60 << 8) + (0x60 << 16);

	else if (texture < 0x06)
		ret += 0x50 + (0x50 << 8) + (0x50 << 16);

	else if (texture < 0x0b)
		ret += 0x30 + (0x30 << 8) + (0x30 << 16);

	else if (texture < 0x0f)
		ret += 0x20 + (0x20 << 8) + (0x20 << 16);

	else if (texture < 0x28)
		ret += 0x10 + (0x10 << 8) + (0x10 << 16);

	return ret;
}


static inline uint32_t texture_add_colours(uint32_t a, uint32_t b)
{
	if ((a & 0xff) + (b & 0xff) > 0xff)
		a |= 0xff;
	else
		a += (b & 0xff);

	if ((a & 0xff00) + (b & 0xff00) > 0xff00)
		a |= 0xff00;
	else
		a += (b & 0xff00);

	if ((a & 0xff0000) + (b & 0xff0000) > 0xff0000)
		a |= 0xff0000;
	else
		a += (b & 0xff0000);

	return a;
}


static inline uint32_t texture_starscape_add_nebula_component(
		const struct point_3d p, uint32_t seed)
{
	uint32_t t;
	t = noise_get_value_at_pos_standard_range_2d(p, seed, 9, 4);
	t = (t >> 24) / 4;
	return (t > 0x0f) ? t - 0x0f : 0;
}


static inline uint32_t texture_starscape_add_nebula(
		const struct point_3d p, const uint32_t seeds[3])
{
	uint32_t pixel;
	uint32_t t;

	/* Add blue component */
	t = texture_starscape_add_nebula_component(p, seeds[0]);
	pixel = t + ((t >> 2) << 8) + ((t >> 2) << 16);

	/* Add red component */
	t = texture_starscape_add_nebula_component(p, seeds[1]);
	return pixel + ((t >> 2) + ((t >> 2) << 8) + (t << 16));
}


static inline void texture_starscape_get_edge_pixel(uint32_t *restrict pixel,
		const struct point_3d p, const uint32_t seeds[3])
{
	/* Nebula */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_add_nebula(p, seeds));

	/* Stars */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_star(p, seeds[2]));
}


static inline void texture_starscape_get_pixel(uint32_t *restrict pixel,
		const struct point_3d p, const uint32_t seeds[3],
		uint32_t stride)
{
	uint32_t star;

	/* Nebula */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_add_nebula(p, seeds));

	/* Star */
	star = texture_starscape_star(p, seeds[2]);
	if (!star)
		return;

	/* Star on this pixel */
	*pixel = texture_add_colours(*pixel, star);

	/* Smear over adjacent pixels */
	pixel = pixel - stride - 1;
	*pixel = texture_add_colours(*pixel, star / 16);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star / 4);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star / 16);
	pixel = pixel + stride - 2;
	*pixel = texture_add_colours(*pixel, star / 4);
	pixel = pixel + 2;
	*pixel = star / 4;
	pixel = pixel + stride - 2;
	*pixel = star / 16;
	pixel = pixel + 1;
	*pixel = star / 4;
	pixel = pixel + 1;
	*pixel = star / 16;
}


bool texture_get_starscape(struct image *image)
{
	SDL_Surface *render = image_get_surface(image);
	uint32_t stride = render->pitch / peltar_opts.screen_bpp;
	uint32_t *row_start = (uint32_t*)render->pixels;
	int w = image_get_width(image) << FIX_SHIFT;
	int h = image_get_height(image) << FIX_SHIFT;
	uint32_t seeds[3];
	uint32_t *pixel;
	struct point_3d p = {
		.x = 0,
		.y = 0,
		.z = 0
	};

	memset(row_start, 0, render->pitch * image_get_height(image));

	seeds[0] = rand();
	seeds[1] = rand();
	seeds[2] = rand();

	/* Top row */
	pixel = row_start;
	for (p.x = 0; p.x < w; p.x += FIX_MULTIPLE) {
		texture_starscape_get_edge_pixel(pixel++, p, seeds);
	}
	row_start += stride;

	/* Up to last row */
	for (p.y = FIX_MULTIPLE; p.y < h - FIX_MULTIPLE; p.y += FIX_MULTIPLE) {
		pixel = row_start;
		texture_starscape_get_edge_pixel(pixel++, p, seeds);
		for (p.x = FIX_MULTIPLE; p.x < w - FIX_MULTIPLE;
				p.x += FIX_MULTIPLE) {
			texture_starscape_get_pixel(pixel++, p, seeds, stride);
		}
		texture_starscape_get_edge_pixel(pixel++, p, seeds);
		row_start += stride;
	}

	/* Last row */
	pixel = row_start;
	for (p.x = 0; p.x < w; p.x += FIX_MULTIPLE) {
		texture_starscape_get_edge_pixel(pixel++, p, seeds);
	}

	return true;
}

