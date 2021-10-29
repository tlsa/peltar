
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "lib/cli.h"
#include "lib/game.h"
#include "lib/types.h"

#define MIN_SIZE 400

struct peltar_config peltar_opts = {
	.screen_width  = 1300,
	.screen_height = 700,
	.screen_bpp   = 4,
	.screen_depth = 32,
};

static const struct cli_table_entry cli_entries[] = {
	{ .l = "fullscreen",  .s = 'f', .t = CLI_BOOL, .v.b = &peltar_opts.fullscreen,
	  .d = "Start up in fullscreen mode." },
	{ .l = "width",       .s = 'w', .t = CLI_UINT, .v.u = &peltar_opts.screen_width,
	  .d = "Window width in pixels." },
	{ .l = "height",      .s = 'h', .t = CLI_UINT, .v.u = &peltar_opts.screen_height,
	  .d = "Window height in pixels." },
};

const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
};

static inline bool peltar_do_stuff(SDL_Surface* screen, struct game *g)
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	game_update(g, screen);

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}

int main(int argc, char* argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	uint32_t ticks, prev_ticks;
	struct game *g;
	int keypress = 0;
	int delay;

	/* Override default options with any command line args */
	if (!cli_parse(&cli, argc, argv)) {
		cli_help(&cli, argv[0]);
		return EXIT_FAILURE;
	}

	if (peltar_opts.screen_width < MIN_SIZE) {
		peltar_opts.screen_width = MIN_SIZE;
	}
	if (peltar_opts.screen_height < MIN_SIZE) {
		peltar_opts.screen_height = MIN_SIZE;
	}

	/* Setup */
	srand(time(NULL));
	game_init();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return EXIT_FAILURE;

	screen = SDL_SetVideoMode(
			peltar_opts.screen_width,
			peltar_opts.screen_height,
			peltar_opts.screen_depth,
			(peltar_opts.fullscreen ? SDL_FULLSCREEN : 0) |
					SDL_DOUBLEBUF | SDL_HWSURFACE);
	if (screen == NULL) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_WM_SetCaption("Peltar", "Peltar");

	if (!game_create(&g,
			peltar_opts.screen_width,
			peltar_opts.screen_height)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	ticks = SDL_GetTicks();

	/* Main game loop */
	while (!keypress) {
		peltar_do_stuff(screen, g);

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				keypress = 1;
				break;

			case SDL_KEYDOWN:
				if (!game_handle_key(g, &event))
					keypress = 1;
				break;

			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				game_handle_mouse(g, &event);
				break;
			}
		}

		prev_ticks = ticks;
		ticks = SDL_GetTicks();
		delay = 1000 / 60 - (ticks - prev_ticks);
		delay = delay < 0 ? 0 : delay;
		SDL_Delay(delay);
	}

	game_free(g);

	SDL_Quit();

	return EXIT_SUCCESS;
}
