#include <pico/stdlib.h>
#include <math.h>

#include <sdk.h>
#include <tft.h>

#include "common.h"

#include <tileset.png.h>
#include <player.png.h>

#define GRAY rgb_to_rgb565(127, 127, 127)

#define TILE_SIZE 8

extern uint32_t maps_map1[MAP_ROWS][MAP_COLS];
extern uint32_t maps_map2[MAP_ROWS][MAP_COLS];

static Tile (*map)[MAP_COLS] = (void *)maps_map2;

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
		map = (void *)maps_map1;
	}

	if (sdk_inputs_delta.y > 0) {
		map = (void *)maps_map2;
	}

	float move_x = player.speed * sdk_inputs.joy_x / 2048.0f;
	float move_y = player.speed * sdk_inputs.joy_y / 2048.0f;

	if (fabsf(move_x) + fabs(move_y) > 0.1f) {
		int pos_x = player.s.x / TILE_SIZE;
		int pos_y = (player.s.y + 4) / TILE_SIZE;

		float next_x = player.s.x + dt * move_x;
		float next_y = player.s.y + dt * move_y;

		next_x = clamp(next_x, TILE_SIZE / 2.0f - 2, TFT_WIDTH - TILE_SIZE / 2.0f + 1);
		next_y = clamp(next_y, TILE_SIZE / 2.0f, TFT_HEIGHT - 1 - TILE_SIZE / 2.0f);

		// TODO: this does not work very well, fix it
		int next_pos_x = (next_x + (move_x > 0 ? 2 : -2)) / TILE_SIZE;
		int next_pos_y = (next_y + (move_y > 0 ? 4 : 0)) / TILE_SIZE;

		if (pos_x != next_pos_x && move_x) {
			//if (map[pos_y][next_pos_x].collides_up == 1) {
			if (move_x > 0) {
				// going right, to the next tile
				if (map[pos_y][next_pos_x].collides_left)
					next_x = player.s.x;
			} else {
				// going left, to the next tile
				if (map[pos_y][next_pos_x].collides_right)
					next_x = player.s.x;
			}
		}

		if (pos_y != next_pos_y && move_y) {
			if (move_y > 0) {
				// going down, to the next tile
				if (map[next_pos_y][pos_x].collides_up)
					next_y = player.s.y;
			} else {
				// going up, to the next tile
				if (map[next_pos_y][pos_x].collides_down)
					next_y = player.s.y;
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

	if (map[pos_y][pos_x].effect == TILE_EFFECT_TELEPORT) {
		switch (map[pos_y][pos_x].parameter) {
		case 0:
			map = (void *)maps_map1;
			break;

		case 1:
			map = (void *)maps_map2;
			break;
		}
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
