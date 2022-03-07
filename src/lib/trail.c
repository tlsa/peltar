
#include <stdbool.h>

#include "trail.h"
#include "draw.h"
#include "util.h"

struct trail {
	int width;
	int height;

	int row_stride;

	size_t count;
	uint32_t *data;
};

static void trail_draw_internal(const trail_t *trail,
		int x0, int y0, int x1, int y1)
{
	const int sx = (x0 < x1) ? 1 : -1;
	const int sy = (y0 < y1) ? 1 : -1;
	const int px = (sx < 0) ? 0 : 1;
	const int dx =  abs(x1 - x0);
	const int dy = -abs(y1 - y0);
	const int y_step = trail->row_stride;
	uint32_t *pixel = trail->data + y0 * y_step + x0 / 32;
	int err = dx + dy;
	int err2;

	while (true) {
		*pixel |= 1u << (x0 & (32 - 1));

		if (x0 == x1 && y0 == y1) {
			break;
		}

		err2 = err * 2;
		if (err2 >= dy) {
			err += dy;
			if (((x0 + px) & (32 - 1)) == 0) {
				pixel += sx;
			}
			x0 += sx;
		}

		if (err2 <= dx) {
			err += dx;
			y0 += sy;
			pixel += sy * y_step;
		}
	}
}

void trail_draw(const trail_t *trail,
		int x0, int y0, int x1, int y1)
{
	if (x0 < 0 || x0 >= trail->width) return;
	if (x1 < 0 || x1 >= trail->width) return;
	if (y0 < 0 || y0 >= trail->height) return;
	if (y1 < 0 || y1 >= trail->height) return;

	trail_draw_internal(trail, x0, y0, x1, y1);
}

void trail_render(const trail_t *trail,
		SDL_Surface *bg,
		uint32_t colour)
{
	draw_bitmap_1bpp(bg, trail->width, trail->height,
			trail->row_stride, trail->data,
			colour);
}

void trail_clear(const trail_t *trail)
{
	memset(trail->data, 0, trail->count * sizeof(*trail->data));
}

void trail_destroy(trail_t *trail)
{
	if (trail != NULL) {
		free(trail->data);
		free(trail);
	}
}

trail_t *trail_create(int width, int height)
{
	trail_t *trail;

	trail = malloc(sizeof(*trail));
	if (trail == NULL) {
		return NULL;
	}

	trail->width = width;
	trail->height = height;
	trail->row_stride = peltar_round_up_n(width / 32, 32);
	trail->count = trail->row_stride * trail->height;

	trail->data = malloc(trail->count * sizeof(*trail->data));
	if (trail->data == NULL) {
		free(trail);
		return NULL;
	}

	trail_clear(trail);
	return trail;
}
