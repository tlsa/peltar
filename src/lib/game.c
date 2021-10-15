

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "draw.h"
#include "game.h"
#include "level.h"
#include "player.h"
#include "types.h"



struct game {
	struct level *l;

	struct player *p1;
	struct player *p2;

	bool start;
};


void game_init(void)
{
	level_init();
}


void game_free(struct game *game)
{
	assert(game != NULL);

	if (game->l != NULL)
		level_free(game->l);

	if (game->p1 != NULL)
		player_free(game->p1);

	if (game->p2 != NULL)
		player_free(game->p2);

	free(game);
}


static bool game_create_details(struct game *game, int width, int height)
{
	if (!player_create(&game->p1, 0x000077ff)) {
		return false;
	}

	if (!player_create(&game->p2, 0x00ff0000)) {
		return false;
	}

	if (!level_create(&game->l, game->p1, game->p2, width, height)) {
		return false;
	}

	return true;
}


bool game_create(struct game **game, int width, int height)
{
	*game = malloc(sizeof(struct game));
	if (*game == NULL)
		return false;

	(*game)->l = NULL;
	(*game)->p1 = NULL;
	(*game)->p2 = NULL;
	(*game)->start = true;

	if (!game_create_details(*game, width, height)) {
		game_free(*game);
		return false;
	}

	return true;
}

bool game_handle_key(struct game *g, SDL_Event *event)
{
	if (level_handle_key(g->l, event))
		return true;

	switch (event->key.keysym.sym) {
	case SDLK_ESCAPE:
		/* Failure to handle this means the main loop exits, and
		 * game escapes. */
		return false;
	default:
		break;
	}
	return true;
}

bool game_handle_mouse(struct game *g, SDL_Event *event)
{
	if (level_handle_mouse(g->l, event))
		return true;

	return false;
}

void game_update(struct game *g, SDL_Surface *screen)
{
	bool complete;

	if (g->start) {
		level_begin(g->l);
		complete = level_update_render_full(g->l, screen);
		g->start = false;

	} else {
		complete = level_update_render(g->l, screen);
	}

	if (complete) {
		printf("Player %i wins!\n", level_get_winner(g->l));
		//TODO: Highscore table, menu, etc, rather than free & exit.
		level_free(g->l);
		player_free(g->p1);
		player_free(g->p2);
		exit(EXIT_SUCCESS);
	}
}

