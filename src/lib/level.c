
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "draw.h"
#include "fixed-point.h"
#include "level.h"
#include "planet.h"
#include "player.h"
#include "image.h"
#include "texture/starscape.h"
#include "types.h"

#define MAX_PLANETS 5
#define MIN_PLANETS 3

#define LEVEL_FIX_SHIFT (FIX_SHIFT - 3)
#define LEVEL_FIX_OFFSET ((1 << FIX_SHIFT) >> 1)

enum level_flags {
	LEV_LIGHTING		= (1 << 0),
	LEV_NEED_REDRAW		= (1 << 1),
	LEV_ANIMATION		= (1 << 2),
	LEV_SCALE		= (1 << 3),
	LEV_SCALE_CHANGED	= (1 << 4),
	LEV_STRENGTH_CHANGED	= (1 << 5),
	LEV_NEED_REDRAW_FULL	= (1 << 6)
};

enum level_pos {
	LEV_X = 0,
	LEV_Y,
	LEV_X_SCALED,
	LEV_Y_SCALED,
	LEV_SIZE,
	LEV_SIZE_SCALED,
	LEV_LAST
};

enum state {
	LEVEL_START,
	TURN_GET_P1_INPUT,
	TURN_SHOW_P1,
	TURN_GET_P2_INPUT,
	TURN_SHOW_P2,
	LEVEL_WIN_P1,
	LEVEL_WIN_P2
};

struct projectile {
	peltar_fixed px;
	peltar_fixed py;
	int x;
	int y;
	int vector_x;
	int vector_y;
	uint32_t colour;
};

struct level {
	struct projectile proj;

	int nplanets;
	struct planet **planets;
	int planet_pos[MAX_PLANETS][LEV_LAST];
	int planet_mass[MAX_PLANETS];

	struct player *p1;
	struct player *p2;
	int player_pos[2][LEV_LAST];
	uint32_t p1_col;
	uint32_t p2_col;

	struct image *background;

	int width; /* Width of level */
	int height; /* Height of level */

	uint32_t flags; /* Flags */

	enum state state;

	enum state prev_render_state;
};

static inline void flag_set(uint32_t *flags, enum level_flags set_flags)
{
	*flags |= set_flags;
}

static inline void flag_unset(uint32_t *flags, enum level_flags unset_flags)
{
	*flags &= ~unset_flags;
}

static inline void flag_toggle(uint32_t *flags, enum level_flags toggle_flags)
{
	*flags ^= toggle_flags;
}

static inline bool flag_get(uint32_t flags, enum level_flags get_flags)
{
	return flags & get_flags;
}

static inline bool flag_get_all(uint32_t flags, enum level_flags get_all_flags)
{
	return get_all_flags == (flags & get_all_flags);
}


/* Coordinate conversion */
static inline void level_fixed_to_level(peltar_fixed fx, peltar_fixed fy,
			int *x, int *y)
{
	*x = (fx - LEVEL_FIX_OFFSET) >> LEVEL_FIX_SHIFT;
	*y = (fy - LEVEL_FIX_OFFSET) >> LEVEL_FIX_SHIFT;
}


/* Coordinate conversion */
static inline void level_level_to_fixed(int x, int y,
		peltar_fixed *fx, peltar_fixed *fy)
{
	*fx = LEVEL_FIX_OFFSET + (x << LEVEL_FIX_SHIFT);
	*fy = LEVEL_FIX_OFFSET + (y << LEVEL_FIX_SHIFT);
}


/* Coordinate conversion */
static inline void level_level_to_screen(struct level *l,
		int lx, int ly, int *x, int *y)
{
	/* Transform level coords to screen coords */
	*x = lx - 3 * l->width / 2;
	*y = ly - 3 * l->height / 2;
}


/* Coordinate conversion */
static inline void level_screen_to_level(struct level *l,
		int x, int y, int *lx, int *ly)
{
	/* Transform screen coords to level coords */
	*lx = 3 * l->width / 2 + x;
	*ly = 3 * l->height / 2 + y;
}


/* Coordinate conversion */
static inline void level_level_to_screen_scaled(
		int lx, int ly, int *x, int *y)
{
	/* Transform level coords to screen coords */
	*x = lx / 4;
	*y = ly / 4;
}


/* Coordinate conversion */
static inline void level_screen_scaled_to_level(int x, int y,
		int *lx, int *ly)
{
	/* Transform scaled screen coords to level coords */
	*lx = x * 4;
	*ly = y * 4;
}


