
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "../src/lib/planet.h"
#include "../src/lib/types.h"
#include "../src/lib/cli.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct peltar_config peltar_opts;

static struct peltar_options {
	bool cellular;
	uint64_t radius;

} opt = {
	.cellular = false,
	.radius = 100,
};

static const struct cli_table_entry cli_entries[] = {
	{ .l = "cellular", .s = 'c', .t = CLI_BOOL, .v.b = &opt.cellular,
	  .d = "Use a cellular texture."  },
	{ .l = "radius", .s = 'r', .t = CLI_UINT, .v.u = &opt.radius,
	  .d = "Radius of planet in pixels." },
};

const struct cli_table cli = {
	.entries = cli_entries,
	.count = (sizeof(cli_entries))/(sizeof(*cli_entries)),
};

static inline int get_border(void)
{
	return opt.radius / 16;
}

static inline int get_height(void)
{
	return opt.radius * 4 + get_border() * 3;
}

static inline int get_width(void)
{
	return get_border()
		+ opt.radius * 2
		+ get_border()
		+ opt.radius * 2 * M_PI
		+ get_border()
		+ opt.radius / 4 * 2 * M_PI
		+ get_border();
}

bool screen_draw(SDL_Surface* screen, struct planet *p1, struct planet *p2,
		unsigned int t)
{
	const int border = get_border();
	const int diameter = opt.radius * 2;
	const struct {
		struct point bp;
		struct point bt;
		struct point sp;
		struct point st;
	} pos = {
		.bp = {
			.x = border,
			.y = border,
		},
		.bt = {
			.x = border + diameter + border,
			.y = border,
		},
		.sp = {
			.x = border + diameter + border + diameter * M_PI + border,
			.y = border + diameter / 4 + border,
		},
		.st = {
			.x = border + diameter + border + diameter * M_PI + border,
			.y = border,
		},
	};
	const int y2 = diameter + border;

	if (SDL_MUSTLOCK(screen)) {
		if (SDL_LockSurface(screen) < 0)
			return false;
	}

	planet_update_render(p1, screen, pos.bp.x, pos.bp.y);
	planet_update_render(p2, screen, pos.bp.x, pos.bp.y + y2);

	planet_update_render_scaled(p1, screen, pos.sp.x, pos.sp.y);
	planet_update_render_scaled(p2, screen, pos.sp.x, pos.sp.y + y2);

	if (t < 2) {
		/* render textures */
		planet_plot_texture(p1, screen, pos.bt.x, pos.bt.y);
		planet_plot_texture(p2, screen, pos.bt.x, pos.bt.y + y2);
		planet_plot_texture_scaled(p1, screen, pos.st.x, pos.st.y);
		planet_plot_texture_scaled(p2, screen,pos.st.x, pos.st.y + y2);
	}

	if (SDL_MUSTLOCK(screen))
		SDL_UnlockSurface(screen);

	SDL_Flip(screen);

	return true;
}

int main(int argc, char *argv[])
{
	SDL_Surface *screen;
	SDL_Event event;
	struct planet *p1;
	struct planet *p2;
	unsigned int t = 0;
	int keypress = 0;
	bool lighting = false;

	if (!cli_parse(&cli, argc, (void *)argv)) {
		cli_help(&cli, argv[0]);
		return EXIT_FAILURE;
	}

	peltar_opts.screen_width = get_width();
	peltar_opts.screen_height = get_height();
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

	if (!planet_create(&p1, opt.radius * 2)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}
	if (!planet_create(&p2, opt.radius * 2)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}

	if (!planet_get_texture_from_file(p1, "test/texture.png", screen)) {
		SDL_Quit();
		return EXIT_FAILURE;
	}
	if (opt.cellular) {
		if (!planet_generate_texture_man_made(p2, (struct colour)
				{ .r = 0xff, .g = 0x00, .b = 0x00 },
				screen)) {
			SDL_Quit();
			return EXIT_FAILURE;
		}
	} else {
		if (!planet_generate_texture(p2, screen)) {
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
					if (!planet_generate_texture(p2,
							screen)) {
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

