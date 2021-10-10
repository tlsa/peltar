
#ifndef _PELTAR_DRAW_H_
#define _PELTAR_DRAW_H_

#include <stdbool.h>
#include <SDL.h>

#include "types.h"


/*                                         ## = 1 pixel
 *         ##  ##
 *
 *         ##  ##
 */
static inline void draw_target_3x3(SDL_Surface *screen, int x, int y,
		uint32_t colour)
{
	int y_step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + (y - 1) * y_step + x - 1;

	*pixel = colour;
	pixel += 2;
	*pixel = colour;

	pixel += 2 * y_step;

	*pixel = colour;
	pixel -= 2;
	*pixel = colour;
}


/*                                         ## = 1 pixel
 *         ######
 *         ######
 *         ######
 */
static inline void draw_shot_3x3(SDL_Surface *screen, int x, int y,
		uint32_t colour)
{
	int y_step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + (y - 1) * y_step + x - 1;

	*pixel++ = colour;
	*pixel++ = colour;
	*pixel = colour;

	pixel += y_step - 2;

	*pixel++ = colour;
	*pixel++ = colour;
	*pixel = colour;

	pixel += y_step - 2;

	*pixel++ = colour;
	*pixel++ = colour;
	*pixel = colour;
}

/*                                         ## = 1 pixel
 *       ##      ##
 *         ##  ##
 *
 *         ##  ##
 *       ##      ##
 */
static inline void draw_target_5x5(SDL_Surface *screen, int x, int y,
		uint32_t colour)
{
	int y_step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + (y - 2) * y_step + x - 2;

	*pixel = colour;
	pixel += 4;
	*pixel = colour;

	pixel += y_step - 1;

	*pixel = colour;
	pixel -= 2;
	*pixel = colour;

	pixel += 2 * y_step;

	*pixel = colour;
	pixel += 2;
	*pixel = colour;

	pixel += y_step - 3;

	*pixel = colour;
	pixel += 4;
	*pixel = colour;
}


/*                                         ## = 1 pixel
 *       ##      ##
 *     ####      ####
 *         ##  ##
 *
 *         ##  ##
 *     ####      ####
 *       ##      ##
 */
static inline void draw_target_7x7(SDL_Surface *screen, int x, int y,
		uint32_t colour)
{
	int y_step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + (y - 3) * y_step + x - 2;

	*pixel = colour;
	pixel += 4;
	*pixel = colour;

	pixel += y_step - 5;

	*pixel++ = colour;
	*pixel = colour;
	pixel += 4;
	*pixel++ = colour;
	*pixel = colour;

	pixel += y_step - 4;

	*pixel = colour;
	pixel += 2;
	*pixel = colour;

	pixel += 2 * y_step;

	*pixel = colour;
	pixel -= 2;
	*pixel = colour;

	pixel += y_step - 2;

	*pixel++ = colour;
	*pixel = colour;
	pixel += 4;
	*pixel++ = colour;
	*pixel = colour;

	pixel += y_step - 5;

	*pixel = colour;
	pixel += 4;
	*pixel = colour;
}

static inline void draw_line(SDL_Surface *screen,
		int x0, int y0, int x1, int y1,
		uint32_t colour)
{
	const int sx = (x0 < x1) ? 1 : -1;
	const int sy = (y0 < y1) ? 1 : -1;
	const int dx =  abs(x1 - x0);
	const int dy = -abs(y1 - y0);
	const int y_step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + y0 * y_step + x0;
	int err = dx + dy;
	int err2;

	while (true) {
		*pixel = colour;

		if (x0 == x1 && y0 == y1) {
			break;
		}

		err2 = err * 2;
		if (err2 >= dy) {
			err += dy;
			x0 += sx;
			pixel += sx;
		}

		if (err2 <= dx) {
			err += dx;
			y0 += sy;
			pixel += sy * y_step;
		}
	}
}

static inline void draw_h_line(SDL_Surface *screen, int x, int y, int len,
		uint32_t colour)
{
	int i;
	uint32_t *pixel = (uint32_t*)screen->pixels +
			y * screen->pitch / peltar_opts.screen_bpp + x;

	for (i = 0; i < len; i++) {
		*pixel++ = colour;
	}
}


static inline void draw_v_line(SDL_Surface *screen, int x, int y, int len,
		uint32_t colour)
{
	int i;
	uint32_t step = screen->pitch / peltar_opts.screen_bpp;
	uint32_t *pixel = (uint32_t*)screen->pixels + y * step + x;

	for (i = 0; i < len; i++) {
		*pixel = colour;
		pixel += step;
	}
}


static inline void draw_box(SDL_Surface *screen, int x, int y,
		int w, int h, uint32_t colour)
{
	draw_h_line(screen, x, y,     w, colour);
	draw_h_line(screen, x, y + h, w, colour);

	draw_v_line(screen, x,     y, h, colour);
	draw_v_line(screen, x + w, y, h, colour);
}


static inline void draw_block(SDL_Surface *screen, int x, int y,
		int w, int h, uint32_t colour)
{
	int i;
	for (i = 0; i < h; i++)
		draw_h_line(screen, x, y + i, w, colour);
}


#endif

