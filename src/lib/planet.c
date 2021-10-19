
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "fixed-point.h"
#include "colours.h"
#include "planet.h"
#include "cellular-texture.h"
#include "texture/player.h"
#include "texture/earth-like.h"
#include "types.h"


#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#define PLANET_DENSITY_NUM 2
#define PLANET_DENSITY_DEN 16

#define LUT_MAX_SETTING 10
#define LUT_MAX (1 << LUT_MAX_SETTING)

#define CONV_FIX_TO_LUT (FIX_SHIFT - LUT_MAX_SETTING)

#define ROT_STEP 2048


struct planet_internals {
	int size; /* Planet diameter */
	int *line_lengths; /* Cache of circle quadrant line lengths */
	int *angles; /* Cache of circle quadrant pixel texturing angles */
	Uint8 *lighting; /* Cache of circle half lighting fractions */

	int texture_w; /* Width of texture */
	int texture_h; /* Height of texture */
	uint32_t *texture; /* Data matches planet render surface colour format */
	int texture_w2; /* Half width of texture */
};


struct planet {
	struct planet_internals big;
	struct planet_internals small;

	int size; /* Planet diameter */
	int rotation; /* Fixed point.  2 * FIX_MULTIPLE is a full circle */

	void (*update_render)(struct planet_internals *p, SDL_Surface *screen,
			int screen_x, int screen_y, int rotation);
};




static int arc_cosine_table[LUT_MAX];

/*
 * Initialise the arc_cosine lookup table.
 *
 * Only holds values from acos(0) to acos(1)
 */
static void arc_cosine_lut_init()
{
	double arg, res;
	int i;

	for (i = 0; i < LUT_MAX; i++) {
		arg = i / (double)(LUT_MAX);
		res = acos(arg);

		/* Factor out pi, and store as fixed point */
		arc_cosine_table[i] = (res / M_PI) * FIX_MULTIPLE;
	}
}


/*
 * Look up the arc cosine of value, x (between 0 and 1 are valid)
 *
 * \params x  Value to get arc cosine of
 * \return arc cosine.
 *
 * Note returned angle is scaled to range 0-0.5 and multiplied by FIX_MULTIPLE.
 * (Only handles first 1/4 of cosine period.)
 */
static inline int arc_cosine(int x)
{
	return arc_cosine_table[x];
}


void planet_init(void)
{
	arc_cosine_lut_init();
}