void level_init(void)
{
	planet_init();
}


static void level_p1_fire(struct level *l)
{
	int x, y;
	player_get_target(l->p1, &l->proj.x, &l->proj.y,
			&l->proj.vector_x, &l->proj.vector_y);
	l->proj.colour = player_get_colour(l->p1);

	if (flag_get(l->flags, LEV_SCALE)) {
		level_screen_scaled_to_level(l->proj.x, l->proj.y, &x, &y);
	} else {
		level_screen_to_level(l, l->proj.x, l->proj.y, &x, &y);
	}

	level_level_to_fixed(x, y, &(l->proj.px), &(l->proj.py));

	l->proj.vector_x *= 16 + player_get_strength(l->p1);
	l->proj.vector_y *= 16 + player_get_strength(l->p1);

	if (flag_get(l->flags, LEV_SCALE)) {
		/* Scale strength */
		l->proj.vector_x *= 4;
		l->proj.vector_y *= 4;
	}

	return;
}


static void level_p2_fire(struct level *l)
{
	int x, y;
	player_get_target(l->p2, &l->proj.x, &l->proj.y,
			&l->proj.vector_x, &l->proj.vector_y);
	l->proj.colour = player_get_colour(l->p2);

	if (flag_get(l->flags, LEV_SCALE)) {
		level_screen_scaled_to_level(l->proj.x, l->proj.y, &x, &y);
	} else {
		level_screen_to_level(l, l->proj.x, l->proj.y, &x, &y);
	}

	level_level_to_fixed(x, y, &l->proj.px, &l->proj.py);

	l->proj.vector_x *= 16 + player_get_strength(l->p2);
	l->proj.vector_y *= 16 + player_get_strength(l->p2);

	if (flag_get(l->flags, LEV_SCALE)) {
		/* Scale strength */
		l->proj.vector_x *= 4;
		l->proj.vector_y *= 4;
	}

	return;
}


static void level_set_state(struct level *l, enum state s)
{
	switch (s) {
	case LEVEL_START:
		break;

	case TURN_GET_P1_INPUT:
		assert(l->state == LEVEL_START ||
				l->state == TURN_SHOW_P2);

		player_set_mouse_pos_to_target(l->p1);
		player_show_direction(l->p1, true);
		break;

	case TURN_SHOW_P1:
		assert(l->state == TURN_GET_P1_INPUT);
		flag_set(&l->flags, LEV_STRENGTH_CHANGED);
		player_show_direction(l->p1, false);
		level_p1_fire(l);
		break;

	case TURN_GET_P2_INPUT:
		assert(l->state == TURN_SHOW_P1);

		player_set_mouse_pos_to_target(l->p2);
		player_show_direction(l->p2, true);
		break;

	case TURN_SHOW_P2:
		assert(l->state == TURN_GET_P2_INPUT);
		flag_set(&l->flags, LEV_STRENGTH_CHANGED);
		player_show_direction(l->p2, false);
		level_p2_fire(l);
		break;

	case LEVEL_WIN_P1:
	case LEVEL_WIN_P2:
		assert(l->state == TURN_SHOW_P1 ||
				l->state == TURN_SHOW_P2);
		//TODO: Something about showing highscores / game menu screen?
		break;
	}

	l->state = s;
}


void level_begin(struct level *l)
{
	level_set_state(l, TURN_GET_P1_INPUT);
}


int level_get_winner(struct level *l)
{
	switch (l->state) {
	case LEVEL_WIN_P1:
		return 1;
	case LEVEL_WIN_P2:
		return 2;
	default:
		return -1;
	}
}


static bool level_too_close(int x1, int y1, int r1,
		int x2, int y2, int r2)
{
	int x = x2 - x1;
	int y = y2 - y1;
	int required_dist = (22 * (r1 + r2)) / 16;

	/* Compare distance with 3/16 extra slack, to ensure gaps between
	 * circles */
	if (((x * x) + (y * y)) > (required_dist * required_dist))
		/* Circles are not too close */
		return false;

	/* Circles are too close */
	return true;
}

int compare_int(const void *a, const void *b)
{
	return *(int *)b - *(int *)a;
}

