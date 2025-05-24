#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <tft.h>

#include "level.h"

#include <tileset.png.h>
#include <player.png.h>

#include <cover.png.h>

sdk_game_info("deep", &image_cover_png);

#define GRAY rgb_to_rgb565(127, 127, 127)

typedef struct Character {
	sdk_sprite_t s;
	float phase;
	float speed;
	float x, y;
} Character;

static Character player = {
	.s = {
		.ts = &ts_player_png,
		.ox = 7.5f,
		.oy = 14.0f,
		.x = TFT_WIDTH / 2.0f,
		.y = TFT_HEIGHT / 2.0f,
	},
};

static Level level;

static void change_floor(void)
{
	level.seed = 0x1337 + level.floor - 1;

	uint32_t t0 = time_us_32();
	level_generate(&level);
	uint32_t t1 = time_us_32();

	printf("level generated in %u us\n", (unsigned)(t1 - t0));

	player.x = level.sx * TILE_SIZE + 0.5f * TILE_SIZE;
	player.y = level.sy * TILE_SIZE + 0.5f * TILE_SIZE;
}

void game_reset(void)
{
	level.floor = 1;
	change_floor();

	player.speed = 50.0f;
	player.s.tile = 0;
}

static void move_and_slide(Character *ch, float dx, float dy)
{
	float nx = ch->x + dx;
	float ny = ch->y + dy;

	int tx = ch->x / TILE_SIZE;
	int ty = ch->y / TILE_SIZE;

	int tnx = nx / TILE_SIZE;
	int tny = ny / TILE_SIZE;

	bool x_solid = level.map[ty][tnx].solid;
	bool y_solid = level.map[tny][tx].solid;
	bool xy_solid = level.map[tny][tnx].solid;

	if (!xy_solid && !x_solid && !y_solid) {
		ch->x = clamp(ch->x + dx, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
		ch->y = clamp(ch->y + dy, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	} else if (!x_solid) {
		ch->x = clamp(ch->x + dx, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	} else if (!y_solid) {
		ch->y = clamp(ch->y + dy, 0, MAP_SIZE * TILE_SIZE - 1e-9f);
	}
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs.start) {
		player.speed = 200.0f;
	} else {
		player.speed = 50.0f;
	}

	float move_x = fabsf(sdk_inputs.jx) >= 0.25f ? player.speed * sdk_inputs.jx : 0;
	float move_y = fabsf(sdk_inputs.jy) >= 0.25f ? player.speed * sdk_inputs.jy : 0;

	if (move_x || move_y) {
		move_and_slide(&player, move_x * dt, move_y * dt);

		player.s.flip_x = 0;

		if (fabsf(move_x) > fabsf(move_y)) {
			player.s.tile = 4 + fmodf(player.phase * 4, 4);
			player.s.flip_x = (move_x < 0);
			player.phase += dt * 1.5;
		} else if (move_y > 0) {
			player.s.tile = 2 + fmodf(player.phase * 2, 2);
			player.phase += dt * 2;
		} else if (move_y < 0) {
			player.s.tile = 0 + fmodf(player.phase * 2, 2);
			player.phase += dt * 2;
		} else if (player.s.tile >= 4 && player.s.tile < 8) {
			player.s.tile = 5;
			player.phase = 0.25;
		}
	}

	int pos_x = player.x / TILE_SIZE;
	int pos_y = player.y / TILE_SIZE;

	if (pos_x == level.ex && pos_y == level.ey) {
		level.floor++;
		change_floor();
	}
}

static uint32_t xorshift_seed = 0x1337;

static inline uint32_t xorshift(void)
{
	uint32_t x = xorshift_seed;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	xorshift_seed = x;
	return x;
}

static inline uint32_t xorshift_bits(unsigned bits)
{
	return xorshift() >> (32 - bits);
}

static inline int estimate_distance(int x0, int y0, int x1, int y1)
{
	int x = abs(x1 - x0);
	int y = abs(y1 - y0);

	int taxicab = x + y;
	int chebyshev = MAX(x, y);
	return (taxicab * 7 + chebyshev * 9) >> 4;
}

static inline color_t color_mul(color_t color, uint8_t amount)
{
	return rgb_to_rgb565((rgb565_red(color) * amount) >> 8, (rgb565_green(color) * amount) >> 8,
			     (rgb565_blue(color) * amount) >> 8);
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	int pos_x = player.x / TILE_SIZE;
	int pos_y = player.y / TILE_SIZE;

	float origin_x = player.x - TFT_WIDTH / 2.0f;
	float origin_y = player.y - TFT_HEIGHT / 2.0f;

	tft_set_origin(origin_x, origin_y);

	int y0 = clamp(pos_y - TFT_HEIGHT / 2 / TILE_SIZE - 1, 0, MAP_SIZE - 1);
	int y1 = clamp(pos_y + TFT_HEIGHT / 2 / TILE_SIZE + 1, 0, MAP_SIZE - 1);
	int x0 = clamp(pos_x - TFT_WIDTH / 2 / TILE_SIZE - 1, 0, MAP_SIZE - 1);
	int x1 = clamp(pos_x + TFT_WIDTH / 2 / TILE_SIZE + 1, 0, MAP_SIZE - 1);

	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      level.map[y][x].tile_id);
		}
	}

	static int avg_range = 40 << 8;
	int range = (46 << 8) + xorshift_bits(4 + 8);
	avg_range += (range - avg_range) >> 3;
	range = avg_range >> 8;

	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			int dist = estimate_distance(TFT_WIDTH / 2, TFT_HEIGHT / 2, x, y);
			dist += xorshift_bits(2);

			if (dist > range) {
				dist -= range;
				int mag = MAX(0, 255 - ((dist * dist) >> 3));
				tft_input[x][y] = color_mul(tft_input[x][y], mag);
			}
		}
	}

	tft_set_origin(0, 0);

	// tft_draw_string(3, 3, rgb_to_rgb565(255, 0, 0), "%i", level.floor);
	sdk_draw_sprite(&player.s);
}

int main()
{
	struct sdk_config config = {
		.wait_for_usb = true,
		.show_fps = true,
		.off_on_select = true,
		.fps_color = GRAY,
	};

	sdk_main(&config);
}