static bool planet_create_details(struct planet_internals *p, int size)
{
	int x, y, y2;
	int line_start, line_length;
	int px_count;
	int i;
	int adjacent, angle;

	/* Texture dimensions */
	p->texture_h = size;
	p->texture_w = (size * M_PI) + 0.5;
	p->texture_w2 = p->texture_w / 2;

	/* Allocate memory for texture */
	p->texture = malloc(sizeof(uint32_t) * p->texture_h * p->texture_w);
	if (p->texture == NULL) {
		return false;
	}

	/* Allocate memory for line_lengths cache */
	p->line_lengths = malloc(sizeof(int) * size / 2);
	if (p->line_lengths == NULL) {
		return false;
	}

	/* Loop through top left quarter of circle and find length of row of
	 * pixels in circle.  Also count number of pixels inside the quarter
	 * circle. */
	px_count = 0;
	for (y = 0; y < size / 2; y++) {
		y2 = (y - size / 2) * (y - size / 2);

		p->line_lengths[y] = 0;
		/* Find length and start of row of pixels that are inside
		 * the top left quarter of the circle. */
		for (x = 0; x < size / 2; x++) {
			if ((x - size / 2) * (x - size / 2) + y2 <=
					(size / 2) * (size / 2)) {
				/* Pixel is in circle */
				p->line_lengths[y] += 1;
			}
		}
		px_count += p->line_lengths[y];
	}

	/* Allocate memory for angle cache */
	p->angles = malloc(sizeof(int) * px_count);
	if (p->angles == NULL) {
		return false;
	}

	/* Populate the angle cache */
	i = 0;
	for (y = 0; y < size / 2; y++) {

		/* Look up the number of pixels that are within the quarter
		 * circle on this row. */
		line_length = p->line_lengths[y];

		if (line_length == 0)
			/* Nothing to render */
			continue;

		/* Get offset to start of pixels within cricle on this row */
		line_start = size / 2 - line_length;

		/* Cache angles for row of points in quarter circle */
		for (x = line_start; x < size / 2; x++) {

			/* Get "adjacent" side length, for left side */
			adjacent = line_length + line_start - x;
			/* Get angle that the two pixels at this 'x'
			 * are at, for texturing.  acos(adjacent / hypotenuse),
			 * argument is scaled for acos() LUT size */
			/* Subtracting 0.5 from adjacent to bring centre columns
			 * from each half away from each other.
			 * (Avoid dupication seam down planet centre.) */
			angle = arc_cosine((((adjacent << FIX_SHIFT) -
					(FIX_MULTIPLE / 2)) / line_length) >>
					CONV_FIX_TO_LUT);

			/* Cache the angle */
			p->angles[i] = angle;
			i++;
		}
	}

	/* Allocate memory for lighting cache */
	p->lighting = malloc(sizeof(int) * 2 * px_count);
	if (p->lighting == NULL) {
		return false;
	}

	int r = size / 2;
	/* Populate the angle cache */
	i = 0;
	for (y = 0; y < size / 2; y++) {
		/* Look up the number of pixels that are within the quarter
		 * circle on this row. */
		line_length = p->line_lengths[y];

		if (line_length == 0)
			/* Nothing to render */
			continue;

		/* Get offset to start of pixels within cricle on this row */
		line_start = r - line_length;

		/* Cache lighting values for half circle */
		for (x = line_start; x < r; x++) {
			double lighting;
			double z = sqrt(fabs((r + 0.5) * (r + 0.5) -
					(r - x) * (r - x) -
					(r - y) * (r - y)));

			/* L: Lighting vector, N: Surface normal vector
			 *
			 * L & N are both unit vectors (magnitude == 1)
			 *
			 * cos(theta) = L . N     (dot product)
			 *            = (xN * xL) + (yN * yL) + (zN + zL)
			 *
			 * (Theta's the angle between L and N)
			 *
			 * Want lighting to come from left / front, so Ly
			 * component is 0, so y is irrelevent.
			 *
			 * For easy maths (unit vector), using 8-15-17 triangle
			 * for lighting:
			 *
			 *   Lx = -8 / 17,
			 *   Lz = 15 / 17
			 */
			lighting = ((x - r) / (r + 0.5)) * (-8.0 / 17.0) +
					(z  / (r + 0.5)) * (15.0 / 17.0);

			if (lighting < 0)
				p->lighting[i] = 0;
			else if (lighting <= 1)
				p->lighting[i] = 255 * lighting;
			else
				printf("lighting: %f\t z: %f\n", lighting, z);

			i++;

			/* Caching lighting for top half of circle.
			 * Do same as above for top right quarter. */

			z = sqrt(fabs((r + 0.5) * (r + 0.5) -
					(r - x) * (r - x) -
					(r - y) * (r - y)));

			lighting = ((r - x) / (r + 0.5)) * (-8.0 / 17.0) +
					(z  / (r + 0.5)) * (15.0 / 17.0);

			if (lighting < 0)
				p->lighting[i] = 0;
			else if (lighting <= 1)
				p->lighting[i] = 255 * lighting;
			else
				printf("lighting: %f\t z: %f\n", lighting, z);
			i++;
		}
	}

	/* Initialise values */
	p->size = size;

	return true;
}


