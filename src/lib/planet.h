
#ifndef _PELTAR_PLANET_H_
#define _PELTAR_PLANET_H_

#include <stdbool.h>
#include <SDL.h>


struct planet;

void planet_init(void);

bool planet_create(struct planet **p, int size);
void planet_free(struct planet *p);

bool planet_get_texture_from_file(struct planet *planet, const char *filename,
		SDL_Surface *screen);
bool planet_generate_texture(struct planet *planet);
bool planet_generate_texture_man_made(struct planet *planet, uint32_t colour);

void planet_update_render(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y);
void planet_update_render_scaled(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y);

void planet_plot_texture(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y);
void planet_plot_texture_scaled(struct planet *p, SDL_Surface *screen,
		int screen_x, int screen_y);

void planet_set_lighting(struct planet *p, bool lighting);

int planet_get_size(const struct planet *p);
int planet_get_size_scaled(const struct planet *p);

int planet_get_mass(const struct planet *p);

#endif

