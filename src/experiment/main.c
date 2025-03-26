#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

#include "common.h"

#include <tileset.png.h>
#include <player.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

#define TILE_SIZE 8

extern int maps_map1[MAP_ROWS][MAP_COLS];
extern int maps_map2[MAP_ROWS][MAP_COLS];

static int (*map)[MAP_COLS] = maps_map1;

struct character {
	sdk_sprite_t s;
	float speed;
};

static struct character player = {
	.s = {
		.ts = &ts_player_png,
		.ox = 3.5f,
		.oy = 3.5f,
	},
};

void game_reset(void)
{
}

void game_start(void)
{
	sdk_set_output_gain_db(6);

	player.s.tile = 0;
	player.s.x = TFT_WIDTH / 2.0f;
	player.s.y = TFT_HEIGHT / 2.0f;
	player.speed = 50.0f;
}

void game_audio(int nsamples)
{
	(void)nsamples;
}

void game_input(unsigned dt_usec)
{
	float dt = dt_usec / 1000000.0f;

	if (sdk_inputs_delta.x > 0) {
		map = maps_map1;
	}

	if (sdk_inputs_delta.y > 0) {
		map = maps_map2;
	}

	float move_x = player.speed * sdk_inputs.joy_x / 2048.0f;
	float move_y = player.speed * sdk_inputs.joy_y / 2048.0f;

	if (fabsf(move_x) + fabs(move_y) > 0.1f) {
		player.s.x += dt * move_x;
		player.s.y += dt * move_y;

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
}

void game_paint(unsigned __unused dt_usec)
{
	tft_fill(rgb_to_rgb565(0, 0, 0));

	for (int y = 0; y < MAP_ROWS; y++) {
		for (int x = 0; x < MAP_COLS; x++) {
			sdk_draw_tile(x * TILE_SIZE, y * TILE_SIZE, &ts_tileset_png, map[y][x]);
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
