
#ifndef _PELTAR_GAME_H_
#define _PELTAR_GAME_H_

#include <stdbool.h>
#include <SDL.h>

struct game;

void game_init(void);

bool game_create(struct game **game, int width, int height,
		const SDL_Surface *screen);
void game_free(struct game *game);

bool game_handle_key(struct game *g, SDL_Event *event,
		const SDL_Surface *screen);
bool game_handle_mouse(struct game *g, SDL_Event *event);

/* render what's changed */
void game_update(struct game *g, SDL_Surface *screen);

#endif

