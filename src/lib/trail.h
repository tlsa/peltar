
#ifndef _PELTAR_TRAIL_H_
#define _PELTAR_TRAIL_H_

#include <stdint.h>

#include <SDL/SDL.h>

typedef struct trail trail_t;

trail_t *trail_create(int width, int height);
void trail_destroy(trail_t *trail);
void trail_clear(const trail_t *trail);
void trail_draw(const trail_t *trail, int x0, int y0, int x1, int y1);
void trial_render(const trail_t *trail, SDL_Surface *bg, uint32_t colour);

#endif