bool planet_create(struct planet **p, int size)
{
	*p = malloc(sizeof(struct planet));
	if (*p == NULL)
		return false;

	size &= ~0x7;

	(*p)->big.texture = NULL;
	(*p)->big.lighting = NULL;
	(*p)->big.line_lengths = NULL;
	(*p)->big.angles = NULL;

	(*p)->small.texture = NULL;
	(*p)->small.lighting = NULL;
	(*p)->small.line_lengths = NULL;
	(*p)->small.angles = NULL;

	(*p)->rotation = 1 << FIX_SHIFT;
	(*p)->size = size;

	if (!planet_create_details(&((*p)->big), size)) {
		planet_free(*p);
		return false;
	}

	if (!planet_create_details(&((*p)->small), size / 4)) {
		planet_free(*p);
		return false;
	}

	planet_set_lighting(*p, false);

	return true;
}


static void planet_free_internals(struct planet_internals *p)
{
	if (p->texture != NULL)
		free(p->texture);

	if (p->line_lengths != NULL)
		free(p->line_lengths);

	if (p->angles != NULL)
		free(p->angles);

	if (p->lighting != NULL)
		free(p->lighting);
}


void planet_free(struct planet *p)
{
	assert(p != NULL);

	planet_free_internals(&p->big);
	planet_free_internals(&p->small);

	free(p);
}


static inline void planet_set_pixel_flat(uint32_t *restrict pixel,
		const uint32_t *restrict texture)
{
	/* Set it to colour of appropriate pixel in texture */
	*pixel = *texture;
}


static void planet_update_render_flat(struct planet_internals *p,
		SDL_Surface *screen, int screen_x, int screen_y, int rotation)
{
	const int radius = p->size / 2;
	const int diameter = p->size - 1;
	int x, y;
	int line_length;
	int offset;
	uint32_t *restrict row_offset_t = (uint32_t*)screen->pixels +
			screen_y * screen->pitch / peltar_opts.screen_bpp +
			screen_x;
	uint32_t *restrict row_offset_b = row_offset_t +
			diameter * screen->pitch / peltar_opts.screen_bpp;
	const uint32_t *restrict texture_row_offset_t = p->texture;
	const uint32_t *restrict texture_row_offset_b = p->texture +
			(p->texture_h - 1) * p->texture_w;
	int angle, rot;
	const int *restrict angle_cache = p->angles;

	/* Loop through top left quarter of circle, and render symmetrically
	 * reflected points on each itteration. */
	for (y = 0; y < radius; y++) {

		/* Look up the number of pixels that are within the quarter
		 * circle on this row. */
		line_length = p->line_lengths[y];

		if (line_length == 0)
			/* Nothing to render */
			continue;

		/* Set offsets to SDL surface pixel data for row */
		row_offset_t += screen->pitch / peltar_opts.screen_bpp;
		row_offset_b -= screen->pitch / peltar_opts.screen_bpp;

		/* Set offsets to texture pixel data for row */
		texture_row_offset_t += p->texture_w;
		texture_row_offset_b -= p->texture_w;

		/* Render a row of points in each quarter of the circle */
		for (x = radius - line_length; x < radius; x++) {
			int right;

			/* Get cached angle subtended by adjacent/hypotenuse, or
			 * [position along line]/[line length]. */
			angle = *angle_cache++;

			/* Apply planet's current rotation (between 0 and 2) to
			 * angle (which is between 0 and 0.5), and wrap back to
			 * range 0 to 2 */
			rot = angle - rotation;
			if (rot < 0)
				rot += (2 << FIX_SHIFT);

			/* Get offset into texture, for current angle. */
			offset = (p->texture_w2 * rot) >> FIX_SHIFT;

			/* Set pixel colour from texture, for top and bottom
			 * rows */
			planet_set_pixel_flat(row_offset_t + x,
					texture_row_offset_t + offset);
			planet_set_pixel_flat(row_offset_b + x,
					texture_row_offset_b + offset);

			/* Do same to render the two pixels on the right side */
			/* Angle from other half can be reused as (1 - angle),
			 * explioting cosine symmetry.  (To map from first
			 * quadrant to second quadrant.) */
			angle = FIX_MULTIPLE - angle - rotation;
			if (angle < 0)
				angle += (2 << FIX_SHIFT);

			offset = (p->texture_w2 * angle) >> FIX_SHIFT;

			/* Get offset to pixels on right hand side of circle */
			right = diameter - x;

			planet_set_pixel_flat(row_offset_t + right,
					texture_row_offset_t + offset);
			planet_set_pixel_flat(row_offset_b + right,
					texture_row_offset_b + offset);
		}
	}
}


