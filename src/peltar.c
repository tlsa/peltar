
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "lib/game.h"
#include "lib/types.h"


#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif


//#define WIDTH 1360
//#define HEIGHT 768

#define WIDTH 1300
#define HEIGHT 700
#define MIN_SIZE 400

struct peltar_config peltar_opts;

static bool read_u32(
		const char *value,
		uint32_t *out)
{
	unsigned long long temp;
	char *end = NULL;

	errno = 0;
	temp = strtoull(value, &end, 0);

	if (end == value || errno == ERANGE || temp > INT32_MAX) {
		return false;
	}

	*out = (uint32_t)temp;
	return true;
}

static void cli_parse(int argc, char* argv[])
{
	int i;

	for (i = 1; i < argc; i++) {
		uint32_t val;

		if (*argv[i]++ != '-') {
			/* Only interested in stuff starting with '-' */
			continue;
		}
		if (*argv[i] == 'f') {
			peltar_opts.fullscreen = true;
		}
		if (*argv[i] == 'w' && read_u32(++argv[i], &val)) {
			peltar_opts.screen_width = val;
		}
		if (*argv[i] == 'h' && read_u32(++argv[i], &val)) {
			peltar_opts.screen_height = val;
		}
	}

	if (peltar_opts.screen_width < MIN_SIZE) {
		peltar_opts.screen_width = MIN_SIZE;
	}
	if (peltar_opts.screen_height < MIN_SIZE) {
		peltar_opts.screen_height = MIN_SIZE;
	}
}


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

	/* Default options */
	peltar_opts.fullscreen = false;
	peltar_opts.screen_width = WIDTH;
	peltar_opts.screen_height = HEIGHT;
	peltar_opts.screen_bpp = 4;
	peltar_opts.screen_depth = 32;

	/* Override default options with any command line args */
	cli_parse(argc, argv);

	/* Setup */
	srand(time(NULL));
	game_init();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return EXIT_FAILURE;

	screen = SDL_SetVideoMode(peltar_opts.screen_width,
			peltar_opts.screen_height,
			peltar_opts.screen_depth,
			(peltar_opts.fullscreen ? SDL_FULLSCREEN : 0) |
					SDL_DOUBLEBUF | SDL_HWSURFACE);
	if (screen == NULL) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_WM_SetCaption("Peltar", "Peltar");

	if (!game_create(&g, peltar_opts.screen_width,
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

