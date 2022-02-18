
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/cli.h"
#include "../src/lib/types.h"
#include "../src/lib/image.h"
#include "../src/lib/texture/starscape.h"


#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif


#define WIDTH 1360
#define HEIGHT 768

struct peltar_config peltar_opts;


bool screen_draw(SDL_Surface* screen, struct image *bg1, unsigned int t)
{
	SDL_Rect rect1;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	rect1.x = 0;
	rect1.y = 0;
	rect1.w = 0;
	rect1.h = 0;

	if (t < 2) {
		/* render starscape */
		SDL_BlitSurface(image_get_surface(bg1), NULL, screen, &rect1);
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}

static struct peltar_options {
	bool time;
	uint64_t count;
	uint64_t w;
	uint64_t h;
} opt = {
	.time = false,
	.count = 25,
	.w = WIDTH,
	.h = HEIGHT,
};

static const struct cli_table_entry cli_entries[] = {
	{ .l = "time"  , .s = 't', .t = CLI_BOOL, .v.b = &opt.time,
	  .d = "Exit after generating a fixed number of textures."  },
	{ .l = "count" , .s = 'c', .t = CLI_UINT, .v.u = &opt.count,
	  .d = "Number of textures to generate before exiting. " },
	{ .l = "width" , .s = 'w', .t = CLI_UINT, .v.u = &opt.w,
	  .d = "Window width in pixels. " },
	{ .l = "height", .s = 'h', .t = CLI_UINT, .v.u = &opt.h,
	  .d = "Window height in pixels. " },
};

const struct cli_table cli = {
	.entries = cli_entries,
	.count = CLI_ARRAY_LEN(cli_entries),
};

int main(int argc, const char *argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct image *bg1;
	unsigned int t = 0;
	int keypress = 0;

	if (!cli_parse(&cli, argc, argv)) {
		cli_help(&cli, argv[0]);
		return EXIT_FAILURE;
	}

	peltar_opts.screen_width = opt.w;
	peltar_opts.screen_height = opt.h;
	peltar_opts.screen_bpp = 4;
	peltar_opts.screen_depth = 32;

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

	SDL_WM_SetCaption("Test: Starscapes", "Test: Starscapes");

	if (!image_create(&bg1, WIDTH, HEIGHT)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (opt.time) {
		for (uint64_t i = 0; i < opt.count; i++) {
			if (!texture_get_starscape(bg1)) {
				printf("Failed.");
				SDL_Quit();
				return EXIT_FAILURE;
			}
		}
	} else {
		while (!keypress) {
			screen_draw(screen, bg1, t++);

			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_QUIT:
					keypress = 1;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_b) {
						if (!texture_get_starscape(bg1)) {
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
	}

	image_free(bg1);

	SDL_Quit();

	return EXIT_SUCCESS;
}

