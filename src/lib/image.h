
#ifndef _PELTAR_IMAGE_H_
#define _PELTAR_IMAGE_H_

#include <stdbool.h>
#include <SDL.h>

struct image;

bool image_create(struct image **image, int width, int height);
bool image_clone(const struct image *orig, struct image **image);
void image_free(struct image *image);

SDL_Surface * image_get_surface(const struct image *image);
int image_get_width(const struct image *image);
int image_get_height(const struct image *image);
#endif