static void level_set_scaled_pos(int pos[LEV_LAST], int width, int height)
{
	pos[LEV_X_SCALED] = (pos[LEV_X] + pos[LEV_SIZE] / 2) / 4 +
			(width - width / 4) / 2 - pos[LEV_SIZE_SCALED] / 2;
	pos[LEV_Y_SCALED] = (pos[LEV_Y] + pos[LEV_SIZE] / 2) / 4 +
			(height - height / 4) / 2 - pos[LEV_SIZE_SCALED] / 2;
}

void level_arrange_planets(int sizes[MAX_PLANETS], int size,
		struct level *level, int width, int height)
{
	int i, j;
	bool ok;

	i = 0;
	while (i < level->nplanets) {
		/* place planet */
		level->planet_pos[i][0] = sizes[i] / 2 +  2 * width / 16 +
				rand() % (12 * width / 16 - sizes[i]);
		level->planet_pos[i][1] = sizes[i] / 2 + height / 16 +
				rand() % (14 * height / 16 - sizes[i]);

		ok = true;
		/* Check placed planets */
		for (j = 0; j < i; j++) {
			if (level_too_close(level->planet_pos[i][LEV_X],
					level->planet_pos[i][LEV_Y],
					sizes[i] / 2,
					level->planet_pos[j][LEV_X],
					level->planet_pos[j][LEV_Y],
					sizes[j] / 2)) {
				ok = false;
				break;
			}
		}
		/* Check players */
		if (level_too_close(level->planet_pos[i][LEV_X],
				level->planet_pos[i][LEV_Y],
				sizes[i] / 2,
				level->player_pos[0][LEV_X] + size / 2,
				level->player_pos[0][LEV_Y] + size / 2,
				size / 2) ||
		    level_too_close(level->planet_pos[i][LEV_X],
				level->planet_pos[i][LEV_Y],
				sizes[i] / 2,
				level->player_pos[1][LEV_X] + size / 2,
				level->player_pos[1][LEV_Y] + size / 2,
				size / 2)) {
			ok = false;
		}
		if (ok)
			/* Next planet */
			i++;
	}

	for (i = 0; i < level->nplanets; i++) {
		level->planet_pos[i][0] -= sizes[i] / 2;
		level->planet_pos[i][1] -= sizes[i] / 2;
	}
}


static bool level_player_setup(struct player *p, int pos[LEV_LAST], int size,
		struct level *l)
{
	if (!player_setup_graphics(p, size)) {
		return false;
	}

	pos[LEV_SIZE] = player_get_size(p);
	pos[LEV_SIZE_SCALED] = player_get_size_scaled(p);

	level_set_scaled_pos(pos, l->width, l->height);

	player_set_pos(p, pos[LEV_X], pos[LEV_Y],
			pos[LEV_X_SCALED], pos[LEV_Y_SCALED]);

	player_set_lighting(p, flag_get(l->flags, LEV_LIGHTING));

	return true;
}


static bool level_create_details(struct level *level, int width, int height)
{
	int i;
	int sizes[MAX_PLANETS]; /* max no of planets */
	int total_diameter;
	int total = 0;
	int min_size = (((width + height) / 2) / 8) / 4;
	int player_size = 6 * (min_size * 4) / 8;
	player_size += 4 - (player_size % 4);

	level->width = width;
	level->height = height;

	/* Get background starscape */
	if (!image_create(&level->background, width, height)) {
		return false;
	}

	/* Player 1 */
	level->player_pos[0][LEV_X] = 1 * player_size / 4;
	level->player_pos[0][LEV_Y] = height / 2 - player_size / 2;

	if (!level_player_setup(level->p1, level->player_pos[0],
			player_size, level)) {
		return false;
	}
	player_set_target(level->p1, width / 2, height / 2);

	/* Player 2 */
	level->player_pos[1][LEV_X] = width - 5 * player_size / 4;
	level->player_pos[1][LEV_Y] = height / 2 - player_size / 2;

	if (!level_player_setup(level->p2, level->player_pos[1],
			player_size, level)) {
		return false;
	}
	player_set_target(level->p2, width / 2, height / 2);

	/*
	 * Create random number of randomly sized planets in random places
	 */

	/* Find a total for all planet diameters */
	total_diameter = (7 * ((width + height) / 2) / 8) / 8;

	/* Number of planets */
	level->nplanets = MIN_PLANETS + rand() %
			(MAX_PLANETS - MIN_PLANETS + 1);

	total_diameter -= ((16 + MIN_PLANETS - level->nplanets) *
			level->nplanets * min_size) / 16;

	for (i = 0; i < level->nplanets; i++) {
		sizes[i] = rand() % 64;
		total += sizes[i];
	}

	/* Find planet sizes */
	for (i = 0; i < level->nplanets; i++) {
		sizes[i] = ((total_diameter * sizes[i]) / total +
				min_size) * 4;
	}

	/* Sort sizes */
	qsort(sizes, level->nplanets, sizeof(int), compare_int);

	/* Get planet coords */
	level_arrange_planets(sizes, player_size, level, width, height);

	level->planets = malloc(level->nplanets * sizeof(struct planet*));
	if (level->planets == NULL)
		return false;

	for (i = 0; i < level->nplanets; i++) {
		if (!planet_create(&level->planets[i], sizes[i])) {
			level->nplanets = i;
			return false;
		}
		
		if (!planet_generate_texture(level->planets[i])) {
			level->nplanets = i;
			return false;
		}

		planet_set_lighting(level->planets[i],
				flag_get(level->flags, LEV_LIGHTING));

		level->planet_pos[i][LEV_SIZE] =
				planet_get_size(level->planets[i]);
		level->planet_pos[i][LEV_SIZE_SCALED] =
				planet_get_size_scaled(level->planets[i]);
		level->planet_mass[i] = planet_get_mass(level->planets[i]);

		level_set_scaled_pos(level->planet_pos[i],
				level->width, level->height);
	}

	return true;
}


