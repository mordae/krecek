#pragma once
#include <pico/stdlib.h>

#include <sdk/panic.h>

#include <tft.h>

#if !defined(TRANSPARENT)
#define TRANSPARENT rgb_to_rgb565(255, 63, 255)
#endif

struct sdk_image {
	uint16_t width;
	uint16_t height;
	color_t data[];
};

typedef struct sdk_image sdk_image_t;

struct sdk_tileset {
	uint16_t width;
	uint16_t height;
	uint16_t count;
	const color_t *tiles[];
};

typedef struct sdk_tileset sdk_tileset_t;

/* Get pointer to given tile inside the tileset. */
inline static const color_t *sdk_get_tile_data(const sdk_tileset_t *ts, uint16_t tile)
{
	tile = tile % ts->count;
	return ts->tiles[tile];
}

/* Draw a tile from given tileset, possibly mirrored or with flipped x/y. */
inline static void sdk_draw_tile_flipped(int x, int y, const sdk_tileset_t *ts, uint16_t tile,
					 bool flip_x, bool flip_y, bool swap_xy)
{
	const color_t *data = sdk_get_tile_data(ts, tile);
	tft_draw_sprite_flipped(x, y, ts->width, ts->height, data, TRANSPARENT, flip_x, flip_y,
				swap_xy);
}

/* Draw a tile from given tileset. */
inline static void __unused sdk_draw_tile(int x, int y, const sdk_tileset_t *ts, uint16_t tile)
{
	sdk_draw_tile_flipped(x, y, ts, tile, false, false, false);
}

/* Draw rotated tile, where angle is times 90° clockwise. */
inline static void __unused sdk_draw_tile_rotated(int x, int y, const sdk_tileset_t *ts,
						  uint16_t tile, int angle)
{
	bool a = angle & 1;
	bool b = (angle >> 1) & 1;
	sdk_draw_tile_flipped(x, y, ts, tile, b, b ^ a, a);
}
