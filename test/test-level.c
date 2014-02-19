
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/level.h"
#include "../src/lib/types.h"
#include "../src/lib/player.h"


#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif


//#define WIDTH 1360
//#define HEIGHT 768
//#define WIDTH 800
//#define HEIGHT 600

#define WIDTH 1300
#define HEIGHT 700

#define SIZE 200

struct peltar_config peltar_opts;


static inline bool screen_draw(SDL_Surface* screen, struct level *l,
		unsigned int *t)
{
	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	if (*t < 2)
		level_update_render_full(l, screen);
	else
		level_update_render(l, screen);

	(*t)++;

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}


int main(int argc, char* argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct level *l;
	struct player *p1;
	struct player *p2;
	int keypress = 0;
	unsigned int t;

	peltar_opts.screen_width = WIDTH;
	peltar_opts.screen_height = HEIGHT;
	peltar_opts.screen_bpp = 4;
	peltar_opts.screen_depth = 32;

	srand(time(NULL));
	level_init();

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

	SDL_WM_SetCaption("Test: Level creation", "Test: Levels");

	if (!player_create(&p1, 0x001144ff)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!player_create(&p2, 0x00ff0000)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!level_create(&l, p1, p2, peltar_opts.screen_width,
			peltar_opts.screen_height)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	t = 0;
	while (!keypress) {
		screen_draw(screen, l, &t);

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				keypress = 1;
				break;
			case SDL_KEYDOWN:
				if (!level_handle_key(l, &event))
					keypress = 1;
				break;
			}
		}
	}

	level_free(l);

	SDL_Quit();

	return EXIT_SUCCESS;
}

