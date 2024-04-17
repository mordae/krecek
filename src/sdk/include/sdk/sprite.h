#pragma once
#include <pico/stdlib.h>

#include <sdk/embed.h>

struct sdk_sprite {
	float x, y;   // Sprite coordinates
	float ox, oy; // Origin offset

	sdk_tileset_t *ts; // Tileset to use
	uint8_t tile;	   // Current tile

	uint8_t angle : 2;   // Rotation angle (in 90° increments, clockwise)
	uint8_t flip_x : 1;  // Flip tile horizontally
	uint8_t flip_y : 1;  // Flip tile vertically
	uint8_t swap_xy : 1; // Flip tile x/y axes
	uint8_t : 3;
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