void level_free(struct level *level)
{
	int i;
	assert(level != NULL);

	if (level->planets != NULL) {
		for (i = 0; i < level->nplanets; i++)
			planet_free(level->planets[i]);
		free(level->planets);
	}

	if (level->background != NULL)
		image_free(level->background);

	free(level);
}


bool level_create(struct level **level, struct player *p1, struct player *p2,
		int width, int height)
{
	*level = malloc(sizeof(struct level));
	if (*level == NULL)
		return false;

	(*level)->p1 = p1;
	(*level)->p2 = p2;

	(*level)->p1_col = player_get_colour(p1);
	(*level)->p2_col = player_get_colour(p2);

	(*level)->planets = NULL;
	(*level)->background = NULL;
	(*level)->nplanets = 0;
	(*level)->state = LEVEL_START;

	(*level)->flags = LEV_LIGHTING | LEV_ANIMATION;

	if (!level_create_details(*level, width, height)) {
		level_free(*level);
		return false;
	}

	return true;
}


static inline void level_remove_object(SDL_Surface *screen, SDL_Surface *bg,
		SDL_Rect *rect1, SDL_Rect *rect2, int pos[LEV_LAST])
{
	rect1->x = rect2->x = pos[LEV_X];
	rect1->y = rect2->y = pos[LEV_Y];
	rect2->w = rect2->h = pos[LEV_SIZE];

	SDL_BlitSurface(bg, rect2, screen, rect1);
}


static inline void level_remove_object_scaled(SDL_Surface *screen,
		SDL_Surface *bg, SDL_Rect *rect1, SDL_Rect *rect2,
		int pos[LEV_LAST])
{
	rect1->x = rect2->x = pos[LEV_X_SCALED];
	rect1->y = rect2->y = pos[LEV_Y_SCALED];
	rect2->w = rect2->h = pos[LEV_SIZE_SCALED];

	SDL_BlitSurface(bg, rect2, screen, rect1);
}


static inline void level_remove_box(SDL_Surface *screen, SDL_Surface *bg,
		int x, int y, int w, int h)
{
	SDL_Rect rect1 = {
		.w = 0,
		.h = 0
	};
	SDL_Rect rect2;

	rect1.x = rect2.x = x;
	rect1.y = rect2.y = y;
	rect2.w = w;
	rect2.h = 1;

	SDL_BlitSurface(bg, &rect2, screen, &rect1);
	rect1.y = rect2.y = y + h;
	SDL_BlitSurface(bg, &rect2, screen, &rect1);

	rect1.x = rect2.x = x;
	rect1.y = rect2.y = y;
	rect2.w = 1;
	rect2.h = h;

	SDL_BlitSurface(bg, &rect2, screen, &rect1);
	rect1.x = rect2.x = x + w;
	SDL_BlitSurface(bg, &rect2, screen, &rect1);
}


