
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "fixed-point.h"
#include "cellular-texture.h"
#include "types.h"


#define CENTER_MAX 1
#define CELL_SIZE 20
#define DISTS 4;

struct cell {
	struct point_3d c;	/* Centres */
};

struct cellular_texture {
	struct cell *cells; /* The cells */

	int x; /* Number of cells in x direction */
	int y; /* Number of cells in y direction */
	int z; /* Number of cells in z direction */

	int slice;
	int row;
};


static bool cellular_texture_create_details(struct cellular_texture *cell,
		int diameter)
{
	int i, j, k, index;
	int n_cells = diameter / CELL_SIZE + 3;
	int n_cells_squared;
	int n_cells_cubed;

	peltar_fixed x_off, y_off, z_off;
	peltar_fixed cell_step = CELL_SIZE << FIX_SHIFT;

	cell->x = n_cells;
	cell->y = n_cells;
	cell->z = n_cells;

	n_cells_squared = n_cells * n_cells;
	n_cells_cubed = n_cells_squared * n_cells;

	cell->row = n_cells;
	cell->slice = n_cells_squared;

	/* Allocate cells */
	cell->cells = malloc(n_cells_cubed * sizeof(struct cell));
	if (cell->cells == NULL)
		return false;

	/* Make random cell centres */
	index = 0;
	for (i = 0; i < n_cells; i++) {
		x_off = (i * CELL_SIZE) << FIX_SHIFT;
		for (j = 0; j < n_cells; j++) {
			y_off = (j * CELL_SIZE) << FIX_SHIFT;
			for (k = 0; k < n_cells; k++) {
				z_off = (k * CELL_SIZE) << FIX_SHIFT;

				cell->cells[index].c.x = x_off +
						rand() % (14 * cell_step / 16) +
						cell_step / 16;
				cell->cells[index].c.y = y_off +
						rand() % (14 * cell_step / 16) +
						cell_step / 16;
				cell->cells[index].c.z = z_off +
						rand() % (14 * cell_step / 16) +
						cell_step / 16;
				index++;
			}
		}
	}

	return true;
}


bool cellular_texture_create(struct cellular_texture **cell, int diameter)
{
	*cell = malloc(sizeof(struct cellular_texture));
	if (*cell == NULL)
		return false;

	if (!cellular_texture_create_details(*cell, diameter)) {
		return false;
	}

	return true;
}


void cellular_texture_free(struct cellular_texture *cell)
{
	assert(cell != NULL);

	if (cell->cells != NULL)
		free(cell->cells);

	free(cell);
}


static inline void cellular_texture_get_dist_cell(
		struct cellular_texture *cell, int x, int y, int z,
		uint64_t dist[2], struct point_3d p)
{
	struct cell *c = cell->cells + (x * cell->slice) + (y * cell->row) + z;
	uint64_t dx, dy, dz;

	if (c->c.x > p.x)
		dx = c->c.x - p.x;
	else
		dx = p.x - c->c.x;
	dx *= dx;

	if (c->c.y > p.y)
		dy = c->c.y - p.y;
	else
		dy = p.y - c->c.y;
	dy *= dy;

	if (c->c.z > p.z)
		dz = c->c.z - p.z;
	else
		dz = p.z - c->c.z;
	dz *= dz;

	if (dx + dy + dz < dist[1]) {
		dist[0] = dist[1];
		dist[1] = dx + dy + dz;

	} else if (dx + dy + dz < dist[0]) {
		dist[0] = dx + dy + dz;
	}
}


static inline void cellular_texture_get_dist_row(
		struct cellular_texture *cell, int x, int y,
		uint64_t dist[2], struct point_3d p)
{
	int z;

	z = ((p.z / CELL_SIZE) >> FIX_SHIFT);

	cellular_texture_get_dist_cell(cell, x, y, z - 1, dist, p);
	cellular_texture_get_dist_cell(cell, x, y, z    , dist, p);
	cellular_texture_get_dist_cell(cell, x, y, z + 1, dist, p);
}


static inline void cellular_texture_get_dist_slice(
		struct cellular_texture *cell, int x,
		uint64_t dist[2], struct point_3d p)
{
	int y;

	y = ((p.y / CELL_SIZE) >> FIX_SHIFT);

	cellular_texture_get_dist_row(cell, x, y - 1, dist, p);
	cellular_texture_get_dist_row(cell, x, y    , dist, p);
	cellular_texture_get_dist_row(cell, x, y + 1, dist, p);
}


peltar_fixed cellular_texture_get_dist(struct cellular_texture *cell,
		struct point_3d p)
{
	uint64_t dist[2];
	int x;

	p.x += CELL_SIZE << FIX_SHIFT;
	p.y += CELL_SIZE << FIX_SHIFT;
	p.z += CELL_SIZE << FIX_SHIFT;

	x = ((p.x / CELL_SIZE) >> FIX_SHIFT);

	dist[0] = ~0;
	dist[1] = ~0;

	cellular_texture_get_dist_slice(cell, x - 1, dist, p);
	cellular_texture_get_dist_slice(cell, x    , dist, p);
	cellular_texture_get_dist_slice(cell, x + 1, dist, p);

	return sqrt(dist[0]) - sqrt(dist[1]);
}


