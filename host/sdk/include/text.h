#pragma once
#include <SDL.h>

typedef enum Text_Alignment {
	TEXT_ALIGN_START = 0,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_END,
} Text_Alignment;

/* Initialize text rendering. */
int Text_Init(void);

/*
 * Set text scale. Should correspond to DPI scale.
 * We assume 96 is the base DPI.
 * Caching it will drop cached surfaces.
 */
void Text_SetScale(float scale_x, float scale_y);

/*
 * Set horizontal and vertical text alignment.
 * This does not affect the arrangement of text inside the bounding box,
 * only position of the bounding box relative to the x, y coordinates.
 */
void Text_SetAlignment(Text_Alignment new_halign, Text_Alignment new_valign);

/*
 * Set current renderer.
 * Caching it will drop cached surfaces.
 */
void Text_SetRenderer(SDL_Renderer *renderer);

/*
 * Draw given markup at specified coordinates.
 */
int Text_RendererDrawMarkup(SDL_Renderer *renderer, int x, int y, const char *text);

/*
 * Drop cached surfaces.
 */
void Text_DropCache(void);

/*
 * Retrieve cached surface for given markup.
 */
SDL_Surface *Text_GetCachedSurface(const char *markup, int *tw, int *th);

/*
 * Retrieve cached surface for given markup or create a new one.
 */
SDL_Surface *Text_CacheSurface(const char *markup, int *tw, int *th);
