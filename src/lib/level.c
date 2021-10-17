
#include <assert.h>
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
#include "trail.h"
#include "types.h"
#include "util.h"

#define PLANETS_MAX 5
#define PLANETS_MIN 3

#define LEVEL_FIX_SHIFT (FIX_SHIFT - 3)
#define LEVEL_FIX_OFFSET ((1 << FIX_SHIFT) >> 1)

enum players {
	PLAYERS_1,
	PLAYERS_2,
	PLAYERS_MAX,
};

enum level_scale {
	NORMAL,
	SCALED,
	SCALE_COUNT,
};

enum level_flags {
	LEV_LIGHTING		= (1 << 0),
	LEV_NEED_REDRAW		= (1 << 1),
	LEV_ANIMATION		= (1 << 2),
	LEV_STRENGTH_CHANGED	= (1 << 3),
	LEV_NEED_REDRAW_FULL	= (1 << 4)
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

struct point {
	int x;
	int y;
};

struct projectile {
	peltar_fixed px;
	peltar_fixed py;
	struct point screen[SCALE_COUNT];
	int vector_x;
	int vector_y;
	uint32_t colour;
	enum level_scale scale;
	int count;
};

struct asset_pos {
	int x;
	int y;
	int size;
};

struct level {
	struct projectile proj;

	int nplanets;
	struct planet **planets;
	int planet_mass[PLANETS_MAX];

	struct asset_pos planet[PLANETS_MAX][SCALE_COUNT];
	struct asset_pos player[PLAYERS_MAX][SCALE_COUNT];

	struct player *p[PLAYERS_MAX];
	uint32_t colour[PLAYERS_MAX];

	trail_t *trails[SCALE_COUNT];

	struct point min[SCALE_COUNT];
	struct point max[SCALE_COUNT];

	struct image *background[SCALE_COUNT];

	int width; /* Width of level */
	int height; /* Height of level */

	uint32_t flags; /* Flags */

