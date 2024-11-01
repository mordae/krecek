#include <SDL.h>
#include <SDL2_Pango.h>

#include "text.h"

static SDLPango_Context *context;

static Text_Alignment halign = TEXT_ALIGN_START;
static Text_Alignment valign = TEXT_ALIGN_START;

static float scaleX = 1.0;
static float scaleY = 1.0;

#define MAX_CACHE_DEPTH 256

typedef struct Text_Cache {
	char *markup;
	int hash;
	SDL_Surface *surface;
	int tw, th;
	struct Text_Cache *next;
	struct Text_Cache *prev;
} Text_Cache;

static Text_Cache *cache = NULL;
static int cache_depth = 0;

static int hash_markup(const char *markup)
{
	int res = 0;

	do {
		res += *markup;
	} while (*markup++);

	return res;
}

static void cache_insert(Text_Cache *item)
{
	if (cache) {
		item->prev = cache->prev;
		item->next = cache;

		cache->prev->next = item;
		cache->prev = item;

		cache = item;
	} else {
		item->prev = item;
		item->next = item;
		cache = item;
	}

	cache_depth++;
}

static Text_Cache *cache_remove(Text_Cache *item)
{
	Text_Cache *prev = item->prev;
	Text_Cache *next = item->next;

	prev->next = item->next;
	next->prev = item->prev;

	if (item == cache)
		cache = next;

	if (item == cache)
		cache = NULL;

	item->next = item;
	item->prev = item;

	cache_depth--;
	return item;
}

static void cache_promote(Text_Cache *item)
{
	cache_insert(cache_remove(item));
}

static void cache_prune(void)
{
	while (cache_depth > MAX_CACHE_DEPTH) {
		Text_Cache *item = cache_remove(cache->prev);
		SDL_FreeSurface(item->surface);
		free(item->markup);
		free(item);
	}
}

int Text_Init(void)
{
	SDL_ClearError();

	if (SDLPango_Init()) {
		SDL_SetError("SDLPango_Init: %s", SDL_GetError());
		return -1;
	}

	if (!(context = SDLPango_CreateContext())) {
		SDL_SetError("SDLPango_CreateContext: %s", SDL_GetError());
		return -1;
	}

	SDLPango_SetDefaultColor(context, MATRIX_TRANSPARENT_BACK_WHITE_LETTER);
	SDLPango_SetMinimumSize(context, -1, -1);

	return 0;
}

void Text_DropCache(void)
{
	SDL_ClearError();

	while (cache) {
		Text_Cache *item = cache_remove(cache);
		SDL_FreeSurface(item->surface);
		free(item->markup);
		free(item);
	}
}

void Text_SetScale(float scale_x, float scale_y)
{
	if ((scaleX != scale_x) || (scaleY != scale_y))
		Text_DropCache();

	scaleX = scale_x;
	scaleY = scale_y;
	SDLPango_SetDpi(context, 96.0 * scaleX, 96.0 * scaleY);
}

void Text_SetAlignment(Text_Alignment new_halign, Text_Alignment new_valign)
{
	halign = new_halign;
	valign = new_valign;
}

SDL_Surface *Text_GetCachedSurface(const char *markup, int *tw, int *th)
{
	SDL_ClearError();

	Text_Cache *iter = cache;
	int hash = hash_markup(markup);

	while (iter) {
		if ((iter->hash == hash) && !strcmp(iter->markup, markup)) {
			*tw = iter->tw;
			*th = iter->th;
			cache_promote(iter);
			return iter->surface;
		}

		iter = iter->next;
		if (iter == cache)
			break;
	}

	return NULL;
}

SDL_Surface *Text_CacheSurface(const char *markup, int *tw, int *th)
{
	SDL_ClearError();

	SDL_Surface *surface = Text_GetCachedSurface(markup, tw, th);

	if (!surface) {
		SDLPango_SetMarkup(context, markup, -1);

		surface = SDLPango_CreateSurfaceDraw(context);
		if (!surface) {
			SDL_SetError("SDLPango_CreateSurfaceDraw: %s", SDL_GetError());
			return NULL;
		}

		*tw = SDLPango_GetLayoutWidth(context) / scaleX;
		*th = SDLPango_GetLayoutHeight(context) / scaleY;

		Text_Cache *item = malloc(sizeof(*item));
		item->markup = strdup(markup);
		item->hash = hash_markup(markup);
		item->surface = surface;
		item->tw = *tw;
		item->th = *th;
		cache_insert(item);
	}

	cache_prune();

	return surface;
}

int Text_RendererDrawMarkup(SDL_Renderer *renderer, int x, int y, const char *text)
{
	SDL_ClearError();

	int tw, th;
	SDL_Surface *surface = Text_CacheSurface(text, &tw, &th);

	if (!surface) {
		SDL_SetError("Text_CacheSurface: %s", SDL_GetError());
		return -1;
	}

	SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surface);
	if (!tex) {
		SDL_SetError("SDL_CreateTextureFromSurface: %s", SDL_GetError());
		return -1;
	}

	SDL_Rect rect = { x, y, tw, th };

	switch (halign) {
	case TEXT_ALIGN_START:
		break;

	case TEXT_ALIGN_CENTER:
		rect.x = x - tw / 2;
		break;

	case TEXT_ALIGN_END:
		rect.x = x - tw;
		break;
	}

	switch (valign) {
	case TEXT_ALIGN_START:
		break;

	case TEXT_ALIGN_CENTER:
		rect.y = y - th / 2;
		break;

	case TEXT_ALIGN_END:
		rect.y = y - th;
		break;
	}

	int res = SDL_RenderCopy(renderer, tex, NULL, &rect);
	SDL_DestroyTexture(tex);
	return res;
}
