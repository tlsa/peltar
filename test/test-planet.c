
#include <stdio.h>
#include <stdbool.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/planet.h"
#include "../src/lib/types.h"


//#define WIDTH 1360
//#define HEIGHT 768
#define WIDTH 520
#define HEIGHT 420

#define SIZE 200

struct peltar_config peltar_opts;


static inline bool screen_draw(SDL_Surface* screen, struct planet *planet)
{
	SDL_Rect rect = {
		.x = (screen->w - SIZE) / 2,
		.y = (screen->h - SIZE) / 2,
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


int main(int argc, char* argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct planet *planet;
	int keypress = 0;

	peltar_opts.screen_width = WIDTH;
	peltar_opts.screen_height = HEIGHT;
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

	if (!planet_create(&planet, SIZE)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!planet_get_texture_from_file(planet, "test/texture.png", screen)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (argc == 1) {
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
	} else {
		int i;
		for (i = 0; i < 50000; i++) {
			planet_update_render(planet, screen, 0, 0);
		}
	}

	planet_free(planet);

	SDL_Quit();

	return EXIT_SUCCESS;
}

