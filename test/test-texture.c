
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/planet.h"
#include "../src/lib/types.h"


#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif


//#define WIDTH 1360
//#define HEIGHT 768
//#define WIDTH 880
#define WIDTH 1020
#define HEIGHT 440

#define SIZE 200

struct peltar_config peltar_opts;


bool screen_draw(SDL_Surface* screen, struct planet *p1, struct planet *p2,
		unsigned int t)
{
	SDL_Rect rect1;
	SDL_Rect rect2;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	rect1.x = (screen->w -
			(SIZE + SIZE * M_PI + 8 + SIZE * M_PI / 4 + 8)) / 2;
	rect1.y = (screen->h - 2 * SIZE) / 2 - 4;
	rect1.w = 0;
	rect1.h = 0;

	rect2.x = (screen->w -
			(SIZE + SIZE * M_PI + 8 + SIZE * M_PI / 4 + 8)) / 2;
	rect2.y = screen->h / 2 + 4;
	rect2.w = 0;
	rect2.h = 0;

	planet_update_render(p1, screen, rect1.x, rect1.y);
	planet_update_render(p2, screen, rect2.x, rect2.y);

	planet_update_render_scaled(p1, screen,
			rect1.x + SIZE + 8 + SIZE * M_PI + 8,
			rect1.y + SIZE / 4 + 8);
	planet_update_render_scaled(p2, screen,
			rect2.x + SIZE + 8 + SIZE * M_PI + 8,
			rect2.y + SIZE / 4 + 8);

	if (t < 2) {
		/* render textures */
		planet_plot_texture(p1, screen, rect1.x + SIZE + 8, rect1.y);
		planet_plot_texture(p2, screen, rect2.x + SIZE + 8, rect2.y);
		planet_plot_texture_scaled(p1, screen,
				rect1.x + SIZE + 8 + SIZE * M_PI + 8, rect1.y);
		planet_plot_texture_scaled(p2, screen,
				rect2.x + SIZE + 8 + SIZE * M_PI + 8, rect2.y);
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}


int main(int argc, char* argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct planet *p1;
	struct planet *p2;
	unsigned int t = 0;
	int keypress = 0;
	bool lighting = false;

	peltar_opts.screen_width = WIDTH;
	peltar_opts.screen_height = HEIGHT;
	peltar_opts.screen_bpp = 4;
	peltar_opts.screen_depth = 32;

	planet_init();
	srand(time(NULL));

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

	SDL_WM_SetCaption("Test: Planet texture generation", "Test: Texture");

	if (!planet_create(&p1, SIZE)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}
	if (!planet_create(&p2, SIZE)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!planet_get_texture_from_file(p1, "test/texture.png", screen)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}
	if (argc == 1) {
		if (!planet_generate_texture(p2)) {
			SDL_Quit();
			return EXIT_FAILURE;
		}
	} else {
		if (!planet_generate_texture_man_made(p2, 0xff0000)) {
			SDL_Quit();
			return EXIT_FAILURE;
		}
	}

	while (!keypress) {
		screen_draw(screen, p1, p2, t++);

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT:
				keypress = 1;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_l) {
					lighting = !lighting;
					planet_set_lighting(p1, lighting);
					planet_set_lighting(p2, lighting);
				} else if (event.key.keysym.sym == SDLK_t) {
					if (!planet_generate_texture(p2)) {
						SDL_Quit();
						return EXIT_FAILURE;
					}
					t = 0;
				} else {
					keypress = 1;
				}
				break;
			}
		}
	}

	planet_free(p1);
	planet_free(p2);

	SDL_Quit();

	return EXIT_SUCCESS;
}

