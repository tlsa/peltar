
#ifndef _PELTAR_TYPES_H_
#define _PELTAR_TYPES_H_

#include <stdbool.h>

#include "fixed-point.h"

struct point_3d {
	peltar_fixed x;
	peltar_fixed y;
	peltar_fixed z;
};

struct peltar_config {
	bool fullscreen;
	uint32_t screen_width;
	uint32_t screen_height;
	uint32_t screen_bpp;
	uint32_t screen_depth;
};

extern struct peltar_config peltar_opts;

#endif

