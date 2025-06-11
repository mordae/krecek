#pragma once
#include <math.h>
#include <pico/stdlib.h>
#include <sdk/image.h>

struct sdk_rect {
	int x0, x1;
	int y0, y1;
};

typedef struct sdk_rect sdk_rect_t;

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

inline static void sdk_draw_sprite(const sdk_sprite_t *s)
{
	bool a = s->angle & 1;
	bool b = s->angle >> 1;

	bool flip_x = s->flip_x ^ b;
	bool flip_y = s->flip_y ^ (b ^ a);
	bool swap_xy = s->swap_xy ^ a;

	sdk_draw_tile_flipped(s->x - s->ox, s->y - s->oy, s->ts, s->tile, flip_x, flip_y, swap_xy);
}

inline static sdk_rect_t sdk_rect_intersect(sdk_rect_t a, sdk_rect_t b)
{
	a.x0 = MAX(a.x0, b.x0);
	a.x1 = MIN(a.x1, b.x1);
	a.y0 = MAX(a.y0, b.y0);
	a.y1 = MIN(a.y1, b.y1);
	return a;
}

inline static sdk_rect_t sdk_sprite_get_overlap(const sdk_sprite_t *a, const sdk_sprite_t *b)
{
	sdk_rect_t ra;
	ra.x0 = a->x - a->ox;
	ra.y0 = a->y - a->oy;
	ra.x1 = ra.x0 + a->ts->width;
	ra.y1 = ra.y0 + a->ts->height;

	sdk_rect_t rb;
	rb.x0 = b->x - b->ox;
	rb.y0 = b->y - b->oy;
	rb.x1 = rb.x0 + b->ts->width;
	rb.y1 = rb.y0 + b->ts->height;

	return sdk_rect_intersect(ra, rb);
}

inline static color_t sdk_sprite_sample_world(const sdk_sprite_t *s, int x, int y)
{
	bool a = s->angle & 1;
	bool b = s->angle >> 1;

	bool flip_x = s->flip_x ^ b;
	bool flip_y = s->flip_y ^ (b ^ a);
	bool swap_xy = s->swap_xy ^ a;

	int w = s->ts->width;
	int h = s->ts->height;

	int rx = x - floorf(s->x - s->ox);
	int ry = y - floorf(s->y - s->oy);

	int sx, sy;

	if (swap_xy) {
		sy = flip_y ? (h - 1) - rx : rx;
		sx = flip_x ? (w - 1) - ry : ry;
	} else {
		sx = flip_x ? (w - 1) - rx : rx;
		sy = flip_y ? (h - 1) - ry : ry;
	}

	if (sx < 0 || sy < 0 || sx >= w || sy >= h)
		return TRANSPARENT;

	const color_t *data = sdk_get_tile_data(s->ts, s->tile);
	return data[sy * w + sx];
}

inline static bool sdk_sprites_collide_bbox(const sdk_sprite_t *s1, const sdk_sprite_t *s2)
{
	sdk_rect_t overlap = sdk_sprite_get_overlap(s1, s2);

	if (overlap.x1 <= overlap.x0 || overlap.y1 <= overlap.y0)
		return false;

	return true;
}

inline static int sdk_sprites_collide(const sdk_sprite_t *s1, const sdk_sprite_t *s2)
{
	sdk_rect_t overlap = sdk_sprite_get_overlap(s1, s2);

	if (overlap.x1 <= overlap.x0 || overlap.y1 <= overlap.y0)
		return 0;

	int points = 0;

	for (int y = overlap.y0; y < overlap.y1; y++) {
		for (int x = overlap.x0; x < overlap.x1; x++) {
			color_t c1 = sdk_sprite_sample_world(s1, x, y);

			if (TRANSPARENT == c1)
				continue;

			color_t c2 = sdk_sprite_sample_world(s2, x, y);

			if (TRANSPARENT == c2)
				continue;

			points++;
		}
	}

	return points;
}
