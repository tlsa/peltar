
#ifndef _PELTAR_TYPES_H_
#define _PELTAR_TYPES_H_

#include <stdbool.h>

#include "fixed-point.h"

struct point_3d {
	peltar_fixed x;
	peltar_fixed y;
	peltar_fixed z;
};

struct point {
	int x;
	int y;
};

struct peltar_config {
	bool fullscreen;
	uint64_t screen_width;
	uint64_t screen_height;
	uint64_t screen_bpp;
	uint64_t screen_depth;
};

extern struct peltar_config peltar_opts;

#endif

