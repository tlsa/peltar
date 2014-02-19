
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "fixed-point.h"
#include "planet.h"
#include "image.h"
#include "texture/starscape.h"
#include "types.h"

struct image {
	SDL_Surface *render; /* Image bitmap data */

	int width; /* Width of image */
	int height; /* Height of image */
};


static bool image_create_details(struct image *image, int width, int height)
{

	/* Create surface for rendered image */
	image->render = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height,
			peltar_opts.screen_depth, 0, 0, 0, 0);
	if (image->render == NULL) {
		return false;
	}

	/* Texture dimensions */
	image->width = width;
	image->height = height;

	return true;
}


bool image_create(struct image **image, int width, int height)
{
	*image = malloc(sizeof(struct image));
	if (*image == NULL)
		return false;

	(*image)->render = NULL;

	if (!image_create_details(*image, width, height)) {
		image_free(*image);
		return false;
	}

	if (!texture_get_starscape(*image)) {
		image_free(*image);
		return false;
	}

	return true;
}


void image_free(struct image *image)
{
	assert(image != NULL);

	if (image->render != NULL)	
		SDL_FreeSurface(image->render);

	free(image);
}


SDL_Surface * image_get_surface(const struct image *image)
{
	return image->render;
}


int image_get_width(const struct image *image)
{
	return image->width;
}


int image_get_height(const struct image *image)
{
	return image->height;
}


