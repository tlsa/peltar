
#ifndef _PELTAR_LEVEL_H_
#define _PELTAR_LEVEL_H_

#include <stdbool.h>
#include <SDL.h>

struct level;
struct player;

void level_init(void);

bool level_create(struct level **level, struct player *p1, struct player *p2,
		int width, int height, const SDL_Surface *screen);
void level_free(struct level *level);

void level_begin(struct level *l);
int level_get_winner(struct level *l);

bool level_handle_key(struct level *l, SDL_Event *event,
		const SDL_Surface *screen);
bool level_handle_mouse(struct level *l, SDL_Event *event);

/* render what's changed.  return true iff level is finished. */
bool level_update_render(struct level *l, SDL_Surface *screen);

/* render everything.  return true iff level is finished. */
bool level_update_render_full(struct level *l, SDL_Surface *screen);

#endif