	enum state state;
	enum level_scale scale;

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

static inline enum level_scale level_get_scale(const struct level *l)
{
	return l->scale;
}

static inline struct image *level_get_bg(const struct level *l)
{
	return l->background[level_get_scale(l)];
}

static inline SDL_Surface *level_get_bg_surface(const struct level *l)
{
	return image_get_surface(level_get_bg(l));
}

/* Coordinate conversion */
static inline void level_fixed_to_level(peltar_fixed fx, peltar_fixed fy,
			struct point *l)
{
	l->x = (fx - LEVEL_FIX_OFFSET) >> LEVEL_FIX_SHIFT;
	l->y = (fy - LEVEL_FIX_OFFSET) >> LEVEL_FIX_SHIFT;
}

/* Coordinate conversion */
static inline void level_level_to_fixed(int x, int y,
		peltar_fixed *fx, peltar_fixed *fy)
{
	*fx = LEVEL_FIX_OFFSET + (x << LEVEL_FIX_SHIFT);
	*fy = LEVEL_FIX_OFFSET + (y << LEVEL_FIX_SHIFT);
}

/* Coordinate conversion */
static inline void level_level_to_screen(struct level *level,
		const struct point *l, struct point *s)
{
	/* Transform level coords to screen coords */
	s->x = l->x - 3 * level->width / 2;
	s->y = l->y - 3 * level->height / 2;
}

/* Coordinate conversion */
static inline void level_screen_to_level(struct level *level,
		const struct point *s, struct point *l)
{
	/* Transform screen coords to level coords */
	l->x = 3 * level->width / 2 + s->x;
	l->y = 3 * level->height / 2 + s->y;
}

/* Coordinate conversion */
static inline void level_level_to_screen_scaled(
		const struct point *l, struct point *s)
{
	/* Transform level coords to screen coords */
	s->x = l->x / 4;
	s->y = l->y / 4;
}

/* Coordinate conversion */
static inline void level_screen_scaled_to_level(
		const struct point *s, struct point *l)
{
	/* Transform scaled screen coords to level coords */
	l->x = s->x * 4;
	l->y = s->y * 4;
}

void level_init(void)
{
	planet_init();
}

static void level_player_fire(struct level *level, int player)
{
	struct point l;
	player_get_target(level->p[player],
			&level->proj.screen[NORMAL].x,
			&level->proj.screen[NORMAL].y,
			&level->proj.vector_x,
			&level->proj.vector_y);
	level->proj.colour = level->colour[player];
	level->proj.count = 0;

	if (level->scale == SCALED) {
		level_screen_scaled_to_level(&level->proj.screen[SCALED], &l);
	} else {
		level_screen_to_level(level, &level->proj.screen[NORMAL], &l);
	}

	level_level_to_fixed(l.x, l.y, &(level->proj.px), &(level->proj.py));

	level->proj.vector_x *= 16 + player_get_strength(level->p[player]);
	level->proj.vector_y *= 16 + player_get_strength(level->p[player]);

	if (level->scale == SCALED) {
		/* Scale strength */
		level->proj.vector_x *= 4;
		level->proj.vector_y *= 4;
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

		player_set_mouse_pos_to_target(l->p[PLAYERS_1]);
		player_show_direction(l->p[PLAYERS_1], true);
		break;

	case TURN_SHOW_P1:
		assert(l->state == TURN_GET_P1_INPUT);
		flag_set(&l->flags, LEV_STRENGTH_CHANGED);
		player_show_direction(l->p[PLAYERS_1], false);
		level_player_fire(l, PLAYERS_1);
		break;

	case TURN_GET_P2_INPUT:
		assert(l->state == TURN_SHOW_P1);

		player_set_mouse_pos_to_target(l->p[PLAYERS_2]);
		player_show_direction(l->p[PLAYERS_2], true);
		break;

	case TURN_SHOW_P2:
		assert(l->state == TURN_GET_P2_INPUT);
		flag_set(&l->flags, LEV_STRENGTH_CHANGED);
		player_show_direction(l->p[PLAYERS_2], false);
		level_player_fire(l, PLAYERS_2);
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

static void level_set_scaled_pos(
		const struct asset_pos *orig,
		struct asset_pos *scaled,
		int width, int height)
{
	scaled->x= (orig->x + orig->size / 2) / 4 + (width - width / 4) / 2 -
			scaled->size / 2;
	scaled->y = (orig->y + orig->size / 2) / 4 + (height - height / 4) / 2 -
			scaled->size / 2;
}

void level_arrange_planets(int sizes[PLANETS_MAX], int size,
		struct level *level, int width, int height)
{
	int i, j;
	bool ok;

	i = 0;
	while (i < level->nplanets) {
		/* place planet */
		level->planet[i][NORMAL].x = sizes[i] / 2 +  2 * width / 16 +
				rand() % (12 * width / 16 - sizes[i]);
		level->planet[i][NORMAL].y = sizes[i] / 2 + height / 16 +
				rand() % (14 * height / 16 - sizes[i]);

		ok = true;
		/* Check placed planets */
		for (j = 0; j < i; j++) {
			if (level_too_close(
					level->planet[i][NORMAL].x,
					level->planet[i][NORMAL].y,
					sizes[i] / 2,
					level->planet[j][NORMAL].x,
					level->planet[j][NORMAL].y,
					sizes[j] / 2)) {
				ok = false;
				break;
			}
		}
		/* Check players */
		if (level_too_close(
				level->planet[i][NORMAL].x,
				level->planet[i][NORMAL].y,
				sizes[i] / 2,
				level->player[0][NORMAL].x + size / 2,
				level->player[0][NORMAL].y + size / 2,
				size / 2) ||
		    level_too_close(
				level->planet[i][NORMAL].x,
				level->planet[i][NORMAL].y,
				sizes[i] / 2,
				level->player[1][NORMAL].x + size / 2,
				level->player[1][NORMAL].y + size / 2,
				size / 2)) {
			ok = false;
		}
		if (ok)
			/* Next planet */
			i++;
	}

	for (i = 0; i < level->nplanets; i++) {
		level->planet[i][NORMAL].x -= sizes[i] / 2;
		level->planet[i][NORMAL].y -= sizes[i] / 2;
	}
}

static bool level_player_setup(struct level *level,
		int width, int height, int player_size)
{
	level->player[PLAYERS_1][NORMAL].x = 1 * player_size / 4;
	level->player[PLAYERS_2][NORMAL].x = width - 5 * player_size / 4;

	for (int i = 0; i < PLAYERS_MAX; i++) {
		struct player *p = level->p[i];

		if (!player_setup_graphics(p, player_size)) {
			return false;
		}

		level->player[i][NORMAL].y = height / 2 - player_size / 2;
		level->player[i][NORMAL].size = player_get_size(p);
		level->player[i][SCALED].size = player_get_size_scaled(p);

		level_set_scaled_pos(
				&level->player[i][NORMAL],
				&level->player[i][SCALED],
				level->width, level->height);

		player_set_pos(p,
				level->player[i][NORMAL].x,
				level->player[i][NORMAL].y,
				level->player[i][SCALED].x,
				level->player[i][SCALED].y);

		player_set_lighting(p, flag_get(level->flags, LEV_LIGHTING));
		player_set_target(p, width / 2, height / 2);
	}

	return true;
}

static bool level_create_details(struct level *level, int width, int height)
{
	int i;
	int sizes[PLANETS_MAX]; /* max no of planets */
	int total_diameter;
	int total = 0;
	int min_size = (((width + height) / 2) / 8) / 4;
	int player_size = 6 * (min_size * 4) / 8;
	player_size += 4 - (player_size % 4);

	level->width = width;
	level->height = height;

	/* Get background starscape */
	if (!image_create(&level->background[NORMAL], width, height)) {
		return false;
	}

	/* Get background starscape */
	if (!image_clone(level->background[NORMAL],
			&level->background[SCALED])) {
		return false;
	}

	if (!level_player_setup(level, width, height, player_size)) {
		return false;
	}

	/*
	 * Create random number of randomly sized planets in random places
	 */

	/* Find a total for all planet diameters */
	total_diameter = (7 * ((width + height) / 2) / 8) / 8;

	/* Number of planets */
	level->nplanets = PLANETS_MIN + rand() %
			(PLANETS_MAX - PLANETS_MIN + 1);

	total_diameter -= ((16 + PLANETS_MIN - level->nplanets) *
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

		level->planet[i][NORMAL].size = planet_get_size(level->planets[i]);
		level->planet[i][SCALED].size = planet_get_size_scaled(level->planets[i]);
		level->planet_mass[i] = planet_get_mass(level->planets[i]);

		level_set_scaled_pos(
				&level->planet[i][NORMAL],
				&level->planet[i][SCALED],
				level->width, level->height);
	}

	return true;
}

static void level_destroy_trails(struct level *level)
{
	if (level != NULL) {
		trail_destroy(level->trails[NORMAL]);
		trail_destroy(level->trails[SCALED]);
	}
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

	if (level->background[NORMAL] != NULL) {
		image_free(level->background[NORMAL]);
	}

	if (level->background[SCALED] != NULL) {
		image_free(level->background[SCALED]);
	}

	level_destroy_trails(level);
	free(level);
}

static bool level_create_trails(struct level *level, int width, int height)
{
	level->trails[NORMAL] = trail_create(width, height);
	level->trails[SCALED] = trail_create(width, height);

	if (level->trails[NORMAL] == NULL ||
	    level->trails[SCALED] == NULL) {
		level_destroy_trails(level);
		return false;
	}

	return true;
}

static void level_setup_projectile_bounds(struct level *l)
{
		static const struct point min = {
			.x = 1,
			.y = 1,
		};
		struct point max = {
			.x = l->width - 1,
			.y = l->height - 1,
		};

		level_screen_to_level(l, &min, &l->min[NORMAL]);
		level_screen_to_level(l, &max, &l->max[NORMAL]);
		level_screen_scaled_to_level(&min, &l->min[SCALED]);
		level_screen_scaled_to_level(&max, &l->max[SCALED]);
}

bool level_create(struct level **level, struct player *p1, struct player *p2,
		int width, int height)
{
	*level = malloc(sizeof(struct level));
	if (*level == NULL)
		return false;

	(*level)->p[PLAYERS_1] = p1;
	(*level)->p[PLAYERS_2] = p2;

	(*level)->colour[PLAYERS_1] = player_get_colour(p1);
	(*level)->colour[PLAYERS_2] = player_get_colour(p2);

	(*level)->planets = NULL;
	(*level)->background[NORMAL] = NULL;
	(*level)->background[SCALED] = NULL;
	(*level)->nplanets = 0;
	(*level)->state = LEVEL_START;
	(*level)->scale = NORMAL;

	(*level)->flags = LEV_LIGHTING | LEV_ANIMATION;

	if (!level_create_details(*level, width, height)) {
		level_free(*level);
		return false;
	}

	if (!level_create_trails(*level, width, height)) {
		level_free(*level);
		return false;
	}

	level_setup_projectile_bounds(*level);

	return true;
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

static void level_render_whole_background(
		struct level *l, SDL_Surface *screen)
{
	SDL_Surface *bg = image_get_surface(l->background[l->scale]);

	SDL_BlitSurface(bg, NULL, screen, NULL);

	if (l->scale == SCALED) {
		draw_box(screen, 3 * l->width / 8, 3 * l->height / 8,
				l->width / 4, l->height / 4, 0x00ffffff);
	}
}

static void level_update_render_turn_borders(
		struct level *l, SDL_Surface *screen)
{
	switch (l->state) {
	case TURN_GET_P1_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1,
				l->colour[PLAYERS_1]);
		return;

	case TURN_GET_P2_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1,
				l->colour[PLAYERS_2]);
		return;

	default:
		break;
	}

	switch (l->prev_render_state) {
	case TURN_GET_P1_INPUT:
	case TURN_GET_P2_INPUT:
		level_remove_box(screen, level_get_bg_surface(l),
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
 */
static inline bool level_get_gravity_vector_at_point(struct level *l,
		peltar_fixed px, peltar_fixed py, int *vx, int *vy)
{
	int i;
	int x = 0;
	int y = 0;
	struct point point_l;

	level_fixed_to_level(px, py, &point_l);

	for (i = 0; i < l->nplanets; i++) {
		struct asset_pos *planet_pos = &l->planet[i][SCALED];
		int distance_x, distance_y;
		int64_t mass = l->planet_mass[i];
		int distance;
		int a;
		struct point centre_level;
		struct point centre = {
			.x = planet_pos->x + planet_pos->size / 2,
			.y = planet_pos->y + planet_pos->size / 2,
		};

		level_screen_scaled_to_level(&centre, &centre_level);

		distance_x = (centre_level.x - point_l.x);
		distance_y = (centre_level.y - point_l.y);

		distance = peltar_hypot(distance_x, distance_y);

		if (distance <= l->planet[i][NORMAL].size / 2)
			return true;

		a = (mass << LEVEL_FIX_SHIFT) / (distance * distance);
		x += a * distance_x / distance;
		y += a * distance_y / distance;
	}

	*vx = x;
	*vy = y;

	return false;
}

static bool level_has_hit_player(struct level *l, struct asset_pos *pos)
{
	int r = pos->size / 2;
	int x = l->proj.screen[NORMAL].x - (pos->x + r);
	int y = l->proj.screen[NORMAL].y - (pos->y + r);

	if ((x * x) + (y * y) < (r * r))
		return true;

	return false;
}

static void level_plot_bg_box(
		SDL_Surface *screen,
		SDL_Surface *bg,
		int x0, int y0,
		int x1, int y1)
{
	if (x0 > x1) {
		int temp = x0;
		x0 = x1;
		x1 = temp;
	}
	if (y0 > y1) {
		int temp = y0;
		y0 = y1;
		y1 = temp;
	}

	SDL_Rect r = {
		.x = x0,
		.y = y0,
		.w = x1 - x0 + 1,
		.h = y1 - y0 + 1,
	};

	SDL_BlitSurface(bg, &r, screen, &r);
}

static void level_remove_projectile(
		const struct level *l,
		const struct projectile *p,
		SDL_Surface *screen)
{
	if (l->prev_render_state == TURN_SHOW_P1 ||
	    l->prev_render_state == TURN_SHOW_P2) {
		SDL_Surface *bg = level_get_bg_surface(l);
		SDL_Rect rect = {
			.x = l->proj.screen[l->proj.scale].x - 1,
			.y = l->proj.screen[l->proj.scale].y - 1,
			.w = 3,
			.h = 3
		};
		SDL_BlitSurface(bg, &rect, screen, &rect);

		if (l->state != TURN_SHOW_P1 &&
		    l->state != TURN_SHOW_P2) {
			/* Nothing else to do */
			return;
		}
	}
}

static void level_end_turn(
		struct level *l,
		enum players player,
		SDL_Surface *screen)
{
	trial_render(l->trails[SCALED],
			image_get_surface(l->background[SCALED]),
			blend_colour(l->colour[player], 0));
	trail_clear(l->trails[SCALED]);
	trial_render(l->trails[NORMAL],
			image_get_surface(l->background[NORMAL]),
			blend_colour(l->colour[player], 0));
	trail_clear(l->trails[NORMAL]);

	if (player == PLAYERS_1) {
		level_set_state(l, TURN_GET_P2_INPUT);
	} else {
		level_set_state(l, TURN_GET_P1_INPUT);
	}

	level_render_whole_background(l, screen);
}

static void level_update_projectile(struct level *l, SDL_Surface *screen)
{
	struct point proj_pos;
	int grav_x, grav_y;
	peltar_fixed prev_x, prev_y;
	enum level_scale scale = l->scale;
	enum players player = (l->state == TURN_SHOW_P1) ?
			PLAYERS_1 : PLAYERS_2;

	level_remove_projectile(l, &l->proj, screen);

	prev_x = l->proj.px;
	prev_y = l->proj.py;
	struct point prev[SCALE_COUNT] = {
		[NORMAL] = l->proj.screen[NORMAL],
		[SCALED] = l->proj.screen[SCALED],
	};

	/* Work out new shot position */
	if (level_get_gravity_vector_at_point(l,
			l->proj.px, l->proj.py, &grav_x, &grav_y)) {
		/* Last frame we hit a planet! */
		level_end_turn(l, player, screen);

	} else {
		/* Update projectile position */
		l->proj.px += grav_x / 32 + l->proj.vector_x;
		l->proj.py += grav_y / 32 + l->proj.vector_y;
		level_fixed_to_level(l->proj.px, l->proj.py, &proj_pos);

		l->proj.vector_x = l->proj.px - prev_x;
		l->proj.vector_y = l->proj.py - prev_y;

		/* Check whether projectile is within bounds */
		if (proj_pos.x > l->min[NORMAL].x &&
		    proj_pos.x < l->max[NORMAL].x &&
		    proj_pos.y > l->min[NORMAL].y &&
		    proj_pos.y < l->max[NORMAL].y) {
			/* Within full scale area; ensure not scaled view */
			if (scale == SCALED) {
				l->scale = NORMAL;
				/* Handle clearance to background for scale
				 * change */
				level_render_whole_background(l, screen);
			}
		} else if (proj_pos.x > l->min[SCALED].x &&
		           proj_pos.x < l->max[SCALED].x &&
		           proj_pos.y > l->min[SCALED].y &&
		           proj_pos.y < l->max[SCALED].y) {
			/* Within full scale area; ensure scaled view */
			if (scale == NORMAL) {
				l->scale = SCALED;
				/* Handle clearance to background for scale
				 * change */
				level_render_whole_background(l, screen);
			}
		} else {
			/* Return to unscaled view */
			if (scale == SCALED) {
				l->scale = NORMAL;
			}
			level_end_turn(l, player, screen);
			return;
		}

		level_level_to_screen_scaled(&proj_pos, &l->proj.screen[SCALED]);
		level_level_to_screen(l, &proj_pos, &l->proj.screen[NORMAL]);

		/* Render shot for this frame */
		if (l->proj.count > 0) {
			if (scale == NORMAL) {
				trail_draw(l->trails[NORMAL],
						prev[NORMAL].x,
						prev[NORMAL].y,
						l->proj.screen[NORMAL].x,
						l->proj.screen[NORMAL].y);
				trial_render(l->trails[NORMAL],
						image_get_surface(l->background[NORMAL]),
						l->colour[player]);
			}

			trail_draw(l->trails[SCALED],
					prev[SCALED].x,
					prev[SCALED].y,
					l->proj.screen[SCALED].x,
					l->proj.screen[SCALED].y);
			trial_render(l->trails[SCALED],
					image_get_surface(l->background[SCALED]),
					l->colour[player]);

			level_plot_bg_box(screen,
					image_get_surface(l->background[scale]),
					prev[scale].x,
					prev[scale].y,
					l->proj.screen[scale].x,
					l->proj.screen[scale].y);
		}
		l->proj.scale = scale;
		draw_shot_3x3(screen,
				l->proj.screen[scale].x,
				l->proj.screen[scale].y, l->proj.colour);
		l->proj.count++;

		/* If scaled, can't hit player, so escape */
		if (scale == SCALED)
			return;

		if (level_has_hit_player(l, &l->player[PLAYERS_1][NORMAL])) {
			/* Hit player 1 */
			level_set_state(l, LEVEL_WIN_P2);
			return;
		}

		if (level_has_hit_player(l, &l->player[PLAYERS_2][NORMAL])) {
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
	enum level_scale scale = l->scale;
	SDL_Surface *bg = level_get_bg_surface(l);

	if (flag_get(l->flags, LEV_NEED_REDRAW_FULL)) {
		level_render_whole_background(l, screen);
		flag_toggle(&l->flags, LEV_NEED_REDRAW_FULL);
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
			player_render_strength(l->p[PLAYERS_1], screen,
					l->width / 4,
					l->height / 64, width, height);
		else
			player_render_strength(l->p[PLAYERS_2], screen,
					l->width / 4,
					l->height / 64, width, height);

	} else if (flag_get(l->flags, LEV_STRENGTH_CHANGED) &&
			l->state != TURN_GET_P1_INPUT &&
			l->state != TURN_GET_P2_INPUT) {
		int width = l->width / 2;
		int height = l->height / 64;
		SDL_Surface *bg = level_get_bg_surface(l);
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

	if (!flag_get(l->flags, LEV_ANIMATION | (scale != l->scale) |
			LEV_NEED_REDRAW)) {
		/* Not showing animations */

		if (l->scale == SCALED) {
			player_render_direction_scaled(l->p[PLAYERS_1], screen, bg);
			player_render_direction_scaled(l->p[PLAYERS_2], screen, bg);
		} else {
			player_render_direction(l->p[PLAYERS_1], screen, bg);
			player_render_direction(l->p[PLAYERS_2], screen, bg);
		}

		l->prev_render_state = l->state;

		if (l->state == LEVEL_WIN_P1 || l->state == LEVEL_WIN_P2) {
			return true;
		}

		return false;
	}

	if (l->scale == SCALED) {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render_scaled(l->planets[i], screen,
					l->planet[i][SCALED].x,
					l->planet[i][SCALED].y);
		}
		SDL_Surface *bg = level_get_bg_surface(l);
		player_update_render_scaled(l->p[PLAYERS_1], screen, bg);
		player_update_render_scaled(l->p[PLAYERS_2], screen, bg);
	} else {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render(l->planets[i], screen,
					l->planet[i][NORMAL].x,
					l->planet[i][NORMAL].y);
		}
		SDL_Surface *bg = level_get_bg_surface(l);
		player_update_render(l->p[PLAYERS_1], screen, bg);
		player_update_render(l->p[PLAYERS_2], screen, bg);
	}

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
	SDL_BlitSurface(level_get_bg_surface(l), NULL, screen, NULL);

	switch (l->state) {
	case TURN_GET_P1_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1,
				l->colour[PLAYERS_1]);
		break;

	case TURN_GET_P2_INPUT:
		draw_box(screen, 0, 0, l->width - 1, l->height - 1,
				l->colour[PLAYERS_2]);
		break;

	default:
		break;
	}

	if (l->scale == SCALED) {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render_scaled(l->planets[i], screen,
					l->planet[i][SCALED].x,
					l->planet[i][SCALED].y);
		}
		SDL_Surface *bg = level_get_bg_surface(l);
		player_update_render_scaled(l->p[PLAYERS_1], screen, bg);
		player_update_render_scaled(l->p[PLAYERS_2], screen, bg);
	} else {
		for (i = 0; i < l->nplanets; i++) {
			planet_update_render(l->planets[i], screen,
					l->planet[i][NORMAL].x,
					l->planet[i][NORMAL].y);
		}
		SDL_Surface *bg = level_get_bg_surface(l);
		player_update_render(l->p[PLAYERS_1], screen, bg);
		player_update_render(l->p[PLAYERS_2], screen, bg);
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
		player_set_lighting(l->p[PLAYERS_1], flag_get(l->flags, LEV_LIGHTING));
		player_set_lighting(l->p[PLAYERS_2], flag_get(l->flags, LEV_LIGHTING));

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
		l->scale = (l->scale == NORMAL) ? SCALED : NORMAL;
		flag_set(&l->flags, LEV_NEED_REDRAW_FULL);

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
		if (!texture_get_starscape(level_get_bg(l))) {
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
			player_set_target(l->p[PLAYERS_1],
					event->motion.x, event->motion.y);

		} else if (l->state == TURN_GET_P2_INPUT) {
			player_set_target(l->p[PLAYERS_2],
					event->motion.x, event->motion.y);
		}

		return true;

	case SDL_MOUSEBUTTONDOWN:
		if (event->button.button == SDL_BUTTON_LEFT) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_set_target(l->p[PLAYERS_1],
						event->button.x,
						event->button.y);
				level_set_state(l, TURN_SHOW_P1);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_set_target(l->p[PLAYERS_2],
						event->button.x,
						event->button.y);
				level_set_state(l, TURN_SHOW_P2);
			}
		}

		if (event->button.button == SDL_BUTTON_WHEELUP) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_increase_strength(l->p[PLAYERS_1]);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_increase_strength(l->p[PLAYERS_2]);
			}
		}

		if (event->button.button == SDL_BUTTON_WHEELDOWN) {
			if (l->state == TURN_GET_P1_INPUT) {
				player_reduce_strength(l->p[PLAYERS_1]);

			} else if (l->state == TURN_GET_P2_INPUT) {
				player_reduce_strength(l->p[PLAYERS_2]);
			}
		}

		break;

	default:
		break;
	}

	/* Mouseage not handled */
	return false;
}
