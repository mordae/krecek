#pragma once
#include <pico/stdlib.h>

#include <sdk/panic.h>

#include <tft.h>

struct sdk_file {
	const void *data;
	int len;
};

struct sdk_tileset {
	const uint8_t *data;
	uint8_t last;
	uint8_t w;
	uint8_t h;
	uint8_t trsp;
};

typedef const struct sdk_file sdk_file_t;
typedef const struct sdk_tileset sdk_tileset_t;

#if defined(RUNNING_ON_HOST)
#define SDK_EMBED_SECTION ".pushsection \".rodata\",\"a\",@progbits\n"
#define SDK_EMBED_END ".popsection\n"
#define SDK_EMBED_POINTER ".quad"
#else
#define SDK_EMBED_SECTION ".section \".flashdata.files\"\n"
#define SDK_EMBED_END ""
#define SDK_EMBED_POINTER ".int"
asm(".section \".flashdata.files\", \"a\"");
#endif

#define SDK_TO_STRING_(x) #x
#define SDK_TO_STRING(x) SDK_TO_STRING_((x))

#define embed_file(name, path)                                         \
	extern const struct sdk_file name;                             \
	asm(".align 4\n");                                             \
	asm(SDK_EMBED_SECTION);                                        \
	asm("_" #name "_start:\n");                                    \
	asm(".incbin \"" path "\"\n");                                 \
	asm("_" #name "_end:\n");                                      \
	asm(".align 4\n");                                             \
	asm(#name ":\n");                                              \
	asm(SDK_EMBED_POINTER " _" #name "_start\n");                  \
	asm(SDK_EMBED_POINTER " _" #name "_end - _" #name "_start\n"); \
	asm(SDK_EMBED_END)

#define embed_tileset(name, c, w, h, t, path)         \
	extern const struct sdk_tileset name;         \
	asm(".align 4\n");                            \
	asm(SDK_EMBED_SECTION);                       \
	asm("_" #name "_start:\n");                   \
	asm(".incbin \"" path "\"\n");                \
	asm(".align 4\n");                            \
	asm(#name ":\n");                             \
	asm(SDK_EMBED_POINTER " _" #name "_start\n"); \
	asm(".byte " SDK_TO_STRING((c) - 1) "\n");    \
	asm(".byte " SDK_TO_STRING((w)) "\n");        \
	asm(".byte " SDK_TO_STRING((h)) "\n");        \
	asm(".byte " SDK_TO_STRING((t)) "\n");        \
	asm(SDK_EMBED_END)

/* Get pointer to given tile inside the tileset. */
inline static const uint8_t *sdk_get_tile_data(sdk_tileset_t *ts, uint8_t tile)
{
	if (tile > ts->last)
		sdk_panic("sdk: attempted to access non-existent tile %p[%hhu]", ts, tile);

	const uint8_t *data = ts->data + tile * ts->h * ts->w;

#if defined(XIP_BASE)
	/* Make sure not to cache data from the flash. */
	if ((unsigned)data >= XIP_BASE && (unsigned)data < XIP_NOALLOC_BASE)
		return data + XIP_NOALLOC_BASE - XIP_BASE;
#endif

	return data;
}

/* Draw a tile from given tileset, possibly mirrored or with flipped x/y. */
inline static void sdk_draw_tile_flipped(int x, int y, sdk_tileset_t *ts, uint8_t tile, bool flip_x,
					 bool flip_y, bool swap_xy)
{
	const uint8_t *data = sdk_get_tile_data(ts, tile);
	tft_draw_sprite_flipped(x, y, ts->w, ts->h, data, ts->trsp, flip_x, flip_y, swap_xy);
}

/* Draw a tile from given tileset. */
inline static void __unused sdk_draw_tile(int x, int y, sdk_tileset_t *ts, int tile)
{
	sdk_draw_tile_flipped(x, y, ts, tile, false, false, false);
}

/* Draw rotated tile, where angle is times 90° clockwise. */
inline static void __unused sdk_draw_tile_rotated(int x, int y, sdk_tileset_t *ts, int tile,
						  int angle)
{
	bool a = angle & 1;
	bool b = (angle >> 1) & 1;
	sdk_draw_tile_flipped(x, y, ts, tile, b, b ^ a, a);
}
