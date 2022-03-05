
#include <stdbool.h>

#include <SDL/SDL.h>

#include "../../noise/noise.h"
#include "../fixed-point.h"
#include "../colours.h"
#include "../image.h"
#include "../types.h"

static inline struct colour texture_starscape_star(const struct point_3d p,
		uint32_t seed)
{
	struct colour ret = { 0 };
	uint32_t texture;
	uint32_t density;

	texture = noise_random(p.x, p.y, p.z, seed) >> 24;
	density = noise_get_value_at_pos_standard_range_2d(p, seed, 8, 4) >> 24;

	texture += ((density > 128) ? density - 128 : 128 - density) / 2;

	if (texture < 0x01) {
		ret.r = 0x90;
		ret.g = 0xa0;
		ret.b = 0x90;

	} else if (texture < 0x02) {
		ret.r = 0x80;
		ret.g = 0x90;
		ret.b = 0x80;

	} else if (texture < 0x04) {
		ret.r = 0x60;
		ret.g = 0x60;
		ret.b = 0x60;

	} else if (texture < 0x06) {
		ret.r = 0x50;
		ret.g = 0x50;
		ret.b = 0x50;

	} else if (texture < 0x0b) {
		ret.r = 0x30;
		ret.g = 0x30;
		ret.b = 0x30;

	} else if (texture < 0x0f) {
		ret.r = 0x20;
		ret.g = 0x20;
		ret.b = 0x20;

	} else if (texture < 0x28) {
		ret.r = 0x10;
		ret.g = 0x10;
		ret.b = 0x10;
	}

	return ret;
}

static inline struct colour texture_add_colours(
		struct colour a,
		struct colour b)
{
	a.r = (a.r + b.r < 0xff) ? a.r + b.r : 0xff;
	a.g = (a.g + b.g < 0xff) ? a.g + b.g : 0xff;
	a.b = (a.b + b.b < 0xff) ? a.b + b.b : 0xff;

	return a;
}

static inline uint32_t texture_starscape_get_nebula_component(
		const struct point_3d p, uint32_t seed)
{
	uint32_t t;
	t = noise_get_value_at_pos_standard_range_2d(p, seed, 9, 4);
	t = (t >> 24) / 4;
	return (t > 0x0f) ? t - 0x0f : 0;
}

static inline struct colour texture_starscape_add_nebula(
		const struct point_3d p, const uint32_t seeds[3])
{
	struct colour pixel = { 0 };
	uint32_t blue = texture_starscape_get_nebula_component(p, seeds[0]);
	uint32_t red  = texture_starscape_get_nebula_component(p, seeds[1]);

	pixel.r = (blue >> 2) + (red     );
	pixel.g = (blue >> 2) + (red >> 2);
	pixel.b = (blue     ) + (red >> 3);

	return pixel;
}

static inline void texture_starscape_get_edge_pixel(
		struct colour *restrict pixel,
		const struct point_3d p,
		const uint32_t seeds[3])
{
	/* Nebula */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_add_nebula(p, seeds));

	/* Stars */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_star(p, seeds[2]));
}

static inline void texture_starscape_get_pixel(struct colour *restrict pixel,
		const struct point_3d p, const uint32_t seeds[3],
		uint32_t stride)
{
	struct colour star;
	struct colour star4;
	struct colour star16;

	/* Nebula */
	*pixel = texture_add_colours(*pixel,
			texture_starscape_add_nebula(p, seeds));

	/* Star */
	star = texture_starscape_star(p, seeds[2]);
	if (star.r == 0 && star.g == 0 && star.b == 0)
		return;

	star4.r = star.r / 4;
	star4.g = star.g / 4;
	star4.b = star.b / 4;

	star16.r = star.r / 16;
	star16.g = star.g / 16;
	star16.b = star.b / 16;

	/* Star on this pixel */
	*pixel = texture_add_colours(*pixel, star);

	/* Smear over adjacent pixels */
	pixel = pixel - stride - 1;
	*pixel = texture_add_colours(*pixel, star16);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star4);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star16);

	pixel = pixel + stride - 2;
	*pixel = texture_add_colours(*pixel, star4);
	pixel = pixel + 2;
	*pixel = texture_add_colours(*pixel, star4);

	pixel = pixel + stride - 2;
	*pixel = texture_add_colours(*pixel, star16);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star4);
	pixel = pixel + 1;
	*pixel = texture_add_colours(*pixel, star16);
}

static void starscape_colour_to_suface_format(struct image *image)
{
	SDL_Surface *render = image_get_surface(image);
	uint32_t stride = render->pitch / peltar_opts.screen_bpp;
	size_t height = image_get_height(image);
	size_t width = image_get_width(image);
	struct colour *colour = render->pixels;
	uint32_t *pixel = render->pixels;

	for (size_t y = 0; y < height; y++) {
		colour_texture_to_screen(render, colour, width, pixel);
		colour += stride;
		pixel += stride;
	}

}

bool texture_get_starscape(struct image *image)
{
	SDL_Surface *render = image_get_surface(image);
	uint32_t stride = render->pitch / peltar_opts.screen_bpp;
	struct colour *row_start = (struct colour *)render->pixels;
	unsigned int w = image_get_width(image) << FIX_SHIFT;
	unsigned int h = image_get_height(image) << FIX_SHIFT;
	uint32_t seeds[3];
	struct colour *pixel;
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

	starscape_colour_to_suface_format(image);

	return true;
}

