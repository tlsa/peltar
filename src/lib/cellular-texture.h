
#ifndef _PELTAR_CELLULAR_TEXTURE_H_
#define _PELTAR_CELLULAR_TEXTURE_H_

#include <stdbool.h>
#include "fixed-point.h"

struct cellular_texture;
struct point_3d;

bool cellular_texture_create(struct cellular_texture **cell, int diameter);
peltar_fixed cellular_texture_get_dist(struct cellular_texture *cell,
		struct point_3d p);
void cellular_texture_free(struct cellular_texture *cell);

#endif