void planet_update_render(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y)
{
	p->update_render(&p->big, screen, screen_x, screen_y, p->rotation);

	/* Update rotation for next call */
	p->rotation += (1 << FIX_SHIFT) / ROT_STEP;
	if (p->rotation > (2 << FIX_SHIFT))
		p->rotation -= (2 << FIX_SHIFT);
}


void planet_update_render_scaled(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y)
{
	p->update_render(&p->small, screen, screen_x, screen_y, p->rotation);

	/* Update rotation for next call */
	p->rotation += (1 << FIX_SHIFT) / ROT_STEP;
	if (p->rotation > (2 << FIX_SHIFT))
		p->rotation -= (2 << FIX_SHIFT);
}


static inline void planet_set_pixel_lighting(uint32_t *restrict pixel,
		const uint32_t *restrict texture, const Uint8 *lighting)
{
	/* Set it to colour of appropriate pixel in texture, with shading */
	*pixel = (((*lighting * (0x00ff00ff & *texture)) >> 8) & 0x00ff00ff) |
		 (((*lighting * (0x0000ff00 & *texture)) >> 8) & 0x0000ff00);
}


static void planet_update_render_lighting(struct planet_internals *p,
		SDL_Surface *screen, int screen_x, int screen_y, int rotation)
{
	const int radius = p->size / 2;
	const int diameter = p->size - 1;
	int x, y;
	int line_length;
	int offset;
	uint32_t *restrict row_offset_t = (uint32_t*)screen->pixels +
			screen_y * screen->pitch / peltar_opts.screen_bpp +
			screen_x;
	uint32_t *restrict row_offset_b = row_offset_t +
			diameter * screen->pitch / peltar_opts.screen_bpp;
	const uint32_t *restrict texture_row_offset_t = p->texture;
	const uint32_t *restrict texture_row_offset_b = p->texture +
			(p->texture_h - 1) * p->texture_w;
	int angle, rot;
	const int *restrict angle_cache = p->angles;
	const Uint8 *restrict l = p->lighting; /* lighting cache index */

	/* Loop through top left quarter of circle, and render symmetrically
	 * reflected points on each itteration. */
	for (y = 0; y < radius; y++) {

		/* Look up the number of pixels that are within the quarter
		 * circle on this row. */
		line_length = p->line_lengths[y];

		if (line_length == 0)
			/* Nothing to render */
			continue;

		/* Set offsets to SDL surface pixel data for row */
		row_offset_t += screen->pitch / peltar_opts.screen_bpp;
		row_offset_b -= screen->pitch / peltar_opts.screen_bpp;

		/* Set offsets to texture pixel data for row */
		texture_row_offset_t += p->texture_w;
		texture_row_offset_b -= p->texture_w;

		/* Render a row of points in each quarter of the circle */
		for (x = radius - line_length; x < radius; x++) {
			int right;

			/* Get cached angle subtended by adjacent/hypotenuse, or
			 * [position along line]/[line length]. */
			angle = *angle_cache++;

			/* Apply planet's current rotation (between 0 and 2) to
			 * angle (which is between 0 and 0.5), and wrap back to
			 * range 0 to 2 */
			rot = angle - rotation;
			if (rot < 0)
				rot += (2 << FIX_SHIFT);

			/* Get offset into texture, for current angle. */
			offset = (p->texture_w2 * rot) >> FIX_SHIFT;

			/* Set pixel colour from texture, for top and bottom
			 * rows */
			planet_set_pixel_lighting(row_offset_t + x,
					texture_row_offset_t + offset,
					l);
			planet_set_pixel_lighting(row_offset_b + x,
					texture_row_offset_b + offset,
					l++);

			/* Do same to render the two pixels on the right side */
			/* Angle from other half can be reused as (1 - angle),
			 * explioting cosine symmetry.  (To map from first
			 * quadrant to second quadrant.) */
			angle = FIX_MULTIPLE - angle - rotation;
			if (angle < 0)
				angle += (2 << FIX_SHIFT);

			offset = (p->texture_w2 * angle) >> FIX_SHIFT;

			/* Get offset to pixels on right hand side of circle */
			right = diameter - x;

			planet_set_pixel_lighting(row_offset_t + right,
					texture_row_offset_t + offset,
					l);
			planet_set_pixel_lighting(row_offset_b + right,
					texture_row_offset_b + offset,
					l++);
		}
	}
}