static void level_update_render_scale_clearance(
		struct level *l, SDL_Surface *screen)
{
	int i;
	SDL_Surface *bg = image_get_surface(l->background);
	SDL_Rect rect1 = {
		.x = 0,
		.y = 0,
		.w = 0,
		.h = 0
	};
	SDL_Rect rect2;

	if (flag_get(l->flags, LEV_SCALE)) {
		/* Changed to scaled out view */
		for (i = 0; i < l->nplanets; i++) {
			level_remove_object(screen, bg,
					&rect1, &rect2, l->planet_pos[i]);
		}
		level_remove_object(screen, bg,
				&rect1, &rect2, l->player_pos[0]);

		level_remove_object(screen, bg,
				&rect1, &rect2, l->player_pos[1]);

		draw_box(screen, 3 * l->width / 8, 3 * l->height / 8,
				l->width / 4, l->height / 4, 0x00ffffff);
	} else {
		/* Changed to normal view */
		for (i = 0; i < l->nplanets; i++) {
			level_remove_object_scaled(screen, bg,
					&rect1, &rect2, l->planet_pos[i]);
		}
		level_remove_object_scaled(screen, bg,
				&rect1, &rect2, l->player_pos[0]);

		level_remove_object_scaled(screen, bg,
				&rect1, &rect2, l->player_pos[1]);

		level_remove_box(screen, bg,
				3 * l->width / 8, 3 * l->height / 8,
				l->width / 4, l->height / 4);
	}
}


static void level_update_render_turn_borders(
		struct level *l, SDL_Surface *screen)
{
	switch (l->state) {
	case TURN_GET_P1_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1, l->p1_col);
		return;

	case TURN_GET_P2_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1, l->p2_col);
		return;

	default:
		break;
	}

	switch (l->prev_render_state) {
	case TURN_GET_P1_INPUT:
	case TURN_GET_P2_INPUT:
		level_remove_box(screen, image_get_surface(l->background),
				0, 0, l->width - 1, l->height - 1);
		return;

	default:
		break;
	}
}


/*
 * Find the gravity vector at a certain point on the level
 *
 * l	the level
 * px	x-coordinate of required point
 * py	y-coordinate of required point
 * vx	updated to x value of gravity vector
 * vy	updated to y value of gravity vector
 * return true iff projectile hit a planet
 *
 * TODO: Optimise
 */
static inline bool level_get_gravity_vector_at_point(struct level *l,
		peltar_fixed px, peltar_fixed py, int *vx, int *vy)
{
	int i;
	int x = 0;
	int y = 0;
	int point_lx, point_ly;

	level_fixed_to_level(px, py, &point_lx, &point_ly);

	for (i = 0; i < l->nplanets; i++) {
		int planet_lx, planet_ly;
		int distance_x, distance_y, distance2;
		int64_t mass = l->planet_mass[i];
		int distance;
		int a;

		level_screen_scaled_to_level(
				l->planet_pos[i][LEV_X_SCALED] +
					l->planet_pos[i][LEV_SIZE_SCALED] / 2,
				l->planet_pos[i][LEV_Y_SCALED] +
					l->planet_pos[i][LEV_SIZE_SCALED] / 2,
				&planet_lx, &planet_ly);

		distance_x = (planet_lx - point_lx);
		distance_y = (planet_ly - point_ly);

		distance2 = distance_x * distance_x + distance_y * distance_y;
		distance2 = (distance2 == 0) ? 1 : distance2;

		//TODO: avoid floating point / sqrt
		distance = sqrt(distance2);

		if (distance <= l->planet_pos[i][LEV_SIZE] / 2)
			return true;

		a = (mass << LEVEL_FIX_SHIFT) / distance2;
		x += a * distance_x / distance;
		y += a * distance_y / distance;
	}

	*vx = x;
	*vy = y;

	return false;
}


static bool level_has_hit_player(struct level *l, int pos[LEV_LAST])
{
	int r = pos[LEV_SIZE] / 2;
	int x = l->proj.x - (pos[LEV_X] + r);
	int y = l->proj.y - (pos[LEV_Y] + r);

	if ((x * x) + (y * y) < (r * r))
		return true;

	return false;
}


