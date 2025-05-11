#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <stdio.h>
#include <tft.h>

#include "tile.h"

#include <tileset.png.h>
#include <player.png.h>

#include <maps/map00.bin.h>
#include <maps/map01.bin.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

#define TILE_SIZE 8

#define NUM_MAPS 2
static const void *maps[NUM_MAPS] = {
	maps_map00_bin,
	maps_map01_bin,
};

static const Tile (*map)[MAP_COLS] = (const void *)maps_map00_bin;

struct character {
	sdk_sprite_t s;
	float speed;
};

static struct character player = {
	.s = {
		.ts = &ts_player_png,
		.ox = 3.5f,
		.oy = 6.5f,
	},
};

void game_reset(void)
{
}

void game_start(void)
{
	player.s.tile = 0;
	player.s.x = TFT_WIDTH / 2.0f;
	player.s.y = TFT_HEIGHT / 2.0f;
	player.speed = 50.0f;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.x > 0) {
		map = maps[0];
	}

	if (sdk_inputs_delta.y > 0) {
		map = maps[1];
	}

	float move_x = player.speed * sdk_inputs.joy_x / 2048.0f;
	float move_y = player.speed * sdk_inputs.joy_y / 2048.0f;

	if (fabsf(move_x) + fabs(move_y) > 0.1f) {
		int pos_x = player.s.x / TILE_SIZE;
		int pos_y = player.s.y / TILE_SIZE;

		float next_x = player.s.x + dt * move_x;
		float next_y = player.s.y + dt * move_y;

		next_x = clamp(next_x, TILE_SIZE / 4.0f, TFT_RIGHT - TILE_SIZE / 4.0f);
		next_y = clamp(next_y, TILE_SIZE / 4.0f, TFT_BOTTOM - TILE_SIZE / 4.0f);
		int next_pos_x = next_x / TILE_SIZE;
		int next_pos_y = next_y / TILE_SIZE;

		if (pos_x != next_pos_x && move_x) {
			if (move_x > 0) {
				// going right, to the next tile
				if (map[pos_y][next_pos_x].collides_left)
					next_x = ceilf(player.s.x) - 1e-3;
			} else {
				// going left, to the next tile
				if (map[pos_y][next_pos_x].collides_right)
					next_x = floorf(player.s.x);
			}
		}

		if (pos_y != next_pos_y && move_y) {
			if (move_y > 0) {
				// going down, to the next tile
				if (map[next_pos_y][pos_x].collides_up)
					next_y = ceilf(player.s.y) - 1e-3;
			} else {
				// going up, to the next tile
				if (map[next_pos_y][pos_x].collides_down)
					next_y = floorf(player.s.y);
			}
		}

		player.s.x = next_x;
		player.s.y = next_y;

		if (fabsf(move_x) > fabsf(move_y)) {
			if (move_x > 0) {
				player.s.tile = 6;
			} else {
				player.s.tile = 4;
			}
		} else {
			if (move_y > 0) {
				player.s.tile = 2;
			} else {
				player.s.tile = 0;
			}
		}

		player.s.tile &= ~1;
		player.s.tile |= (time_us_32() >> 16) & 1;
	}

	int pos_x = player.s.x / TILE_SIZE;
	int pos_y = player.s.y / TILE_SIZE;

	static bool fresh_from_teleport = false;
	Tile tile = map[pos_y][pos_x];

	if (tile.effect == TILE_EFFECT_TELEPORT) {
		if (fresh_from_teleport) {
			// Do nothing, we just teleported in.
		} else if (tile.map < NUM_MAPS) {
			map = maps[tile.map];
			player.s.x = (TILE_SIZE * tile.px) + 3.5f;
			player.s.y = (TILE_SIZE * tile.py) + 3.5f;
			fresh_from_teleport = true;
		} else {
			printf("Cannot teleport to map%02x\n", tile.map);
		}
	} else {
		fresh_from_teleport = false;
	}
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png,
				      map[y][x].tile_id);
		}
	}

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
