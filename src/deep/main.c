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

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs.a) {
		player.speed = 200.0f;
	} else {
		player.speed = 50.0f;
	}

	float move_x =
		abs(sdk_inputs.joy_x) >= 512 ? player.speed * dt * sdk_inputs.joy_x / 2048.0f : 0;
	float move_y =
		abs(sdk_inputs.joy_y) >= 512 ? player.speed * dt * sdk_inputs.joy_y / 2048.0f : 0;

	if (move_x || move_y) {
		int pos_x = player.x / TILE_SIZE;
		int pos_y = player.y / TILE_SIZE;

		float next_x = player.x + move_x;
		float next_y = player.y + move_y;

		next_x = clamp(next_x, 0, TILE_SIZE * MAP_SIZE - 1);
		next_y = clamp(next_y, 0, TILE_SIZE * MAP_SIZE - 1);
		int next_pos_x = next_x / TILE_SIZE;
		int next_pos_y = next_y / TILE_SIZE;

		if (pos_x != next_pos_x && move_x) {
			if (level.map[pos_y][next_pos_x].solid) {
				if (move_x > 0) {
					next_x = ceilf(player.x) - 1e-3;
				} else {
					next_x = floor(player.x);
				}
			}
		}

		if (pos_y != next_pos_y && move_y) {
			if (level.map[next_pos_y][pos_x].solid) {
				if (move_y > 0) {
					next_y = ceilf(player.y) - 1e-3;
				} else {
					next_y = floor(player.y);
				}
			}
		}

		player.x = next_x;
		player.y = next_y;

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

	tft_set_origin(0, 0);

	tft_draw_string(3, 3, rgb_to_rgb565(255, 0, 0), "%i", level.floor);
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