static void level_update_projectile(struct level *l, SDL_Surface *screen)
{
	int proj_pos_x, proj_pos_y;
	int grav_x, grav_y;
	peltar_fixed prev_x, prev_y;

	if (l->prev_render_state == TURN_SHOW_P1 ||
			l->prev_render_state == TURN_SHOW_P2) {
		/* Remove shot from prev. frame */
		SDL_Surface *bg = image_get_surface(l->background);
		SDL_Rect rect1 = {
			.x = l->proj.x - 1,
			.y = l->proj.y - 1,
			.w = 0,
			.h = 0
		};
		SDL_Rect rect2 = {
			.x = l->proj.x - 1,
			.y = l->proj.y - 1,
			.w = 3,
			.h = 3
		};
		SDL_BlitSurface(bg, &rect2, screen, &rect1);

		if (l->state != TURN_SHOW_P1 && l->state != TURN_SHOW_P2)
			/* Nothing else to do */
			return;
	}

	prev_x = l->proj.px;
	prev_y = l->proj.py;

	/* Work out new shot position */
	if (level_get_gravity_vector_at_point(l,
			l->proj.px, l->proj.py, &grav_x, &grav_y)) {
		/* Last frame we hit a planet! */
		if (l->state == TURN_SHOW_P1) {
			level_set_state(l, TURN_GET_P2_INPUT);

		} else {
			level_set_state(l, TURN_GET_P1_INPUT);
		}

	} else {
		int min_x, min_y, max_x, max_y;
		int min_x_scaled, min_y_scaled, max_x_scaled, max_y_scaled;

		/* Update projectile position */
		l->proj.px += grav_x / 16 + l->proj.vector_x;
		l->proj.py += grav_y / 16 + l->proj.vector_y;
		level_fixed_to_level(l->proj.px, l->proj.py,
				&proj_pos_x, &proj_pos_y);

		l->proj.vector_x = l->proj.px - prev_x;
		l->proj.vector_y = l->proj.py - prev_y;

		/* Get projectile bounds */
		level_screen_to_level(l, 1, 1, &min_x, &min_y);
		level_screen_to_level(l, l->width - 1, l->height - 1,
				&max_x, &max_y);

		level_screen_scaled_to_level(1, 1,
				&min_x_scaled, &min_y_scaled);
		level_screen_scaled_to_level(l->width - 1, l->height - 1,
				&max_x_scaled, &max_y_scaled);

		/* Check whether projectile is within bounds */
		if (proj_pos_x > min_x && proj_pos_x < max_x &&
				proj_pos_y > min_y && proj_pos_y < max_y) {
			/* Within full scale area; ensure not scaled view */
			if (flag_get(l->flags, LEV_SCALE)) {
				/* Need to toggled to unscaled */
				flag_toggle(&l->flags, LEV_SCALE);
				/* Handle clearance to background for scale
				 * change */
				level_update_render_scale_clearance(l, screen);
			}
		} else if (proj_pos_x > min_x_scaled &&
				proj_pos_x < max_x_scaled &&
				proj_pos_y > min_y_scaled &&
				proj_pos_y < max_y_scaled) {
			/* Within full scale area; ensure scaled view */
			if (!flag_get(l->flags, LEV_SCALE)) {
				/* Need to toggled to scaled */
				flag_toggle(&l->flags, LEV_SCALE);
				/* Handle clearance to background for scale
				 * change */
				level_update_render_scale_clearance(l, screen);
			}
		} else {
			/* Out of bounds; end turn */
			if (l->state == TURN_SHOW_P1) {
				level_set_state(l, TURN_GET_P2_INPUT);

			} else {
				level_set_state(l, TURN_GET_P1_INPUT);
			}

			/* Return to unscaled view */
			if (flag_get(l->flags, LEV_SCALE)) {
				/* Need to toggled to unscaled */
				flag_toggle(&l->flags, LEV_SCALE);
				/* Handle clearance to background for scale
				 * change */
				level_update_render_scale_clearance(l, screen);
			}
			return;
		}

		if (flag_get(l->flags, LEV_SCALE))
			level_level_to_screen_scaled(proj_pos_x, proj_pos_y,
					&l->proj.x, &l->proj.y);
		else
			level_level_to_screen(l, proj_pos_x, proj_pos_y,
					&l->proj.x, &l->proj.y);

		/* Render shot for this frame */
		draw_shot_3x3(screen, l->proj.x, l->proj.y, l->proj.colour);

		/* If scaled, can't hit planet, so escape */
		if (flag_get(l->flags, LEV_SCALE))
			return;

		if (level_has_hit_player(l, l->player_pos[0])) {
			/* Hit player 1 */
			level_set_state(l, LEVEL_WIN_P2);
			return;
		}

		if (level_has_hit_player(l, l->player_pos[1])) {
			/* Hit player 2 */
			level_set_state(l, LEVEL_WIN_P1);
			return;
		}
	}

	return;
}