/* Plots the big texture */
void planet_plot_texture_internal(struct planet_internals *p, SDL_Surface *screen,
		int screen_x, int screen_y)
{
	uint32_t *row_start = (uint32_t*)screen->pixels;
	uint32_t *pixel;
	int x, y;
	int i = 0;

	row_start += screen_y * screen->pitch / peltar_opts.screen_bpp +
			screen_x;

	for (y = 0; y < p->texture_h; y++) {
		pixel = row_start;
		for (x = 0; x < p->texture_w; x++) {
			*pixel++ = p->texture[i++];
		}
		row_start += screen->pitch / peltar_opts.screen_bpp;
	}
}


void planet_plot_texture(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y)
{
	planet_plot_texture_internal(&p->big, screen, screen_x, screen_y);
}


void planet_plot_texture_scaled(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y)
{
	planet_plot_texture_internal(&p->small, screen, screen_x, screen_y);
}


/* Simple smooth scaling blends the values from a 4x4 grid of pixels into one
 * value.  Call once per channel with appropriate mask.  marker indicates top
 * left pixel in grid, width is row span of large image. */
static uint32_t planet_make_small_texture_px(const uint32_t *marker,
		uint32_t mask, int width)
{
	uint32_t ret = 0;
	width -= 4;

	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	marker += width;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	marker += width;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	marker += width;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;
	ret += (*marker++) & mask;

	ret /= 16;
	return ret & mask;
}


static void planet_make_small_texture(struct planet *p)
{
	const uint32_t *restrict big = p->big.texture;
	uint32_t *restrict small = p->small.texture;
	int x, y;
	int i, j;

	i = j = 0;
	for (y = 0; y < p->small.texture_h - 1; y++) {
		for (x = 0; x < p->small.texture_w - 1; x++) {
			small[i] = planet_make_small_texture_px(
					&big[j], 0xff00ff,
					p->big.texture_w);
			small[i] |= planet_make_small_texture_px(
					&big[j], 0x00ff00,
					p->big.texture_w);
			i++;
			j += 4;
		}
		small[i++] = big[j];
		j = (y + 1) * p->big.texture_w * 4;
	}

	for (x = 0; x < p->small.texture_w; x++) {
		small[i++] = big[j];
		j += 4;
	}
}


bool planet_get_texture_from_file(struct planet *planet, const char *filename,
		SDL_Surface *screen)
{
	int x, y, i;
	int scaled_x, scaled_y;
	SDL_Surface *sdl_texture;
	uint32_t *pixmem32;
	Uint8 *pixmem8;
	Uint8 r, g, b;
	struct planet_internals *p = &planet->big;

	sdl_texture = IMG_Load(filename);
	if (sdl_texture == NULL) {
		printf("Couldn't load %s\n", filename);
		return false;
	}

	i = 0;
	for (y = 0; y < p->texture_h; y++) {
		scaled_y = (y * sdl_texture->h) / p->texture_h;
		for (x = 0; x < p->texture_w; x++) {
			scaled_x = (x * sdl_texture->w) / p->texture_w;
			pixmem8 = (Uint8*)sdl_texture->pixels +
					scaled_y * sdl_texture->pitch +
					scaled_x *
					sdl_texture->format->BytesPerPixel;
			pixmem32 = (uint32_t*)pixmem8;

			SDL_GetRGB(*pixmem32, sdl_texture->format, &r, &g, &b);

			p->texture[i++] = SDL_MapRGB(screen->format, r, g, b);
		}
	}

	SDL_FreeSurface(sdl_texture);

	planet_make_small_texture(planet);

	return true;
}

