
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "draw.h"
#include "planet.h"
#include "player.h"

#define MAX_STRENGTH (256 + 128)


struct player {
	bool show_direction;

	char *name;
	uint32_t colour;

	int strength;

	int x;
	int y;
	int scaled_x;
	int scaled_y;

	int target_x;
	int target_y;

	int direction_x;
	int direction_y;
	int direction_size;

	int size;
	int size_scaled;

	struct planet *planet; /* player graphic */
};


bool player_create(struct player **player, uint32_t colour)
{
	*player = malloc(sizeof(struct player));
	if (*player == NULL)
		return false;

	(*player)->colour = colour;
	(*player)->name = NULL;
	(*player)->strength = MAX_STRENGTH / 2;
	(*player)->show_direction = false;
	(*player)->direction_size = 0;

	(*player)->planet = NULL;

	return true;
}


void player_free(struct player *player)
{
	assert(player != NULL);

	if (player->name != NULL)
		free(player->name);

	if (player->planet != NULL)
		planet_free(player->planet);

	free(player);
}


bool player_setup_graphics(struct player *p, int size)
{
	if (!planet_create(&p->planet, size)) {
		return false;
	}
		
	if (!planet_generate_texture_man_made(p->planet, p->colour)) {
		planet_free(p->planet);
		return false;
	}

	p->x = 0;
	p->y = 0;
	p->scaled_x = 0;
	p->scaled_y = 0;

	p->target_x = 0;
	p->target_y = 0;

	p->size = planet_get_size(p->planet);
	p->size_scaled = planet_get_size_scaled(p->planet);

	planet_set_lighting(p->planet, true);

	return true;
}


void player_set_pos(struct player *p, int x, int y, int scaled_x, int scaled_y)
{
	p->x = x;
	p->y = y;
	p->scaled_x = scaled_x;
	p->scaled_y = scaled_y;
}


bool player_handle_key(struct player *p, SDL_Event *event)
{
	return false;
}


int player_get_size(struct player *p)
{
	return planet_get_size(p->planet);
}


int player_get_size_scaled(struct player *p)
{
	return planet_get_size_scaled(p->planet);
}


uint32_t player_get_colour(struct player *p)
{
	return p->colour;
}


void player_set_lighting(struct player *p, bool lighting)
{
	planet_set_lighting(p->planet, lighting);
}


void player_show_direction(struct player *p, bool show)
{
	p->show_direction = show;
}


static void player_remove_direction(SDL_Surface *screen, SDL_Surface *bg,
		int x, int y, int size)
{
	SDL_Rect rect1 = {
		.w = 0,
		.h = 0
	};
	SDL_Rect rect2;

	rect1.x = rect2.x = x;
	rect1.y = rect2.y = y;
	rect2.w = size;
	rect2.h = size;

	SDL_BlitSurface(bg, &rect2, screen, &rect1);
}


void player_render_direction(struct player *p,
		SDL_Surface *screen, SDL_Surface *bg)
{
	if (p->show_direction) {
		const int size = 7;
		int rad = p->size / 2;
		int cx = p->x + rad;
		int cy = p->y + rad;

		int diffx = p->target_x - cx;
		int diffy = p->target_y - cy;

		double scale = (rad + 7) /
				((diffx == 0 && diffy == 0) ?
				1 : sqrt(diffx * diffx + diffy * diffy));

		int direction_x = cx + scale * diffx;
		int direction_y = cy + scale * diffy;

		if (p->direction_size != 0) {
			player_remove_direction(screen, bg,
					p->direction_x, p->direction_y,
					p->direction_size);
		}

		draw_target_7x7(screen, direction_x, direction_y, p->colour);

		p->direction_x = direction_x - size / 2;
		p->direction_y = direction_y - size / 2;
		p->direction_size = size;

	} else if (p->direction_size != 0) {
		player_remove_direction(screen, bg,
				p->direction_x, p->direction_y,
				p->direction_size);
		p->direction_size = 0;
	}
}


void player_render_direction_scaled(struct player *p,
		SDL_Surface *screen, SDL_Surface *bg)
{
	if (p->show_direction) {
		const int size = 3;
		int rad = p->size_scaled / 2;
		int cx = p->scaled_x + rad;
		int cy = p->scaled_y + rad;

		int diffx = p->target_x - cx;
		int diffy = p->target_y - cy;

		double scale = (rad + 4) / sqrt(diffx * diffx + diffy * diffy);

		int direction_x = cx + scale * diffx;
		int direction_y = cy + scale * diffy;

		if (p->direction_size != 0) {
			player_remove_direction(screen, bg,
					p->direction_x, p->direction_y,
					p->direction_size);
		}

		draw_target_3x3(screen, direction_x, direction_y, p->colour);

		p->direction_x = direction_x - size / 2;
		p->direction_y = direction_y - size / 2;
		p->direction_size = size;

	} else if (p->direction_size != 0) {
		player_remove_direction(screen, bg,
				p->direction_x, p->direction_y,
				p->direction_size);
		p->direction_size = 0;
	}
}


void player_update_render(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg)
{
	player_render_direction(p, screen, bg);
	planet_update_render(p->planet, screen, p->x, p->y);
}


void player_update_render_scaled(struct player *p, SDL_Surface *screen,
		SDL_Surface *bg)
{
	player_render_direction_scaled(p, screen, bg);
	planet_update_render_scaled(p->planet, screen,
			p->scaled_x, p->scaled_y);
}


void player_set_mouse_pos_to_target(struct player *p)
{
	SDL_WarpMouse(p->target_x, p->target_y);
}


void player_set_target(struct player *p, Uint16 x, Uint16 y)
{
	p->target_x = x;
	p->target_y = y;
}


void player_get_target(struct player *p, int *x, int *y, int *vec_x, int *vec_y)
{
	int rad = p->size / 2;
	int cx = p->x + rad;
	int cy = p->y + rad;

	*x = p->direction_x + p->direction_size / 2;
	*y = p->direction_y + p->direction_size / 2;

	*vec_x = *x - cx;
	*vec_y = *y - cy;
}


void player_increase_strength(struct player *p)
{
	p->strength += 4;

	if (p->strength > MAX_STRENGTH)
		p->strength = MAX_STRENGTH;
}


void player_reduce_strength(struct player *p)
{
	p->strength -= 4;

	if (p->strength < 0)
		p->strength = 0;
}


int player_get_strength(struct player *p)
{
	return p->strength;
}


void player_render_strength(struct player *p, SDL_Surface *screen,
		int x, int y, int w, int h)
{
	int split = w * p->strength / MAX_STRENGTH;

	draw_block(screen, x, y, split, h, p->colour);

	draw_block(screen, x + split, y, w - split, h,
			(((p->colour & 0xff00ff) >> 1) & 0xff00ff) |
			(((p->colour & 0x00ff00) >> 1) & 0x00ff00));
}
