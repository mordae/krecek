#pragma once
#include <pico/stdlib.h>
#include <sdk/image.h>

struct sdk_sprite {
	float x, y;   // Sprite coordinates
	float ox, oy; // Origin offset

	const sdk_tileset_t *ts; // Tileset to use
	uint16_t tile;		 // Current tile

	uint16_t angle : 2;   // Rotation angle (in 90° increments, clockwise)
	uint16_t flip_x : 1;  // Flip tile horizontally
	uint16_t flip_y : 1;  // Flip tile vertically
	uint16_t swap_xy : 1; // Flip tile x/y axes
	uint16_t : 11;
};

typedef struct sdk_sprite sdk_sprite_t;

inline static void __unused sdk_draw_sprite(const sdk_sprite_t *s)
{
	bool a = s->angle & 1;
	bool b = s->angle >> 1;

	bool flip_x = s->flip_x ^ b;
	bool flip_y = s->flip_y ^ (b ^ a);
	bool swap_xy = s->swap_xy ^ a;

	sdk_draw_tile_flipped(s->x - s->ox, s->y - s->oy, s->ts, s->tile, flip_x, flip_y, swap_xy);
}

inline static bool __unused sdk_sprite_is_opaque_xy(const sdk_sprite_t *s, int sx, int sy)
{
	if (sx < 0 || sx >= s->ts->width)
		return false;

	if (sy < 0 || sy >= s->ts->height)
		return false;

	const color_t *data = sdk_get_tile_data(s->ts, s->tile);
	return data[sy * s->ts->width + sx] != TRANSPARENT;
}

inline static int __unused sdk_sprites_collide(const sdk_sprite_t *s1, const sdk_sprite_t *s2)
{
	int points = 0;

	int s1x = s1->x - s1->ox;
	int s1y = s1->y - s1->oy;
	int s1w = s1->ts->width;
	int s1h = s1->ts->height;

	int s2x = s2->x - s2->ox;
	int s2y = s2->y - s2->oy;
	int s2w = s2->ts->width;
	int s2h = s2->ts->height;

	if (s1x + s1w <= s2x || s2x + s2w <= s1x)
		return 0;

	if (s1y + s1h <= s2y || s2y + s2h <= s1y)
		return 0;

	for (int y = s1y; y < s1y + s1w; y++) {
		for (int x = s1x; x < s1x + s1h; x++) {
			int s1lx = x - s1x;
			int s1ly = y - s1y;

			int s2lx = x - s2x;
			int s2ly = y - s2y;

			if (sdk_sprite_is_opaque_xy(s1, s1lx, s1ly) &&
			    sdk_sprite_is_opaque_xy(s2, s2lx, s2ly))
				points++;
		}
	}

	return points;
}