static inline int sin_lut(int angle, const int *table, int limit)
{
	if (angle >= 2 * limit)
		angle -= 2 * limit;

	if (angle < limit)
		return table[angle];

	return -table[angle - limit];
}

static inline int cos_lut(int angle, const int *table, int limit)
{
	return sin_lut(angle + limit / 2, table, limit);
}

static inline struct point_3d planet_point_from_texture_coord(
		int x, int y, int r, int h, const int *sine, int limit)
{
	struct point_3d ret;

	r <<= FIX_SHIFT;

	ret.x = r + sin_lut(x, sine, limit) * h;
	ret.y = y << FIX_SHIFT;
	ret.z = r + cos_lut(x, sine, limit) * h;

	return ret;
}

static inline uint32_t interpolate_colour(uint32_t a, uint32_t b, uint32_t f)
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

static inline uint32_t interpolate_colour_32bpp(
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

	r1 = (interpolate_colour(r1, r2, f) & 0xff);
	g1 = (interpolate_colour(g1, g2, f) & 0xff);
	b1 = (interpolate_colour(b1, b2, f) & 0xff);

	res = b1 + (g1 << 8) + (r1 << 16);

	return res;
}


bool planet_generate_texture(struct planet *planet)
{
	struct planet_internals *p = &planet->big;
	int x, y, i;
	uint32_t seeds[4];
	int r = p->texture_h / 2;
	int h, s;
	int half_w = p->texture_w / 2;
	double scaled_pi = M_PI / (double)(half_w);
	int *sine;
	struct point_3d pt;
	uint32_t prev, next, sea_colour;

	/* Allocate sine LUT */
	sine = malloc((half_w) * (sizeof(int)));
	if (sine == NULL)
		return false;

	/* Fill out sine LUT */
	i = 0;
	for (i = 0; i < half_w; i++) {
		sine[i] = sin(i * scaled_pi) * (double)FIX_MULTIPLE;
	}

	/* Random seeds for texture */
	seeds[0] = rand();
	seeds[1] = rand();
	seeds[2] = rand();
	seeds[3] = rand();

	/* Set feature scaling, for texture generator.  Based on radius */
	s = 1;
	while ((r >>= 1) > 0)
		s++;

	/* reset radius */
	r = p->texture_h / 2;

	/* Create texture */
	i = 0;
	for (y = 0; y < p->texture_h; y++) {
		h = sqrt(r * r - ((r - y) * (r - y)));
		for (x = 0; x < p->texture_w; x++) {
			/* Get 3D location of this texture coordinate */
			pt = planet_point_from_texture_coord(x, y, r, h,
					sine, half_w);
			p->texture[i++] = texture_earth_like_planet_32bpp(
					pt, seeds, s, r, y);
		}
	}

	sea_colour = SEA_COLOUR;

	/* Simple smoothing of the texture around coastlines */
	i = 0;
	for (y = 0; y < p->texture_h; y++) {
		for (x = 0; x < p->texture_w; x++) {
			if (p->texture[i] != sea_colour) {
				int m = 0;
				if (x == 0) {
					prev = p->texture[i + p->texture_w - 1];
					next = p->texture[i + 1];
				} else if (x == p->texture_w - 1) {
					prev = p->texture[i - 1];
					next = p->texture[i - p->texture_w + 1];
				} else {
					prev = p->texture[i - 1];
					next = p->texture[i + 1];
				}

				if (y > 0 && y < p->texture_h - 1) {
					if (p->texture[i - p->texture_w] ==
							sea_colour)
						m += 6;
					if (p->texture[i + p->texture_w] ==
							sea_colour)
						m += 6;
				}

				if (next == sea_colour && prev == sea_colour) {
					/* Sea on both sides of pixel */
					m += 14;
					p->texture[i] =
						interpolate_colour_32bpp(
							p->texture[i],
							sea_colour,
							m * FIX_MULTIPLE / 32);
				} else if (next == sea_colour ||
						prev == sea_colour) {
					/* Sea on one side of pixel */
					m += 8;
					p->texture[i] =
						interpolate_colour_32bpp(
							p->texture[i],
							sea_colour,
							m * FIX_MULTIPLE / 32);
				}
			}
			i++;
		}
	}

	free(sine);

	planet_make_small_texture(planet);

	return true;
}


