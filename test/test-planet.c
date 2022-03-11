
#include <stdio.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/planet.h"
#include "../src/lib/types.h"
#include "../src/lib/cli.h"

struct peltar_config peltar_opts;

static struct peltar_options {
	bool time;
	uint64_t count;
	uint64_t radius;
} opt = {
	.time = false,
	.count = 50000,
	.radius = 100,
};

static const struct cli_table_entry cli_entries[] = {
	{ .l = "time" , .s = 't', .t = CLI_BOOL, .v.b = &opt.time,
	  .d = "Exit after rendering a fixed number of frames."  },
	{ .l = "count", .s = 'c', .t = CLI_UINT, .v.u = &opt.count,
	  .d = "Number of frames to render before exiting. " },
	{ .l = "radius", .s = 'r', .t = CLI_UINT, .v.u = &opt.radius,
	  .d = "Radius of planet in pixels." },
};

const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
};

static inline bool screen_draw(SDL_Surface* screen, struct planet *planet)
{
	SDL_Rect rect = {
		.x = (screen->w - opt.radius * 2) / 2,
		.y = (screen->h - opt.radius * 2) / 2,
		.w = 0,
		.h = 0
	};

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	planet_update_render(planet, screen, rect.x, rect.y);

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct planet *planet;
	int keypress = 0;

	if (!cli_parse(&cli, argc, (void *)argv)) {
		cli_help(&cli, argv[0]);
		return EXIT_FAILURE;
	}

	peltar_opts.screen_width = opt.radius * 9 / 4;
	peltar_opts.screen_height = opt.radius * 9 / 4;
	peltar_opts.screen_bpp = 4;
	peltar_opts.screen_depth = 32;

	planet_init();

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		return EXIT_FAILURE;

	screen = SDL_SetVideoMode(peltar_opts.screen_width,
			peltar_opts.screen_height,
			peltar_opts.screen_depth,
			/* SDL_FULLSCREEN | */ SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (screen == NULL) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	SDL_WM_SetCaption("Test: Planet rendering", "Test: Planet");

	if (!planet_create(&planet, opt.radius * 2)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!planet_get_texture_from_file(planet, "test/texture.png", screen)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (opt.time) {
		for (uint64_t i = 0; i < opt.count; i++) {
			planet_update_render(planet, screen, 0, 0);
		}
	} else {
		while (!keypress) {
			screen_draw(screen, planet);

			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_QUIT:
					keypress = 1;
					break;
				case SDL_KEYDOWN:
					keypress = 1;
					break;
				}
			}
		}
	}

	planet_free(planet);

	SDL_Quit();

	return EXIT_SUCCESS;
}

