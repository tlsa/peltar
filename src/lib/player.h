
#ifndef _PELTAR_PLAYER_H_
#define _PELTAR_PLAYER_H_

#include <stdbool.h>
#include <stdint.h>
#include <SDL.h>

struct player;

bool player_create(struct player **player, uint32_t colour);
void player_free(struct player *player);

bool player_setup_graphics(struct player *p, int size);

bool player_handle_key(struct player *p, SDL_Event *event);

void player_set_pos(struct player *p, int x, int y, int scaled_x, int scaled_y);
void player_set_lighting(struct player *p, bool lighting);

void player_show_direction(struct player *p, bool show);


void player_render_direction(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg);
void player_render_direction_scaled(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg);

void player_update_render(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg);
void player_update_render_scaled(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg);

int player_get_size(struct player *p);
int player_get_size_scaled(struct player *p);

uint32_t player_get_colour(struct player *p);

void player_set_mouse_pos_to_target(struct player *p);
void player_set_target(struct player *p, Uint16 x, Uint16 y);
void player_get_target(struct player *p, int *x, int *y,
		int *vec_x, int *vec_y);

void player_increase_strength(struct player *p);
void player_reduce_strength(struct player *p);
int player_get_strength(struct player *p);

void player_render_strength(struct player *p, SDL_Surface *screen,
		int x, int y, int w, int h);

#endif