bool level_update_render(struct level *l, SDL_Surface *screen)
{
	int i;
	SDL_Surface *bg = image_get_surface(l->background);

	if (flag_get(l->flags, LEV_NEED_REDRAW_FULL)) {
		SDL_BlitSurface(image_get_surface(l->background),
				NULL, screen, NULL);
		flag_toggle(&l->flags, LEV_NEED_REDRAW_FULL);
	}

	/* Handle clearance to background for scale change */
	if (flag_get(l->flags, LEV_SCALE_CHANGED)) {
		level_update_render_scale_clearance(l, screen);
	}

	if (l->state == TURN_SHOW_P1 || l->state == TURN_SHOW_P2 ||
			l->prev_render_state == TURN_SHOW_P1 ||
			l->prev_render_state == TURN_SHOW_P2) {
		level_update_projectile(l, screen);
	}

	/* Handle change of turn border indicator change */
	if (l->state != l->prev_render_state) {
		level_update_render_turn_borders(l, screen);
	}

	if ((l->state == TURN_GET_P1_INPUT || l->state == TURN_GET_P2_INPUT) &&
			(flag_get(l->flags, LEV_STRENGTH_CHANGED) ||
			(l->prev_render_state != TURN_GET_P1_INPUT ||
			 l->prev_render_state != TURN_GET_P2_INPUT))) {
		int width = l->width / 2;
		int height = l->height / 64;

		if (l->state == TURN_GET_P1_INPUT)
			player_render_strength(l->p1, screen,
					l->width / 4,
					l->height / 64, width, height);
		else
			player_render_strength(l->p2, screen,
					l->width / 4,
					l->height / 64, width, height);

	} else if (flag_get(l->flags, LEV_STRENGTH_CHANGED) &&
			l->state != TURN_GET_P1_INPUT &&
			l->state != TURN_GET_P2_INPUT) {
		int width = l->width / 2;
		int height = l->height / 64;
		SDL_Surface *bg = image_get_surface(l->background);
		SDL_Rect rect1 = {
			.x = l->width / 4,
			.y = l->height / 64,
			.w = 0,
			.h = 0
		};
		SDL_Rect rect2 = {
			.x = l->width / 4,
			.y = l->height / 64,
			.w = width,
			.h = height
		};
		SDL_BlitSurface(bg, &rect2, screen, &rect1);
	}

	if (!flag_get(l->flags, LEV_ANIMATION | LEV_SCALE_CHANGED |
			LEV_NEED_REDRAW)) {
		/* Not showing animations */

		if (flag_get(l->flags, LEV_SCALE)) {
			player_render_direction_scaled(l->p1, screen, bg);
			player_render_direction_scaled(l->p2, screen, bg);
		} else {
			player_render_direction(l->p1, screen, bg);
			player_render_direction(l->p2, screen, bg);
		}

		l->prev_render_state = l->state;

		if (l->state == LEVEL_WIN_P1 || l->state == LEVEL_WIN_P2) {
			return true;
		}

		return false;
	}

	if (flag_get(l->flags, LEV_SCALE)) {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render_scaled(l->planets[i], screen,
					l->planet_pos[i][LEV_X_SCALED],
					l->planet_pos[i][LEV_Y_SCALED]);
		}
		player_update_render_scaled(l->p1, screen, bg);
		player_update_render_scaled(l->p2, screen, bg);
	} else {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render(l->planets[i], screen,
					l->planet_pos[i][LEV_X],
					l->planet_pos[i][LEV_Y]);
		}
		player_update_render(l->p1, screen, bg);
		player_update_render(l->p2, screen, bg);
	}

	if (flag_get(l->flags, LEV_SCALE_CHANGED))
		flag_toggle(&l->flags, LEV_SCALE_CHANGED);

	if (flag_get(l->flags, LEV_NEED_REDRAW))
		flag_toggle(&l->flags, LEV_NEED_REDRAW);

	l->prev_render_state = l->state;

	if (l->state == LEVEL_WIN_P1 || l->state == LEVEL_WIN_P2) {
		return true;
	}

	return false;
}