bool planet_generate_texture_man_made(struct planet *planet, uint32_t colour)
{
	struct planet_internals *p = &planet->big;
	int x, y, i;
	uint32_t seeds[4];
	int r = p->texture_h / 2;
	int h, s;
	int half_w = p->texture_w / 2;
	double scaled_pi = M_PI / (double)(half_w);
	int *sine;
	struct point_3d pt;
	struct cellular_texture *cells;
	peltar_fixed dist, max_dist;

	/* Allocate sine LUT */
	sine = malloc((half_w) * (sizeof(int)));
	if (sine == NULL)
		return false;

	/* Fill out sine LUT */
	i = 0;
	for (i = 0; i < half_w; i++) {
		sine[i] = sin(i * scaled_pi) * (double)FIX_MULTIPLE;
	}

	/* Random seeds for texture */
	seeds[0] = rand();
	seeds[1] = rand();
	seeds[2] = rand();
	seeds[3] = rand();


	if (!cellular_texture_create(&cells, p->texture_h)) {
		free(sine);
		return false;
	}

	/* Set feature scaling, for texture generator.  Based on radius */
	s = 0;
	while ((r >>= 1) > 0)
		s++;
	if (s < 1)
		s = 1;

	/* reset radius */
	r = p->texture_h / 2;

	/* Get distances */
	i = 0;
	max_dist = 0;
	for (y = 0; y < p->texture_h; y++) {
		h = sqrt(r * r - ((r - y) * (r - y)));
		for (x = 0; x < p->texture_w; x++) {
			/* Get 3D location of this texture coordinate */
			pt = planet_point_from_texture_coord(x, y, r, h,
					sine, half_w);
			dist = cellular_texture_get_dist(cells, pt);
			p->texture[i++] = dist;
			if (dist > max_dist)
				max_dist = dist;
		}
	}

	/* Create texture */
	i = 0;
	for (y = 0; y < p->texture_h; y++) {
		h = sqrt(r * r - ((r - y) * (r - y)));
		for (x = 0; x < p->texture_w; x++) {
			dist = p->texture[i];
			pt = planet_point_from_texture_coord(x, y, r, h,
					sine, half_w);
			p->texture[i++] = texture_man_made_32bpp(pt, dist,
					max_dist, seeds, s, colour);
		}
	}

	cellular_texture_free(cells);
	free(sine);

	planet_make_small_texture(planet);

	return true;
}


int planet_get_size(const struct planet *p)
{
	return p->big.size;
}


int planet_get_size_scaled(const struct planet *p)
{
	return p->small.size;
}

int planet_get_mass(const struct planet *p)
{
	int radius = p->big.size / 2;
	int volume = (4 * M_PI * radius * radius * radius) / 3;
	int mass = volume * PLANET_DENSITY_NUM / PLANET_DENSITY_DEN;

	return mass;
}

void planet_set_lighting(struct planet *p, bool lighting)
{
	if (lighting)
		p->update_render = &planet_update_render_lighting;
	else
		p->update_render = &planet_update_render_flat;
}