bool level_update_render_full(struct level *l, SDL_Surface *screen)
{
	int i;
	SDL_Surface *bg = image_get_surface(l->background);

	SDL_BlitSurface(image_get_surface(l->background),
			NULL, screen, NULL);

	switch (l->state) {
	case TURN_GET_P1_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1, l->p1_col);
		break;

	case TURN_GET_P2_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1, l->p2_col);
		break;

	default:
		break;
	}

	if (flag_get(l->flags, LEV_SCALE)) {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render_scaled(l->planets[i], screen,
					l->planet_pos[i][LEV_X_SCALED],
					l->planet_pos[i][LEV_Y_SCALED]);
		}
		player_update_render_scaled(l->p1, screen, bg);
		player_update_render_scaled(l->p2, screen, bg);
	} else {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render(l->planets[i], screen,
					l->planet_pos[i][LEV_X],
					l->planet_pos[i][LEV_Y]);
		}
		player_update_render(l->p1, screen, bg);
		player_update_render(l->p2, screen, bg);
	}

	l->prev_render_state = l->state;

	if (l->state == LEVEL_WIN_P1 || l->state == LEVEL_WIN_P2) {
		return true;
	}

	return false;
}


bool level_handle_key(struct level *l, SDL_Event *event)
{
	int i;
	assert(event->type == SDL_KEYDOWN);

	switch (event->key.keysym.sym) {
	case SDLK_l:
		/* 'L' key: toggle planet lighting */
		flag_toggle(&l->flags, LEV_LIGHTING);

		for (i = 0; i < l->nplanets; i++) {
			planet_set_lighting(l->planets[i],
					flag_get(l->flags, LEV_LIGHTING));
		}
		player_set_lighting(l->p1, flag_get(l->flags, LEV_LIGHTING));
		player_set_lighting(l->p2, flag_get(l->flags, LEV_LIGHTING));

		flag_set(&l->flags, LEV_NEED_REDRAW);
		/* Handled keypress */
		return true;

	case SDLK_a:
		/* 'A' key: toggle background animations */
		flag_toggle(&l->flags, LEV_ANIMATION);

		/* Handled keypress */
		return true;

	case SDLK_z:
		/* 'Z' key: toggle zoom */
		flag_toggle(&l->flags, LEV_SCALE | LEV_SCALE_CHANGED);

		/* Handled keypress */
		return true;

	case SDLK_t:
		/* 'T' key: redo planet textures */
		for (i = 0; i < l->nplanets; i++) {
			if (!planet_generate_texture(l->planets[i])) {
				SDL_Quit();
				exit(EXIT_FAILURE);
			}
		}
		flag_set(&l->flags, LEV_NEED_REDRAW);
		/* Handled keypress */
		return true;

	case SDLK_b:
		/* 'B' key: redo background starscape */
		if (!texture_get_starscape(l->background)) {
			SDL_Quit();
			return EXIT_FAILURE;
		}
		flag_set(&l->flags, LEV_NEED_REDRAW_FULL);
		/* Handled keypress */
		return true;

	default:
		break;
	}

	/* Key not handled */
	return false;
}


bool level_handle_mouse(struct level *l, SDL_Event *event)
{
	assert(event->type == SDL_MOUSEMOTION ||
			event->type == SDL_MOUSEBUTTONDOWN ||
			event->type == SDL_MOUSEBUTTONUP);

	switch (event->type) {
	case SDL_MOUSEMOTION:
		if (l->state == TURN_GET_P1_INPUT) {
			player_set_target(l->p1,
					event->motion.x, event->motion.y);

		} else if (l->state == TURN_GET_P2_INPUT) {
			player_set_target(l->p2,
					event->motion.x, event->motion.y);
		}

		return true;

	case SDL_MOUSEBUTTONDOWN:
		if (event->button.button == SDL_BUTTON_LEFT) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_set_target(l->p1,
						event->button.x,
						event->button.y);
				level_set_state(l, TURN_SHOW_P1);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_set_target(l->p2,
						event->button.x,
						event->button.y);
				level_set_state(l, TURN_SHOW_P2);
			}
		}

		if (event->button.button == SDL_BUTTON_WHEELUP) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_increase_strength(l->p1);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_increase_strength(l->p2);
			}
		}

		if (event->button.button == SDL_BUTTON_WHEELDOWN) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_reduce_strength(l->p1);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_reduce_strength(l->p2);
			}
		}

		break;

	default:
		break;
	}

	/* Mouseage not handled */
	return false;
}
